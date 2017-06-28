/**
 ******************************************************************************
 *
 * @file       configattitudewidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update settings in the firmware
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */
#include "configattitudewidget.h"
#include "physical_constants.h"

#include "math.h"
#include <QDebug>
#include <QTimer>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QThread>
#include <QErrorMessage>
#include <iostream>
#include <QDesktopServices>
#include <QUrl>
#include <coreplugin/iboardtype.h>
#include <attitudesettings.h>
#include <sensorsettings.h>
#include <inssettings.h>
#include <homelocation.h>
#include <accels.h>
#include <gyros.h>
#include <magnetometer.h>
#include <baroaltitude.h>

#include "assertions.h"
#include "calibration.h"

#define sign(x) ((x < 0) ? -1 : 1)

// Uncomment this to enable 6 point calibration on the accels
#define SIX_POINT_CAL_ACCEL

const double ConfigAttitudeWidget::maxVarValue = 0.1;

// *****************

class Thread : public QThread
{
public:
    static void usleep(unsigned long usecs) { QThread::usleep(usecs); }
};

// *****************

ConfigAttitudeWidget::ConfigAttitudeWidget(QWidget *parent)
    : ConfigTaskWidget(parent)
    , m_ui(new Ui_AttitudeWidget())
    , board_has_accelerometer(false)
    , board_has_magnetometer(false)
{
    m_ui->setupUi(this);

    // Initialization of the Paper plane widget
    m_ui->sixPointHelp->setScene(new QGraphicsScene(this));

    paperplane = new QGraphicsSvgItem();
    paperplane->setSharedRenderer(new QSvgRenderer());
    paperplane->renderer()->load(QString(":/configgadget/images/paper-plane.svg"));
    paperplane->setElementId("plane-horizontal");
    m_ui->sixPointHelp->scene()->addItem(paperplane);
    m_ui->sixPointHelp->setSceneRect(paperplane->boundingRect());

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr = pm->getObject<UAVObjectUtilManager>();
    Q_ASSERT(utilMngr);
    if (utilMngr != NULL) {
        Core::IBoardType *board = utilMngr->getBoardType();
        if (board != NULL) {
            board_has_accelerometer =
                board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_ACCELS);
            board_has_magnetometer =
                board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_MAGS);
        }
    }

    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();
    if (!settings->useExpertMode())
        m_ui->applyButton->setVisible(false);

    // Must set up the UI (above) before setting up the UAVO mappings or refreshWidgetValues
    // will be dealing with some null pointers
    addUAVObject("AttitudeSettings");
    addUAVObject("SensorSettings");
    if (board_has_magnetometer) {
        addUAVObject("INSSettings");
    }
    autoLoadWidgets();

    // Configure the calibration object
    calibration.initialize(board_has_accelerometer, board_has_magnetometer);

    // Configure the calibration UI
    m_ui->cbCalibrateAccels->setChecked(board_has_accelerometer);
    m_ui->cbCalibrateMags->setChecked(board_has_magnetometer);
    if (!board_has_accelerometer
        || !board_has_magnetometer) { // If both are not available, don't provide any choices.
        m_ui->cbCalibrateAccels->setEnabled(false);
        m_ui->cbCalibrateMags->setEnabled(false);
    }

    // Must connect the graphs to the calibration object to see the calibration results
    calibration.configureTempCurves(m_ui->xGyroTemp, m_ui->yGyroTemp, m_ui->zGyroTemp);

    // Connect the signals
    connect(m_ui->yawOrientationStart, &QAbstractButton::clicked, &calibration,
            &Calibration::doStartOrientation);
    connect(m_ui->levelingStart, &QAbstractButton::clicked, &calibration,
            &Calibration::doStartNoBiasLeveling);
    connect(m_ui->levelingAndBiasStart, &QAbstractButton::clicked, &calibration,
            &Calibration::doStartBiasAndLeveling);
    connect(m_ui->sixPointStart, &QAbstractButton::clicked, &calibration,
            &Calibration::doStartSixPoint);
    connect(m_ui->sixPointSave, &QAbstractButton::clicked, &calibration,
            &Calibration::doSaveSixPointPosition);
    connect(m_ui->sixPointCancel, &QAbstractButton::clicked, &calibration,
            &Calibration::doCancelSixPoint);
    connect(m_ui->cbCalibrateAccels, &QAbstractButton::clicked, this,
            &ConfigAttitudeWidget::configureSixPoint);
    connect(m_ui->cbCalibrateMags, &QAbstractButton::clicked, this,
            &ConfigAttitudeWidget::configureSixPoint);
    connect(m_ui->startTempCal, &QAbstractButton::clicked, &calibration,
            &Calibration::doStartTempCal);
    connect(m_ui->acceptTempCal, &QAbstractButton::clicked, &calibration,
            &Calibration::doAcceptTempCal);
    connect(m_ui->cancelTempCal, &QAbstractButton::clicked, &calibration,
            &Calibration::doCancelTempCalPoint);
    connect(m_ui->tempCalRange, QOverload<int>::of(&QSpinBox::valueChanged), &calibration,
            &Calibration::setTempCalRange);
    // only allow the calibration to be accepted after min temperature change
    m_ui->acceptTempCal->setEnabled(false);
    connect(&calibration, &Calibration::tempCalProgressChanged, this, [this](int progress) {
        if (!m_ui->acceptTempCal)
            return;
        if (progress >= 100)
            m_ui->acceptTempCal->setEnabled(true);
        else if (m_ui->acceptTempCal->isEnabled())
            m_ui->acceptTempCal->setEnabled(false);
    });
    calibration.setTempCalRange(m_ui->tempCalRange->value());

    // Let calibration update the UI
    connect(&calibration, &Calibration::yawOrientationProgressChanged, m_ui->pb_yawCalibration,
            &QProgressBar::setValue);
    connect(&calibration, &Calibration::levelingProgressChanged, m_ui->accelBiasProgress,
            &QProgressBar::setValue);
    connect(&calibration, &Calibration::tempCalProgressChanged, m_ui->tempCalProgress,
            &QProgressBar::setValue);
    connect(&calibration, &Calibration::showTempCalMessage, m_ui->tempCalMessage, &QLabel::setText);
    connect(&calibration, &Calibration::sixPointProgressChanged, m_ui->sixPointProgress,
            &QProgressBar::setValue);
    connect(&calibration, &Calibration::showSixPointMessage, m_ui->sixPointCalibInstructions,
            &QTextEdit::setText);
    connect(&calibration, &Calibration::updatePlane, this, &ConfigAttitudeWidget::displayPlane);

    // Let the calibration gadget control some control enables
    connect(&calibration, &Calibration::toggleSavePosition, m_ui->sixPointSave,
            &QWidget::setEnabled);
    connect(&calibration, &Calibration::toggleControls, m_ui->sixPointStart, &QWidget::setEnabled);
    connect(&calibration, &Calibration::toggleControls, m_ui->sixPointCancel,
            &QWidget::setDisabled);
    connect(&calibration, &Calibration::toggleControls, m_ui->yawOrientationStart,
            &QWidget::setEnabled);
    connect(&calibration, &Calibration::toggleControls, m_ui->levelingStart, &QWidget::setEnabled);
    connect(&calibration, &Calibration::toggleControls, m_ui->levelingAndBiasStart,
            &QWidget::setEnabled);
    connect(&calibration, &Calibration::toggleControls, m_ui->startTempCal, &QWidget::setEnabled);
    connect(&calibration, &Calibration::toggleControls, m_ui->acceptTempCal, &QWidget::setDisabled);
    connect(&calibration, &Calibration::toggleControls, m_ui->cancelTempCal, &QWidget::setDisabled);

    // Let the calibration gadget mark the tab as dirty, i.e. having unsaved data.
    connect(&calibration, &Calibration::calibrationCompleted, this,
            &ConfigAttitudeWidget::do_SetDirty);

    // Let the calibration class mark the widget as busy
    connect(&calibration, &Calibration::calibrationBusy, this,
            &ConfigAttitudeWidget::onCalibrationBusy);

    m_ui->sixPointStart->setEnabled(true);
    m_ui->yawOrientationStart->setEnabled(true);
    m_ui->levelingStart->setEnabled(true);
    m_ui->levelingAndBiasStart->setEnabled(true);

    refreshWidgetsValues();
}

ConfigAttitudeWidget::~ConfigAttitudeWidget()
{
    // Do nothing
}

void ConfigAttitudeWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    m_ui->sixPointHelp->fitInView(paperplane, Qt::KeepAspectRatio);
}

void ConfigAttitudeWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    m_ui->sixPointHelp->fitInView(paperplane, Qt::KeepAspectRatio);
}

/**
  Rotate the paper plane
  */
void ConfigAttitudeWidget::displayPlane(int position)
{
    QString displayElement;
    switch (position) {
    case 1:
        displayElement = "plane-horizontal";
        break;
    case 2:
        displayElement = "plane-left";
        break;
    case 3:
        displayElement = "plane-flip";
        break;
    case 4:
        displayElement = "plane-right";
        break;
    case 5:
        displayElement = "plane-up";
        break;
    case 6:
        displayElement = "plane-down";
        break;
    default:
        return;
    }

    paperplane->setElementId(displayElement);
    m_ui->sixPointHelp->setSceneRect(paperplane->boundingRect());
    m_ui->sixPointHelp->fitInView(paperplane, Qt::KeepAspectRatio);
}

/********** UI Functions *************/

/**
  * Called by the ConfigTaskWidget parent when variances are updated
  * to update the UI
  */
void ConfigAttitudeWidget::refreshWidgetsValues(UAVObject *)
{
    ConfigTaskWidget::refreshWidgetsValues();
}

/**
 * @brief ConfigAttitudeWidget::setUpdated Slot that receives signals indicating the UI is updated
 */
void ConfigAttitudeWidget::do_SetDirty()
{
    setDirty(true);
}

void ConfigAttitudeWidget::configureSixPoint()
{
    if (!m_ui->cbCalibrateAccels->isChecked() && !m_ui->cbCalibrateMags->isChecked()) {
        QMessageBox::information(this, "No sensors chosen", "At least one of the sensors must be "
                                                            "chosen. \n\nResetting six-point "
                                                            "sensor calibration selection.");
        m_ui->cbCalibrateAccels->setChecked(true && board_has_accelerometer);
        m_ui->cbCalibrateMags->setChecked(true && board_has_magnetometer);
    }
    calibration.initialize(m_ui->cbCalibrateAccels->isChecked(),
                           m_ui->cbCalibrateMags->isChecked());
}

void ConfigAttitudeWidget::onCalibrationBusy(bool busy)
{
    // Show the UI is blocking
    if (busy)
        QApplication::setOverrideCursor(Qt::WaitCursor);
    else
        QApplication::restoreOverrideCursor();
}

/**
  @}
  @}
  */
