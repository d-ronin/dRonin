/**
 ******************************************************************************
 *
 * @file       configinputwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Servo input/output configuration panel for the config gadget
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "configinputwidget.h"

#include "uavtalk/telemetrymanager.h"

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

#include <extensionsystem/pluginmanager.h>
#include <uavobjectutil/uavobjectutilmanager.h>

#include "actuatorcommand.h"
#include "actuatorsettings.h"
#include "stabilizationsettings.h"
#include "systemsettings.h"

// The fraction of range to place the neutral position in
#define THROTTLE_NEUTRAL_FRACTION 0.02
#define ARMING_NEUTRAL_FRACTION 0.62

#define ACCESS_MIN_MOVE -3
#define ACCESS_MAX_MOVE 3
#define STICK_MIN_MOVE -8
#define STICK_MAX_MOVE 8

#define MIN_SANE_CHANNEL_VALUE 15
#define MAX_SANE_CHANNEL_VALUE 32768
#define MIN_SANE_RANGE 125

// Should exceed "MIN_MEANINGFUL_RANGE" in transmitter_control.c (40)
// Needed to avoid failsafe during input wizard which confounds the stick
// movements.   Also, life the universe and everything.
#define INITIAL_OFFSET 42

const QVector<ConfigInputWidget::ArmingMethod> ConfigInputWidget::armingMethods = {
    { ConfigInputWidget::ARM_ALWAYS_DISARMED, "always disarmed", "always disarmed",
      "always disarmed", "Always Disarmed", false, false, true },
    { ConfigInputWidget::ARM_SWITCH, "switch", "a switch", "a switch", "Switch", true, false,
      false },
    { ConfigInputWidget::ARM_ROLL_LEFT, "roll left", "roll left", "roll right",
      "Roll Left+Throttle", false, true, false },
    { ConfigInputWidget::ARM_ROLL_RIGHT, "roll right", "roll right", "roll left",
      "Roll Right+Throttle", false, true, false },
    { ConfigInputWidget::ARM_YAW_LEFT, "yaw left", "yaw left", "yaw right", "Yaw Left+Throttle",
      false, true, false },
    { ConfigInputWidget::ARM_YAW_RIGHT, "yaw right", "yaw right", "yaw left", "Yaw Right+Throttle",
      false, true, false },
    { ConfigInputWidget::ARM_CORNERS, "corners", "roll left and yaw right",
      "roll right and yaw left", "Corners+Throttle", false, true, false },
};

ConfigInputWidget::ConfigInputWidget(QWidget *parent)
    : ConfigTaskWidget(parent)
    , wizardStep(wizardNone)
    , transmitterType(heli)
    , loop(NULL)
    , skipflag(false)
    , cbArmingOption(nullptr)
    , lastArmingMethod(ARM_INVALID)
    , armingConfigUpdating(false)
{
    manualCommandObj = ManualControlCommand::GetInstance(getObjectManager());
    manualSettingsObj = ManualControlSettings::GetInstance(getObjectManager());
    flightStatusObj = FlightStatus::GetInstance(getObjectManager());
    receiverActivityObj = ReceiverActivity::GetInstance(getObjectManager());
    m_config = new Ui_InputWidget();
    m_config->setupUi(this);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pm);
    // Get telemetry manager and make sure it is valid
    telMngr = pm->getObject<TelemetryManager>();
    Q_ASSERT(telMngr);

    // Generate the rows of buttons in the input channel form GUI
    int index = 0;
    foreach (QString name, manualSettingsObj->getField("ChannelNumber")->getElementNames()) {
        Q_ASSERT(index < static_cast<int>(ManualControlSettings::CHANNELGROUPS_NUMELEM));
        inputChannelForm *inpForm = new inputChannelForm(this, index == 0, true);
        m_config->channelSettings->layout()->addWidget(inpForm); // Add the row to the UI
        inpForm->setName(name);
        addUAVObjectToWidgetRelation("ManualControlSettings", "ChannelGroups",
                                     inpForm->ui->channelGroup, index);
        addUAVObjectToWidgetRelation("ManualControlSettings", "ChannelNumber",
                                     inpForm->ui->channelNumber, index);
        addUAVObjectToWidgetRelation("ManualControlSettings", "ChannelMin", inpForm->ui->channelMin,
                                     index);
        addUAVObjectToWidgetRelation("ManualControlSettings", "ChannelMax", inpForm->ui->channelMax,
                                     index);
        addUAVObjectToWidgetRelation("ManualControlSettings", "ChannelNeutral",
                                     inpForm->ui->channelNeutral, index);

        // we warn the user if collective is assigned on multirotors (it's a rare config)
        if (index == ManualControlSettings::CHANNELGROUPS_COLLECTIVE) {
            connect(inpForm, &inputChannelForm::assignmentChanged,
                    this, &ConfigInputWidget::checkCollective);
        }

        int index2 = manualCommandObj->getField("Channel")->getElementNames().indexOf(name);
        if (index2 >= 0) {
            addUAVObjectToWidgetRelation("ManualControlCommand", "Channel",
                                         inpForm->ui->channelCurrent, index2);
            addUAVObjectToWidgetRelation("ManualControlCommand", "Channel",
                                         inpForm->sbChannelCurrent, index2);
        }
        ++index;
    }

    // RSSI
    inputChannelForm *inpForm =
        new inputChannelForm(this, false, false, inputChannelForm::CHANNELFUNC_RSSI);
    m_config->channelSettings->layout()->addWidget(inpForm);
    QString name = "RSSI";
    inpForm->setName(name);
    addUAVObjectToWidgetRelation("ManualControlSettings", "RssiType", inpForm->ui->channelGroup, 0);
    addUAVObjectToWidgetRelation("ManualControlSettings", "RssiChannelNumber",
                                 inpForm->ui->channelNumber, 0);
    addUAVObjectToWidgetRelation("ManualControlSettings", "RssiMin", inpForm->ui->channelMin, 0);
    addUAVObjectToWidgetRelation("ManualControlSettings", "RssiMax", inpForm->ui->channelMax, 0);
    addUAVObjectToWidgetRelation("ManualControlCommand", "RawRssi", inpForm->ui->channelCurrent, 0);

    connect(m_config->configurationWizard, &QAbstractButton::clicked, this,
            &ConfigInputWidget::goToWizard);
    connect(m_config->runCalibration, &QAbstractButton::toggled, this,
            &ConfigInputWidget::simpleCalibration);

    connect(m_config->wzNext, &QAbstractButton::clicked, this, &ConfigInputWidget::wzNext);
    connect(m_config->wzCancel, &QAbstractButton::clicked, this, &ConfigInputWidget::wzCancel);
    connect(m_config->wzBack, &QAbstractButton::clicked, this, &ConfigInputWidget::wzBack);

    m_config->stackedWidget->setCurrentIndex(0);

    // connect this before the widgets are populated to ensure it always fires
    connect(ManualControlCommand::GetInstance(getObjectManager()), &UAVObject::objectUpdated, this,
            &ConfigInputWidget::moveFMSlider);
    connect(ManualControlSettings::GetInstance(getObjectManager()), &UAVObject::objectUpdated, this,
            &ConfigInputWidget::updatePositionSlider);

    // check reprojection settings
    const QStringList axes({ "Roll", "Pitch", "Yaw", "Rep" });
    for (int i = 1; i <= 3; i++) {
        foreach (const QString &axis, axes) {
            QComboBox *child =
                this->findChild<QComboBox *>(QString("fmsSsPos%1%2").arg(i).arg(axis));
            if (child)
                connect(child, &QComboBox::currentTextChanged, this,
                        &ConfigInputWidget::checkReprojection);
        }
    }

    // check things that affect arming config
    fillArmingComboBox();
    ActuatorSettings *actuatorSettings =
        qobject_cast<ActuatorSettings *>(getObjectManager()->getObject(ActuatorSettings::NAME));
    connect(actuatorSettings, &UAVObject::objectUpdated, this,
            &ConfigInputWidget::checkArmingConfig);
    connect(m_config->cbArmMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigInputWidget::checkArmingConfig);
    connect(m_config->cbThrottleCheck, &QCheckBox::stateChanged, this,
            &ConfigInputWidget::checkArmingConfig);
    connect(m_config->sbThrottleTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ConfigInputWidget::checkArmingConfig);
    connect(m_config->sbFailsafeTimeout, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ConfigInputWidget::checkArmingConfig);
    connect(m_config->cbFailsafeTimeout, &QCheckBox::stateChanged, this,
            &ConfigInputWidget::timeoutCheckboxChanged);
    connect(m_config->cbThrottleTimeout, &QCheckBox::stateChanged, this,
            &ConfigInputWidget::timeoutCheckboxChanged);
    connect(ManualControlSettings::GetInstance(getObjectManager()), &UAVObject::objectUpdated, this,
            &ConfigInputWidget::updateArmingConfig);

    // create a hidden widget so uavo->widget relation can take care of some of the work
    cbArmingOption = new QComboBox(this);
    cbArmingOption->setVisible(false);
    cbArmingOption->setProperty("objrelation",
                                QStringList{ "objname:ManualControlSettings", "fieldname:Arming" });
    m_config->gbArming->layout()->addWidget(cbArmingOption);

    enableControls(false);

    autoLoadWidgets();

    populateWidgets();
    refreshWidgetsValues();

    m_config->graphicsView->setScene(new QGraphicsScene(this));
    m_config->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_renderer = new QSvgRenderer();
    QGraphicsScene *l_scene = m_config->graphicsView->scene();
    if (QFile::exists(":/configgadget/images/TX2.svg")
        && m_renderer->load(QString(":/configgadget/images/TX2.svg")) && m_renderer->isValid()) {
        l_scene->clear(); // Deletes all items contained in the scene as well.

        m_txBackground = new QGraphicsSvgItem();
        // All other items will be clipped to the shape of the background
        m_txBackground->setFlags(QGraphicsItem::ItemClipsChildrenToShape
                                 | QGraphicsItem::ItemClipsToShape);
        m_txBackground->setSharedRenderer(m_renderer);
        m_txBackground->setElementId("background");
        l_scene->addItem(m_txBackground);

        m_txMainBody = new QGraphicsSvgItem();
        m_txMainBody->setParentItem(m_txBackground);
        m_txMainBody->setSharedRenderer(m_renderer);
        m_txMainBody->setElementId("body");

        m_txLeftStick = new QGraphicsSvgItem();
        m_txLeftStick->setParentItem(m_txBackground);
        m_txLeftStick->setSharedRenderer(m_renderer);
        m_txLeftStick->setElementId("ljoy");

        m_txRightStick = new QGraphicsSvgItem();
        m_txRightStick->setParentItem(m_txBackground);
        m_txRightStick->setSharedRenderer(m_renderer);
        m_txRightStick->setElementId("rjoy");

        m_txAccess0 = new QGraphicsSvgItem();
        m_txAccess0->setParentItem(m_txBackground);
        m_txAccess0->setSharedRenderer(m_renderer);
        m_txAccess0->setElementId("access0");

        m_txAccess1 = new QGraphicsSvgItem();
        m_txAccess1->setParentItem(m_txBackground);
        m_txAccess1->setSharedRenderer(m_renderer);
        m_txAccess1->setElementId("access1");

        m_txAccess2 = new QGraphicsSvgItem();
        m_txAccess2->setParentItem(m_txBackground);
        m_txAccess2->setSharedRenderer(m_renderer);
        m_txAccess2->setElementId("access2");

        m_txFlightMode = new QGraphicsSvgItem();
        m_txFlightMode->setParentItem(m_txBackground);
        m_txFlightMode->setSharedRenderer(m_renderer);
        m_txFlightMode->setElementId("flightModeCenter");
        m_txFlightMode->setZValue(-10);

        m_txArming = new QGraphicsSvgItem();
        m_txArming->setParentItem(m_txBackground);
        m_txArming->setSharedRenderer(m_renderer);
        m_txArming->setElementId("armedswitchleft");
        m_txArming->setZValue(-10);

        m_txArrows = new QGraphicsSvgItem();
        m_txArrows->setParentItem(m_txBackground);
        m_txArrows->setSharedRenderer(m_renderer);
        m_txArrows->setElementId("arrows");
        m_txArrows->setVisible(false);

        QRectF orig = m_renderer->boundsOnElement("ljoy");
        QMatrix Matrix = m_renderer->matrixForElement("ljoy");
        orig = Matrix.mapRect(orig);
        m_txLeftStickOrig.translate(orig.x(), orig.y());
        m_txLeftStick->setTransform(m_txLeftStickOrig, false);

        orig = m_renderer->boundsOnElement("arrows");
        Matrix = m_renderer->matrixForElement("arrows");
        orig = Matrix.mapRect(orig);
        m_txArrowsOrig.translate(orig.x(), orig.y());
        m_txArrows->setTransform(m_txArrowsOrig, false);

        orig = m_renderer->boundsOnElement("body");
        Matrix = m_renderer->matrixForElement("body");
        orig = Matrix.mapRect(orig);
        m_txMainBodyOrig.translate(orig.x(), orig.y());
        m_txMainBody->setTransform(m_txMainBodyOrig, false);

        orig = m_renderer->boundsOnElement("flightModeCenter");
        Matrix = m_renderer->matrixForElement("flightModeCenter");
        orig = Matrix.mapRect(orig);
        m_txFlightModeCOrig.translate(orig.x(), orig.y());
        m_txFlightMode->setTransform(m_txFlightModeCOrig, false);

        orig = m_renderer->boundsOnElement("flightModeLeft");
        Matrix = m_renderer->matrixForElement("flightModeLeft");
        orig = Matrix.mapRect(orig);
        m_txFlightModeLOrig.translate(orig.x(), orig.y());

        orig = m_renderer->boundsOnElement("flightModeRight");
        Matrix = m_renderer->matrixForElement("flightModeRight");
        orig = Matrix.mapRect(orig);
        m_txFlightModeROrig.translate(orig.x(), orig.y());

        orig = m_renderer->boundsOnElement("armedswitchright");
        Matrix = m_renderer->matrixForElement("armedswitchright");
        orig = Matrix.mapRect(orig);
        m_txArmingSwitchOrigR.translate(orig.x(), orig.y());

        orig = m_renderer->boundsOnElement("armedswitchleft");
        Matrix = m_renderer->matrixForElement("armedswitchleft");
        orig = Matrix.mapRect(orig);
        m_txArmingSwitchOrigL.translate(orig.x(), orig.y());

        orig = m_renderer->boundsOnElement("rjoy");
        Matrix = m_renderer->matrixForElement("rjoy");
        orig = Matrix.mapRect(orig);
        m_txRightStickOrig.translate(orig.x(), orig.y());
        m_txRightStick->setTransform(m_txRightStickOrig, false);

        orig = m_renderer->boundsOnElement("access0");
        Matrix = m_renderer->matrixForElement("access0");
        orig = Matrix.mapRect(orig);
        m_txAccess0Orig.translate(orig.x(), orig.y());
        m_txAccess0->setTransform(m_txAccess0Orig, false);

        orig = m_renderer->boundsOnElement("access1");
        Matrix = m_renderer->matrixForElement("access1");
        orig = Matrix.mapRect(orig);
        m_txAccess1Orig.translate(orig.x(), orig.y());
        m_txAccess1->setTransform(m_txAccess1Orig, false);

        orig = m_renderer->boundsOnElement("access2");
        Matrix = m_renderer->matrixForElement("access2");
        orig = Matrix.mapRect(orig);
        m_txAccess2Orig.translate(orig.x(), orig.y());
        m_txAccess2->setTransform(m_txAccess2Orig, true);
    }
    m_config->graphicsView->fitInView(m_txMainBody, Qt::KeepAspectRatio);
    m_config->graphicsView->setStyleSheet("background: transparent");
    animate = new QTimer(this);
    connect(animate, &QTimer::timeout, this, &ConfigInputWidget::moveTxControls);

    heliChannelOrder << ManualControlSettings::CHANNELGROUPS_COLLECTIVE
                     << ManualControlSettings::CHANNELGROUPS_THROTTLE
                     << ManualControlSettings::CHANNELGROUPS_ROLL
                     << ManualControlSettings::CHANNELGROUPS_PITCH
                     << ManualControlSettings::CHANNELGROUPS_YAW
                     << ManualControlSettings::CHANNELGROUPS_FLIGHTMODE
                     << ManualControlSettings::CHANNELGROUPS_ACCESSORY0
                     << ManualControlSettings::CHANNELGROUPS_ACCESSORY1
                     << ManualControlSettings::CHANNELGROUPS_ACCESSORY2
                     << ManualControlSettings::CHANNELGROUPS_ARMING;

    acroChannelOrder << ManualControlSettings::CHANNELGROUPS_THROTTLE
                     << ManualControlSettings::CHANNELGROUPS_ROLL
                     << ManualControlSettings::CHANNELGROUPS_PITCH
                     << ManualControlSettings::CHANNELGROUPS_YAW
                     << ManualControlSettings::CHANNELGROUPS_FLIGHTMODE
                     << ManualControlSettings::CHANNELGROUPS_ACCESSORY0
                     << ManualControlSettings::CHANNELGROUPS_ACCESSORY1
                     << ManualControlSettings::CHANNELGROUPS_ACCESSORY2
                     << ManualControlSettings::CHANNELGROUPS_ARMING;

    // force the initial tab
    m_config->tabWidget->setCurrentIndex(0);
}
void ConfigInputWidget::resetTxControls()
{

    m_txLeftStick->setTransform(m_txLeftStickOrig, false);
    m_txRightStick->setTransform(m_txRightStickOrig, false);
    m_txAccess0->setTransform(m_txAccess0Orig, false);
    m_txAccess1->setTransform(m_txAccess1Orig, false);
    m_txAccess2->setTransform(m_txAccess2Orig, false);
    m_txFlightMode->setElementId("flightModeCenter");
    m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
    m_txArming->setElementId("armedswitchleft");
    m_txArming->setTransform(m_txArmingSwitchOrigL, false);
    m_txArrows->setVisible(false);
}

ConfigInputWidget::~ConfigInputWidget() {}

void ConfigInputWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_config->graphicsView->fitInView(m_txBackground, Qt::KeepAspectRatio);
}

void ConfigInputWidget::goToWizard()
{
    // Monitor for connection loss to reset wizard safely
    connect(telMngr, &TelemetryManager::disconnected, this, &ConfigInputWidget::wzCancel);

    QMessageBox msgBox(this);
    msgBox.setText(tr("Arming Settings will be set to Always Disarmed for your safety."));
    msgBox.setDetailedText(tr("You will have to reconfigure the arming settings manually "
                              "when the wizard is finished. After the last step of the "
                              "wizard you will be taken to the Arming Settings screen."));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    switch (ret) {
    case QMessageBox::Ok:
        // Set correct tab visible before starting wizard.
        if (m_config->tabWidget->currentIndex() != 0) {
            m_config->tabWidget->setCurrentIndex(0);
        }
        wizardSetUpStep(wizardWelcome);
        m_config->graphicsView->fitInView(m_txBackground, Qt::KeepAspectRatio);
        break;
    case QMessageBox::Cancel:
        break;
    default:
        break;
    }
}

void ConfigInputWidget::wzCancel()
{
    disconnect(telMngr, &TelemetryManager::disconnected, this, &ConfigInputWidget::wzCancel);

    dimOtherControls(false);
    manualCommandObj->setMetadata(manualCommandObj->getDefaultMetadata());
    m_config->stackedWidget->setCurrentIndex(0);

    if (wizardStep != wizardNone)
        wizardTearDownStep(wizardStep);
    wizardStep = wizardNone;
    m_config->stackedWidget->setCurrentIndex(0);

    // Load manual settings back from beginning of wizard
    manualSettingsObj->setData(previousManualSettingsData);

    // Load original metadata
    restoreMdata();
}

void ConfigInputWidget::wzNext()
{
    // In identify sticks mode the next button can indicate
    // channel advance
    if (wizardStep != wizardNone && wizardStep != wizardIdentifySticks)
        wizardTearDownStep(wizardStep);

    // State transitions for next button
    switch (wizardStep) {
    case wizardWelcome:
        wizardSetUpStep(wizardChooseMode);
        break;
    case wizardChooseMode:
        wizardSetUpStep(wizardChooseType);
        break;
    case wizardChooseType:
        wizardSetUpStep(wizardIdentifySticks);
        break;
    case wizardIdentifySticks:
        nextChannel();
        if (currentChannelNum == -1) { // Gone through all channels
            wizardTearDownStep(wizardIdentifySticks);
            wizardSetUpStep(wizardIdentifyCenter);
        }
        break;
    case wizardIdentifyCenter:
        wizardSetUpStep(wizardIdentifyLimits);
        break;
    case wizardIdentifyLimits:
        wizardSetUpStep(wizardIdentifyInverted);
        break;
    case wizardIdentifyInverted:
        wizardSetUpStep(wizardVerifyFailsafe);
        break;
    case wizardVerifyFailsafe:
        wizardSetUpStep(wizardFinish);
        break;
    case wizardFinish:
        disconnect(telMngr, &TelemetryManager::disconnected, this, &ConfigInputWidget::wzCancel);

        wizardStep = wizardNone;
        m_config->stackedWidget->setCurrentIndex(0);
        m_config->tabWidget->setCurrentIndex(2);
        break;
    default:
        Q_ASSERT(0);
    }
}

void ConfigInputWidget::wzBack()
{
    if (wizardStep != wizardNone && wizardStep != wizardIdentifySticks)
        wizardTearDownStep(wizardStep);

    // State transitions for next button
    switch (wizardStep) {
    case wizardChooseMode:
        wizardSetUpStep(wizardWelcome);
        break;
    case wizardChooseType:
        wizardSetUpStep(wizardChooseMode);
        break;
    case wizardIdentifySticks:
        prevChannel();
        if (currentChannelNum == -1) {
            wizardTearDownStep(wizardIdentifySticks);
            wizardSetUpStep(wizardChooseType);
        }
        break;
    case wizardIdentifyCenter:
        wizardSetUpStep(wizardIdentifySticks);
        break;
    case wizardIdentifyLimits:
        wizardSetUpStep(wizardIdentifyCenter);
        break;
    case wizardIdentifyInverted:
        wizardSetUpStep(wizardIdentifyLimits);
        break;
    case wizardVerifyFailsafe:
        wizardSetUpStep(wizardIdentifyInverted);
        break;
    case wizardFinish:
        wizardSetUpStep(wizardVerifyFailsafe);
        break;
    default:
        Q_ASSERT(0);
    }
}

void ConfigInputWidget::wizardSetUpStep(enum wizardSteps step)
{
    switch (step) {
    case wizardWelcome:
        foreach (QPointer<QWidget> wd, extraWidgets) {
            if (!wd.isNull())
                delete wd;
        }
        extraWidgets.clear();
        m_config->graphicsView->setVisible(false);
        setTxMovement(nothing);

        // Store the previous settings, although set to always disarmed for safety
        manualSettingsData = manualSettingsObj->getData();
        manualSettingsData.Arming = ManualControlSettings::ARMING_ALWAYSDISARMED;
        previousManualSettingsData = manualSettingsData;

        // Now clear all the previous channel settings
        for (uint i = 0; i < ManualControlSettings::CHANNELNUMBER_NUMELEM; i++) {
            manualSettingsData.ChannelNumber[i] = 0;
            manualSettingsData.ChannelMin[i] = 0;
            manualSettingsData.ChannelNeutral[i] = 0;
            manualSettingsData.ChannelMax[i] = 0;
            manualSettingsData.ChannelGroups[i] = ManualControlSettings::CHANNELGROUPS_NONE;
        }

        manualSettingsObj->setData(manualSettingsData);
        m_config->wzText->setText(
            tr("Welcome to the inputs configuration wizard.\n"
               "Please follow the instructions on the screen and move controls when asked to.\n"
               "Make sure you have already configured your hardware settings on the proper tab and "
               "restarted your board.\n"
               "Note: if you are using RSSI injection to S.Bus, HSUM or PPM, please configure that "
               "on the input screen and save before proceeding with this wizard.\n"));
        m_config->stackedWidget->setCurrentIndex(1);
        m_config->wzBack->setEnabled(false);
        m_config->wzNext->setEnabled(true);
        m_config->bypassFailsafeGroup->setVisible(false);
        break;
    case wizardChooseMode: {
        m_config->graphicsView->setVisible(true);
        m_config->graphicsView->fitInView(m_txBackground, Qt::KeepAspectRatio);
        setTxMovement(nothing);
        m_config->wzText->setText(tr("Please choose your transmitter type.\n"
                                     "Mode 1 means your throttle stick is on the right.\n"
                                     "Mode 2 means your throttle stick is on the left.\n"));
        m_config->wzBack->setEnabled(true);
        QRadioButton *mode1 = new QRadioButton(tr("Mode 1"), this);
        QRadioButton *mode2 = new QRadioButton(tr("Mode 2"), this);
        mode2->setChecked(true);
        extraWidgets.clear();
        extraWidgets.append(mode1);
        extraWidgets.append(mode2);
        m_config->checkBoxesLayout->layout()->addWidget(mode1);
        m_config->checkBoxesLayout->layout()->addWidget(mode2);
    } break;
    case wizardChooseType: {
        m_config->wzText->setText(
            tr("Please choose your transmitter mode.\n"
               "Acro means normal transmitter.\n"
               "Heli means there is a collective pitch and throttle input.\n"
               "If you are using a heli transmitter please engage throttle hold now.\n"));
        m_config->wzBack->setEnabled(true);
        QRadioButton *typeAcro = new QRadioButton(tr("Acro"), this);
        QRadioButton *typeHeli = new QRadioButton(tr("Heli"), this);
        typeAcro->setChecked(true);
        typeHeli->setChecked(false);
        extraWidgets.clear();
        extraWidgets.append(typeAcro);
        extraWidgets.append(typeHeli);
        m_config->checkBoxesLayout->layout()->addWidget(typeAcro);
        m_config->checkBoxesLayout->layout()->addWidget(typeHeli);
        wizardStep = wizardChooseType;
    } break;
    case wizardIdentifySticks:
        usedChannels.clear();
        currentChannelNum = -1;
        nextChannel();
        manualSettingsData = manualSettingsObj->getData();
        connect(receiverActivityObj, &UAVObject::objectUpdated, this,
                &ConfigInputWidget::identifyControls);
        fastMdata();
        m_config->wzNext->setEnabled(false);
        break;
    case wizardIdentifyCenter:
        setTxMovement(centerAll);
        m_config->wzText->setText(
            QString(tr("Please center all controls and press next when ready (if your FlightMode "
                       "switch has only two positions, leave it in either position).")));
        fastMdata();
        break;
    case wizardIdentifyLimits: {
        setTxMovement(nothing);
        m_config->wzText->setText(QString(
            tr("Please move all controls <b>(including switches)</b> to their maximum extents in "
               "all directions.  (You may continue when all controls have moved)")));
        fastMdata();
        manualSettingsData = manualSettingsObj->getData();
        for (quint8 i = 0; i < ManualControlSettings::CHANNELMAX_NUMELEM; ++i) {
            // Preserve the inverted status
            if (manualSettingsData.ChannelMin[i] <= manualSettingsData.ChannelMax[i]) {
                manualSettingsData.ChannelMin[i] =
                    manualSettingsData.ChannelNeutral[i] - INITIAL_OFFSET;
                manualSettingsData.ChannelMax[i] =
                    manualSettingsData.ChannelNeutral[i] + INITIAL_OFFSET;
            } else {
                manualSettingsData.ChannelMin[i] =
                    manualSettingsData.ChannelNeutral[i] + INITIAL_OFFSET;
                manualSettingsData.ChannelMax[i] =
                    manualSettingsData.ChannelNeutral[i] - INITIAL_OFFSET;
            }
        }
        connect(manualCommandObj, &UAVObject::objectUpdated, this,
                &ConfigInputWidget::identifyLimits);
        connect(manualCommandObj, &UAVObject::objectUpdated, this, &ConfigInputWidget::moveSticks);

        // Disable next.  When range on all channels is sufficient, enable it
        m_config->wzNext->setEnabled(false);
    } break;
    case wizardIdentifyInverted:
        dimOtherControls(false);
        setTxMovement(nothing);
        extraWidgets.clear();

        for (int index = 0;
             index < manualSettingsObj->getField("ChannelMax")->getElementNames().length();
             index++) {
            QString name = manualSettingsObj->getField("ChannelMax")->getElementNames().at(index);
            if (!name.contains("Access") && !name.contains("Flight")) {
                QCheckBox *cb = new QCheckBox(name, this);
                // Make sure checked status matches current one
                cb->setChecked(manualSettingsData.ChannelMax[index]
                               < manualSettingsData.ChannelMin[index]);

                extraWidgets.append(cb);
                m_config->checkBoxesLayout->layout()->addWidget(cb);

                connect(cb, &QAbstractButton::toggled, this, &ConfigInputWidget::invertControls);
            }
        }
        connect(manualCommandObj, &UAVObject::objectUpdated, this, &ConfigInputWidget::moveSticks);
        m_config->wzText->setText(
            QString(tr("Please check the picture below and correct all the sticks which show an "
                       "inverted movement, press next when ready.")));
        fastMdata();
        break;
    case wizardVerifyFailsafe:
        restoreMdata(); // make sure other updates do not clobber failsafe
        dimOtherControls(false);
        setTxMovement(nothing);
        extraWidgets.clear();
        flightStatusObj->requestUpdate();
        detectFailsafe();
        failsafeDetection = FS_AWAITING_CONNECTION;
        connect(flightStatusObj, &UAVObject::objectUpdated, this,
                &ConfigInputWidget::detectFailsafe);
        m_config->wzNext->setEnabled(false);
        m_config->graphicsView->setVisible(false);
        m_config->bypassFailsafeGroup->setVisible(true);
        connect(m_config->cbBypassFailsafe, &QAbstractButton::toggled, this,
                &ConfigInputWidget::detectFailsafe);
        break;
    case wizardFinish:
        dimOtherControls(false);
        connect(manualCommandObj, &UAVObject::objectUpdated, this, &ConfigInputWidget::moveSticks);
        m_config->wzText->setText(QString(tr(
            "You have completed this wizard, please check below if the picture mimics your sticks "
            "movement.\n"
            "These new settings aren't saved to the board yet, after pressing next you will go to "
            "the Arming Settings "
            "screen where you can set your desired arming sequence and save the configuration.")));
        fastMdata();

        manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_THROTTLE] =
            manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_THROTTLE]
            + ((manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_THROTTLE]
                - manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_THROTTLE])
               * THROTTLE_NEUTRAL_FRACTION);

        if ((abs(manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_FLIGHTMODE]
                 - manualSettingsData
                       .ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_FLIGHTMODE])
             < 100)
            || (abs(manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_FLIGHTMODE]
                    - manualSettingsData
                          .ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_FLIGHTMODE])
                < 100)) {
            manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_FLIGHTMODE] =
                manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_FLIGHTMODE]
                + (manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_FLIGHTMODE]
                   - manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_FLIGHTMODE])
                    / 2;
        }

        manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_ARMING] =
            manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_ARMING]
            + ((manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_ARMING]
                - manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_ARMING])
               * ARMING_NEUTRAL_FRACTION);

        manualSettingsObj->setData(manualSettingsData);
        break;
    default:
        Q_ASSERT(0);
    }
    wizardStep = step;
}

void ConfigInputWidget::wizardTearDownStep(enum wizardSteps step)
{
    QRadioButton *mode, *type;
    Q_ASSERT(step == wizardStep);
    switch (step) {
    case wizardWelcome:
        break;
    case wizardChooseMode:
        mode = qobject_cast<QRadioButton *>(extraWidgets.at(0));
        if (mode->isChecked())
            transmitterMode = mode1;
        else
            transmitterMode = mode2;
        delete extraWidgets.at(0);
        delete extraWidgets.at(1);
        extraWidgets.clear();
        break;
    case wizardChooseType:
        type = qobject_cast<QRadioButton *>(extraWidgets.at(0));
        if (type->isChecked())
            transmitterType = acro;
        else
            transmitterType = heli;
        delete extraWidgets.at(0);
        delete extraWidgets.at(1);
        extraWidgets.clear();
        break;
    case wizardIdentifySticks:
        disconnect(receiverActivityObj, &UAVObject::objectUpdated, this,
                   &ConfigInputWidget::identifyControls);
        m_config->wzNext->setEnabled(true);
        setTxMovement(nothing);
        break;
    case wizardIdentifyCenter:
        manualCommandData = manualCommandObj->getData();
        manualSettingsData = manualSettingsObj->getData();
        for (unsigned int i = 0; i < ManualControlCommand::CHANNEL_NUMELEM; ++i) {
            manualSettingsData.ChannelNeutral[i] = manualCommandData.Channel[i];
            minSeen[i] = manualCommandData.Channel[i];
            maxSeen[i] = manualCommandData.Channel[i];
        }

        // If user skipped flight mode then force the number of flight modes to 1
        // for valid connection
        if (manualSettingsData.ChannelGroups[ManualControlSettings::CHANNELMIN_FLIGHTMODE]
            == ManualControlSettings::CHANNELGROUPS_NONE) {
            manualSettingsData.FlightModeNumber = 1;
        }

        manualSettingsObj->setData(manualSettingsData);
        setTxMovement(nothing);
        break;
    case wizardIdentifyLimits:
        disconnect(manualCommandObj, &UAVObject::objectUpdated, this,
                   &ConfigInputWidget::identifyLimits);
        disconnect(manualCommandObj, &UAVObject::objectUpdated, this,
                   &ConfigInputWidget::moveSticks);
        manualSettingsObj->setData(manualSettingsData);
        setTxMovement(nothing);
        break;
    case wizardIdentifyInverted:
        dimOtherControls(false);
        foreach (QWidget *wd, extraWidgets) {
            QCheckBox *cb = qobject_cast<QCheckBox *>(wd);
            if (cb) {
                disconnect(cb, &QAbstractButton::toggled, this, &ConfigInputWidget::invertControls);
                delete cb;
            }
        }
        extraWidgets.clear();
        disconnect(manualCommandObj, &UAVObject::objectUpdated, this,
                   &ConfigInputWidget::moveSticks);
        break;
    case wizardVerifyFailsafe:
        dimOtherControls(false);
        extraWidgets.clear();
        disconnect(flightStatusObj, &UAVObject::objectUpdated, this,
                   &ConfigInputWidget::detectFailsafe);
        m_config->graphicsView->setVisible(true);
        m_config->bypassFailsafeGroup->setVisible(false);
        disconnect(m_config->cbBypassFailsafe, &QAbstractButton::toggled, this,
                   &ConfigInputWidget::detectFailsafe);
        break;
    case wizardFinish:
        dimOtherControls(false);
        setTxMovement(nothing);
        disconnect(manualCommandObj, &UAVObject::objectUpdated, this,
                   &ConfigInputWidget::moveSticks);
        restoreMdata();
        break;
    default:
        Q_ASSERT(0);
    }
}

/**
 * @brief ConfigInputWidget::fastMdata Set manual control command to fast updates.
 */
void ConfigInputWidget::fastMdata()
{
    // Check that the metadata hasn't already been saved.
    if (!originalMetaData.empty())
        return;

    // Store original metadata
    UAVObjectUtilManager *utilMngr = getObjectUtilManager();
    originalMetaData = utilMngr->readAllNonSettingsMetadata();

    // Update data rates
    quint16 fastUpdate = 150; // in [ms]

    // Iterate over list of UAVObjects, configuring all dynamic data metadata objects.
    UAVObjectManager *objManager = getObjectManager();
    QVector<QVector<UAVDataObject *>> objList = objManager->getDataObjectsVector();
    foreach (QVector<UAVDataObject *> list, objList) {
        foreach (UAVDataObject *obj, list) {
            if (!obj->isSettings()) {
                UAVObject::Metadata mdata = obj->getMetadata();
                UAVObject::SetFlightAccess(mdata, UAVObject::ACCESS_READWRITE);

                switch (obj->getObjID()) {
                case ReceiverActivity::OBJID:
                case FlightStatus::OBJID:
                    UAVObject::SetFlightTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_ONCHANGE);
                    break;
                case ManualControlCommand::OBJID:
                    UAVObject::SetFlightTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_THROTTLED);
                    mdata.flightTelemetryUpdatePeriod = fastUpdate;
                    break;
                default:
                    break;
                }

                // Set the metadata
                obj->setMetadata(mdata);
            }
        }
    }
}

/**
 * Restore previous update settings for manual control data
 */
void ConfigInputWidget::restoreMdata()
{
    foreach (QString objName, originalMetaData.keys()) {
        UAVObject *obj = getObjectManager()->getObject(objName);
        obj->setMetadata(originalMetaData.value(objName));
    }
    originalMetaData.clear();
}

/**
 * Set the display to indicate which channel the person should move
 */
void ConfigInputWidget::setChannel(int newChan)
{
    if (newChan == ManualControlSettings::CHANNELGROUPS_COLLECTIVE)
        m_config->wzText->setText(QString(
            tr("Please enable the throttle hold mode and move the collective pitch stick.")));
    else if (newChan == ManualControlSettings::CHANNELGROUPS_FLIGHTMODE)
        m_config->wzText->setText(QString(tr("Please toggle the flight mode switch. For switches "
                                             "you may have to repeat this rapidly.")));
    else if (newChan == ManualControlSettings::CHANNELGROUPS_ARMING)
        m_config->wzText->setText(QString(tr(
            "Please toggle the arming switch. For switches you may have to repeat this rapidly.")));
    else if ((transmitterType == heli)
             && (newChan == ManualControlSettings::CHANNELGROUPS_THROTTLE))
        m_config->wzText->setText(
            QString(tr("Please disable throttle hold mode and move the throttle stick.")));
    else
        m_config->wzText->setText(
            QString(tr("Please move each control once at a time according to the instructions and "
                       "picture below.\n\n"
                       "Move the %1 stick"))
                .arg(manualSettingsObj->getField("ChannelGroups")->getElementNames().at(newChan)));

    if (manualSettingsObj->getField("ChannelGroups")
            ->getElementNames()
            .at(newChan)
            .contains("Accessory")
        || manualSettingsObj->getField("ChannelGroups")
               ->getElementNames()
               .at(newChan)
               .contains("FlightMode")
        || manualSettingsObj->getField("ChannelGroups")
               ->getElementNames()
               .at(newChan)
               .contains("Arming")) {
        m_config->wzNext->setEnabled(true);
        m_config->wzText->setText(m_config->wzText->text()
                                  + tr(" or click next to skip this channel."));
    } else
        m_config->wzNext->setEnabled(false);

    setMoveFromCommand(newChan);

    currentChannelNum = newChan;
    channelDetected = false;
}

/**
 * Unfortunately order of channel should be different in different conditions.  Selects
 * next channel based on heli or acro mode
 */
void ConfigInputWidget::nextChannel()
{
    QList<int> order = (transmitterType == heli) ? heliChannelOrder : acroChannelOrder;

    if (currentChannelNum == -1) {
        setChannel(order[0]);
        return;
    }
    for (int i = 0; i < order.length() - 1; i++) {
        if (order[i] == currentChannelNum) {
            setChannel(order[i + 1]);
            return;
        }
    }
    currentChannelNum = -1; // hit end of list
}

/**
 * Unfortunately order of channel should be different in different conditions.  Selects
 * previous channel based on heli or acro mode
 */
void ConfigInputWidget::prevChannel()
{
    QList<int> order = transmitterType == heli ? heliChannelOrder : acroChannelOrder;

    // No previous from unset channel or next state
    if (currentChannelNum == -1)
        return;

    for (int i = 1; i < order.length(); i++) {
        if (order[i] == currentChannelNum) {
            setChannel(order[i - 1]);
            usedChannels.removeLast();
            return;
        }
    }
    currentChannelNum = -1; // hit end of list
}

/**
 * @brief ConfigInputWidget::identifyControls Triggers whenever a new ReceiverActivity UAVO is
 * received.
 * This function checks if the channel has already been identified, and if not assigns it to a new
 * ManualControlSettings channel.
 */
void ConfigInputWidget::identifyControls()
{
    static int debounce = 0;

    receiverActivityData = receiverActivityObj->getData();
    if (receiverActivityData.ActiveChannel == 255)
        return;

    if (channelDetected)
        return;
    else {
        receiverActivityData = receiverActivityObj->getData();
        currentChannel.group = receiverActivityData.ActiveGroup;
        currentChannel.number = receiverActivityData.ActiveChannel;
        if (currentChannel == lastChannel)
            ++debounce;
        lastChannel.group = currentChannel.group;
        lastChannel.number = currentChannel.number;

        // If this channel isn't already allocated, and the debounce passes the threshold, set
        // the input channel as detected.
        if (!usedChannels.contains(lastChannel) && debounce > DEBOUNCE_THRESHOLD_COUNT) {
            channelDetected = true;
            debounce = 0;
            usedChannels.append(lastChannel);
            manualSettingsData = manualSettingsObj->getData();
            manualSettingsData.ChannelGroups[currentChannelNum] = currentChannel.group;
            manualSettingsData.ChannelNumber[currentChannelNum] = currentChannel.number;
            manualSettingsObj->setData(manualSettingsData);
        } else
            return;
    }

    // At this point in the code, the channel has been successfully identified
    m_config->wzText->clear();
    setTxMovement(nothing);

    // Wait to ensure that the user is no longer moving the sticks. Empricially,
    // this delay works well across a broad range of users and transmitters.
    QTimer::singleShot(CHANNEL_IDENTIFICATION_WAIT_TIME_MS, this, &ConfigInputWidget::wzNext);
}

void ConfigInputWidget::identifyLimits()
{
    bool allSane = true;

    manualCommandData = manualCommandObj->getData();

    for (quint8 i = 0; i < ManualControlSettings::CHANNELMAX_NUMELEM; ++i) {
        uint16_t channelVal = manualCommandData.Channel[i];

        // Don't mess up the range based on failsafe, etc.
        if ((channelVal > MIN_SANE_CHANNEL_VALUE) && (channelVal < MAX_SANE_CHANNEL_VALUE)) {
            if (channelVal < minSeen[i]) {
                minSeen[i] = channelVal;
            }

            if (channelVal > maxSeen[i]) {
                maxSeen[i] = channelVal;
            }
        }

        switch (i) {
        case ManualControlSettings::CHANNELNUMBER_THROTTLE:
            // Keep the throttle neutral position near the minimum value so that
            // the stick visualization keeps working consistently (it expects this
            // ratio between + and - range.

            // Preserve channel inversion.  Currently no channels will be
            // inverted at this time but input wizard may detect this in the
            // future.
            if (manualSettingsData.ChannelMin[i] <= manualSettingsData.ChannelMax[i]) {
                manualSettingsData.ChannelNeutral[i] =
                    minSeen[i] + (maxSeen[i] - minSeen[i]) * THROTTLE_NEUTRAL_FRACTION;
            } else {
                manualSettingsData.ChannelNeutral[i] =
                    minSeen[i] + (maxSeen[i] - minSeen[i]) * (1.0f - THROTTLE_NEUTRAL_FRACTION);
            }

            break;

        case ManualControlSettings::CHANNELNUMBER_ARMING:
        case ManualControlSettings::CHANNELGROUPS_FLIGHTMODE:
            // Keep switches near the middle.
            manualSettingsData.ChannelNeutral[i] = (maxSeen[i] + minSeen[i]) / 2;
            break;

        default:
            break;
        }

        if (manualSettingsData.ChannelMin[i] <= manualSettingsData.ChannelMax[i]) {
            // Non inverted channel

            // Always update the throttle based on the low part of the range.
            // Otherwise, if the low part of the range will be "wider" than what
            // we initially created-- either because of movement in minimum
            // observed or the neutral value--- update it.
            //
            // This tolerates cases when channels we recalculate neutral for--
            // switches, throttle-- are not initially centered.
            if ((i == ManualControlSettings::CHANNELNUMBER_THROTTLE)
                || ((manualSettingsData.ChannelNeutral[i] - minSeen[i]) > INITIAL_OFFSET)) {
                manualSettingsData.ChannelMin[i] = minSeen[i];
            }

            if ((maxSeen[i] - manualSettingsData.ChannelNeutral[i]) > INITIAL_OFFSET) {
                manualSettingsData.ChannelMax[i] = maxSeen[i];
            }
        } else {
            // Inverted channel

            if ((i == ManualControlSettings::CHANNELNUMBER_THROTTLE)
                || ((maxSeen[i] - manualSettingsData.ChannelNeutral[i]) > INITIAL_OFFSET)) {
                manualSettingsData.ChannelMin[i] = maxSeen[i];
            }

            if ((manualSettingsData.ChannelNeutral[i] - minSeen[i]) > INITIAL_OFFSET) {
                manualSettingsData.ChannelMax[i] = minSeen[i];
            }
        }

        // If this is a used channel, make sure we get a valid range.
        if (manualSettingsData.ChannelGroups[i] != ManualControlSettings::CHANNELGROUPS_NONE) {
            int diff = maxSeen[i] - minSeen[i];

            if (diff < MIN_SANE_RANGE) {
                allSane = false;
            }
        }
    }

    manualSettingsObj->setData(manualSettingsData);

    if (allSane) {
        m_config->wzText->setText(
            QString(tr("Please move all controls <b>(including switches)</b> to their maximum "
                       "extents in all directions.  You may press next when finished.")));
        m_config->wzNext->setEnabled(true);
    }
}
void ConfigInputWidget::setMoveFromCommand(int command)
{
    if (command == ManualControlSettings::CHANNELNUMBER_ROLL) {
        setTxMovement(moveRightHorizontalStick);
    } else if (command == ManualControlSettings::CHANNELNUMBER_PITCH) {
        if (transmitterMode == mode2)
            setTxMovement(moveRightVerticalStick);
        else
            setTxMovement(moveLeftVerticalStick);
    } else if (command == ManualControlSettings::CHANNELNUMBER_YAW) {
        setTxMovement(moveLeftHorizontalStick);
    } else if (command == ManualControlSettings::CHANNELNUMBER_THROTTLE) {
        if (transmitterMode == mode2)
            setTxMovement(moveLeftVerticalStick);
        else
            setTxMovement(moveRightVerticalStick);
    } else if (command == ManualControlSettings::CHANNELNUMBER_COLLECTIVE) {
        if (transmitterMode == mode2)
            setTxMovement(moveLeftVerticalStick);
        else
            setTxMovement(moveRightVerticalStick);
    } else if (command == ManualControlSettings::CHANNELNUMBER_FLIGHTMODE) {
        setTxMovement(moveFlightMode);
    } else if (command == ManualControlSettings::CHANNELNUMBER_ARMING) {
        setTxMovement(armingSwitch);
    } else if (command == ManualControlSettings::CHANNELNUMBER_ACCESSORY0) {
        setTxMovement(moveAccess0);
    } else if (command == ManualControlSettings::CHANNELNUMBER_ACCESSORY1) {
        setTxMovement(moveAccess1);
    } else if (command == ManualControlSettings::CHANNELNUMBER_ACCESSORY2) {
        setTxMovement(moveAccess2);
    }
}

void ConfigInputWidget::setTxMovement(txMovements movement)
{
    resetTxControls();
    switch (movement) {
    case moveLeftVerticalStick:
        movePos = 0;
        growing = true;
        currentMovement = moveLeftVerticalStick;
        animate->start(100);
        break;
    case moveRightVerticalStick:
        movePos = 0;
        growing = true;
        currentMovement = moveRightVerticalStick;
        animate->start(100);
        break;
    case moveLeftHorizontalStick:
        movePos = 0;
        growing = true;
        currentMovement = moveLeftHorizontalStick;
        animate->start(100);
        break;
    case moveRightHorizontalStick:
        movePos = 0;
        growing = true;
        currentMovement = moveRightHorizontalStick;
        animate->start(100);
        break;
    case moveAccess0:
        movePos = 0;
        growing = true;
        currentMovement = moveAccess0;
        animate->start(100);
        break;
    case moveAccess1:
        movePos = 0;
        growing = true;
        currentMovement = moveAccess1;
        animate->start(100);
        break;
    case moveAccess2:
        movePos = 0;
        growing = true;
        currentMovement = moveAccess2;
        animate->start(100);
        break;
    case moveFlightMode:
        movePos = 0;
        growing = true;
        currentMovement = moveFlightMode;
        animate->start(1000);
        break;
    case armingSwitch:
        movePos = 0;
        growing = true;
        currentMovement = armingSwitch;
        animate->start(1000);
        break;
    case centerAll:
        movePos = 0;
        currentMovement = centerAll;
        animate->start(1000);
        break;
    case moveAll:
        movePos = 0;
        growing = true;
        currentMovement = moveAll;
        animate->start(50);
        break;
    case nothing:
        movePos = 0;
        animate->stop();
        break;
    default:
        break;
    }
}

void ConfigInputWidget::moveTxControls()
{
    QTransform trans;
    QGraphicsItem *item = NULL;
    txMovementType move = vertical;
    int limitMax = 0;
    int limitMin = 0;
    static bool auxFlag = false;
    switch (currentMovement) {
    case moveLeftVerticalStick:
        item = m_txLeftStick;
        trans = m_txLeftStickOrig;
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move = vertical;
        break;
    case moveRightVerticalStick:
        item = m_txRightStick;
        trans = m_txRightStickOrig;
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move = vertical;
        break;
    case moveLeftHorizontalStick:
        item = m_txLeftStick;
        trans = m_txLeftStickOrig;
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move = horizontal;
        break;
    case moveRightHorizontalStick:
        item = m_txRightStick;
        trans = m_txRightStickOrig;
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move = horizontal;
        break;
    case moveAccess0:
        item = m_txAccess0;
        trans = m_txAccess0Orig;
        limitMax = ACCESS_MAX_MOVE;
        limitMin = ACCESS_MIN_MOVE;
        move = horizontal;
        break;
    case moveAccess1:
        item = m_txAccess1;
        trans = m_txAccess1Orig;
        limitMax = ACCESS_MAX_MOVE;
        limitMin = ACCESS_MIN_MOVE;
        move = horizontal;
        break;
    case moveAccess2:
        item = m_txAccess2;
        trans = m_txAccess2Orig;
        limitMax = ACCESS_MAX_MOVE;
        limitMin = ACCESS_MIN_MOVE;
        move = horizontal;
        break;
    case moveFlightMode:
        item = m_txFlightMode;
        move = jump;
        break;
    case armingSwitch:
        item = m_txArming;
        move = jump;
        break;
    case centerAll:
        item = m_txArrows;
        move = jump;
        break;
    case moveAll:
        limitMax = STICK_MAX_MOVE;
        limitMin = STICK_MIN_MOVE;
        move = mix;
        break;
    default:
        break;
    }
    if (move == vertical)
        item->setTransform(trans.translate(0, movePos * 10), false);
    else if (move == horizontal)
        item->setTransform(trans.translate(movePos * 10, 0), false);
    else if (move == jump) {
        if (item == m_txArrows) {
            m_txArrows->setVisible(!m_txArrows->isVisible());
        } else if (item == m_txFlightMode) {
            QGraphicsSvgItem *svg;
            svg = static_cast<QGraphicsSvgItem *>(item);
            if (svg) {
                if (svg->elementId() == "flightModeCenter") {
                    if (growing) {
                        svg->setElementId("flightModeRight");
                        m_txFlightMode->setTransform(m_txFlightModeROrig, false);
                    } else {
                        svg->setElementId("flightModeLeft");
                        m_txFlightMode->setTransform(m_txFlightModeLOrig, false);
                    }
                } else if (svg->elementId() == "flightModeRight") {
                    growing = false;
                    svg->setElementId("flightModeCenter");
                    m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
                } else if (svg->elementId() == "flightModeLeft") {
                    growing = true;
                    svg->setElementId("flightModeCenter");
                    m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
                }
            }
        } else if (item == m_txArming) {
            QGraphicsSvgItem *svg;
            svg = static_cast<QGraphicsSvgItem *>(item);
            if (svg) {
                if (svg->elementId() == "armedswitchleft") {
                    svg->setElementId("armedswitchright");
                    m_txArming->setTransform(m_txArmingSwitchOrigR, false);
                } else {
                    svg->setElementId("armedswitchleft");
                    m_txArming->setTransform(m_txArmingSwitchOrigL, false);
                }
            }
        }
    } else if (move == mix) {
        trans = m_txAccess0Orig;
        m_txAccess0->setTransform(
            trans.translate(movePos * 10 * ACCESS_MAX_MOVE / STICK_MAX_MOVE, 0), false);
        trans = m_txAccess1Orig;
        m_txAccess1->setTransform(
            trans.translate(movePos * 10 * ACCESS_MAX_MOVE / STICK_MAX_MOVE, 0), false);
        trans = m_txAccess2Orig;
        m_txAccess2->setTransform(
            trans.translate(movePos * 10 * ACCESS_MAX_MOVE / STICK_MAX_MOVE, 0), false);

        if (auxFlag) {
            trans = m_txLeftStickOrig;
            m_txLeftStick->setTransform(trans.translate(0, movePos * 10), false);
            trans = m_txRightStickOrig;
            m_txRightStick->setTransform(trans.translate(0, movePos * 10), false);
        } else {
            trans = m_txLeftStickOrig;
            m_txLeftStick->setTransform(trans.translate(movePos * 10, 0), false);
            trans = m_txRightStickOrig;
            m_txRightStick->setTransform(trans.translate(movePos * 10, 0), false);
        }

        if (movePos == 0) {
            m_txFlightMode->setElementId("flightModeCenter");
            m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
        } else if (movePos == ACCESS_MAX_MOVE / 2) {
            m_txFlightMode->setElementId("flightModeRight");
            m_txFlightMode->setTransform(m_txFlightModeROrig, false);
        } else if (movePos == ACCESS_MIN_MOVE / 2) {
            m_txFlightMode->setElementId("flightModeLeft");
            m_txFlightMode->setTransform(m_txFlightModeLOrig, false);
        }
    }
    if (move == horizontal || move == vertical || move == mix) {
        if (movePos == 0 && growing)
            auxFlag = !auxFlag;
        if (growing)
            ++movePos;
        else
            --movePos;
        if (movePos > limitMax) {
            movePos = movePos - 2;
            growing = false;
        }
        if (movePos < limitMin) {
            movePos = movePos + 2;
            growing = true;
        }
    }
}

/**
 * @brief ConfigInputWidget::detectFailsafe
 * Detect that the FlightStatus object indicates a Failsafe condition
 */
void ConfigInputWidget::detectFailsafe()
{
    FlightStatus::DataFields flightStatusData = flightStatusObj->getData();
    switch (failsafeDetection) {
    case FS_AWAITING_CONNECTION:
        if (flightStatusData.ControlSource == FlightStatus::CONTROLSOURCE_TRANSMITTER) {
            m_config->wzText->setText(
                QString(tr("To verify that the failsafe mode on your receiver is safe, please turn "
                           "off your transmitter now.\n")));
            failsafeDetection = FS_AWAITING_FAILSAFE; // Now wait for it to go away
        } else {
            m_config->wzText->setText(QString(tr("Unable to detect transmitter to verify failsafe. "
                                                 "Please ensure it is turned on.\n")));
        }
        break;
    case FS_AWAITING_FAILSAFE:
        if (flightStatusData.ControlSource == FlightStatus::CONTROLSOURCE_FAILSAFE) {
            m_config->wzText->setText(
                QString(tr("Failsafe mode detected. Please turn on your transmitter again.\n")));
            failsafeDetection = FS_AWAITING_RECONNECT; // Now wait for it to go away
        }
        break;
    case FS_AWAITING_RECONNECT:
        if (flightStatusData.ControlSource == FlightStatus::CONTROLSOURCE_TRANSMITTER) {
            m_config->wzText->setText(QString(
                tr("Congratulations. Failsafe detection appears to be working reliably.\n")));
            m_config->wzNext->setEnabled(true);
        }
        break;
    }
    if (m_config->cbBypassFailsafe->checkState()) {
        m_config->wzText->setText(
            QString(tr("You are selecting to bypass failsafe detection. If this is not working, "
                       "then the flight controller is likely to fly away. Please check on the "
                       "forums how to configure this properly.\n")));
        m_config->wzNext->setEnabled(true);
    }
}

void ConfigInputWidget::moveSticks()
{
    QTransform trans;
    manualCommandData = manualCommandObj->getData();

    // 0 for throttle is THROTTLE_NEUTRAL_FRACTION of the total range
    // here we map it from [-1,0,1] -> [0,THROTTLE_NEUTRAL_FRACTION,1]
    double throttlePosition = manualCommandData.Throttle < 0
        ? 0 + THROTTLE_NEUTRAL_FRACTION * (1 - manualCommandData.Throttle)
        : THROTTLE_NEUTRAL_FRACTION + manualCommandData.Throttle * (1 - THROTTLE_NEUTRAL_FRACTION);
    // now map [0,1] -> [-1,1] for consistence with other channels
    throttlePosition = 2 * throttlePosition - 1;
    if (transmitterMode == mode2) {
        trans = m_txLeftStickOrig;
        m_txLeftStick->setTransform(trans.translate(manualCommandData.Yaw * STICK_MAX_MOVE * 10,
                                                    -throttlePosition * STICK_MAX_MOVE * 10),
                                    false);
        trans = m_txRightStickOrig;
        m_txRightStick->setTransform(trans.translate(manualCommandData.Roll * STICK_MAX_MOVE * 10,
                                                     manualCommandData.Pitch * STICK_MAX_MOVE * 10),
                                     false);
    } else {
        trans = m_txRightStickOrig;
        m_txRightStick->setTransform(trans.translate(manualCommandData.Roll * STICK_MAX_MOVE * 10,
                                                     -throttlePosition * STICK_MAX_MOVE * 10),
                                     false);
        trans = m_txLeftStickOrig;
        m_txLeftStick->setTransform(trans.translate(manualCommandData.Yaw * STICK_MAX_MOVE * 10,
                                                    manualCommandData.Pitch * STICK_MAX_MOVE * 10),
                                    false);
    }
    switch (scaleSwitchChannel(ManualControlSettings::CHANNELMIN_FLIGHTMODE,
                               manualSettingsObj->getData().FlightModeNumber)) {
    case 0:
        m_txFlightMode->setElementId("flightModeLeft");
        m_txFlightMode->setTransform(m_txFlightModeLOrig, false);
        break;
    case 1:
        m_txFlightMode->setElementId("flightModeCenter");
        m_txFlightMode->setTransform(m_txFlightModeCOrig, false);
        break;
    case 2:
    default:
        m_txFlightMode->setElementId("flightModeRight");
        m_txFlightMode->setTransform(m_txFlightModeROrig, false);
        break;
    }
    switch (scaleSwitchChannel(ManualControlSettings::CHANNELMIN_ARMING, 2)) {
    case 0:
        m_txArming->setElementId("armedswitchleft");
        m_txArming->setTransform(m_txArmingSwitchOrigL, false);
        break;
    case 1:
        m_txArming->setElementId("armedswitchright");
        m_txArming->setTransform(m_txArmingSwitchOrigR, false);
        break;
    default:
        break;
    }
    m_txAccess0->setTransform(
        QTransform(m_txAccess0Orig)
            .translate(manualCommandData.Accessory[0] * ACCESS_MAX_MOVE * 10, 0),
        false);
    m_txAccess1->setTransform(
        QTransform(m_txAccess1Orig)
            .translate(manualCommandData.Accessory[1] * ACCESS_MAX_MOVE * 10, 0),
        false);
    m_txAccess2->setTransform(
        QTransform(m_txAccess2Orig)
            .translate(manualCommandData.Accessory[2] * ACCESS_MAX_MOVE * 10, 0),
        false);
}

void ConfigInputWidget::dimOtherControls(bool value)
{
    qreal opac;
    if (value)
        opac = 0.1;
    else
        opac = 1;
    m_txAccess0->setOpacity(opac);
    m_txAccess1->setOpacity(opac);
    m_txAccess2->setOpacity(opac);
    m_txFlightMode->setOpacity(opac);
    m_txArming->setOpacity(opac);
}

void ConfigInputWidget::enableControls(bool enable)
{
    m_config->configurationWizard->setEnabled(enable);
    m_config->runCalibration->setEnabled(enable);

    ConfigTaskWidget::enableControls(enable);
}

void ConfigInputWidget::invertControls()
{
    manualSettingsData = manualSettingsObj->getData();
    foreach (QWidget *wd, extraWidgets) {
        QCheckBox *cb = qobject_cast<QCheckBox *>(wd);
        if (cb) {
            int index =
                manualSettingsObj->getField("ChannelNumber")->getElementNames().indexOf(cb->text());
            if ((cb->isChecked()
                 && (manualSettingsData.ChannelMax[index] > manualSettingsData.ChannelMin[index]))
                || (!cb->isChecked()
                    && (manualSettingsData.ChannelMax[index]
                        < manualSettingsData.ChannelMin[index]))) {
                qint16 aux;
                aux = manualSettingsData.ChannelMax[index];
                manualSettingsData.ChannelMax[index] = manualSettingsData.ChannelMin[index];
                manualSettingsData.ChannelMin[index] = aux;
                if (index == ManualControlSettings::CHANNELNEUTRAL_THROTTLE) {
                    // Keep the throttle neutral position near the minimum value so that
                    // the stick visualization keeps working consistently (it expects this
                    // ratio between + and - range.
                    manualSettingsData.ChannelNeutral[index] = manualSettingsData.ChannelMin[index]
                        + (manualSettingsData.ChannelMax[index]
                           - manualSettingsData.ChannelMin[index])
                            * THROTTLE_NEUTRAL_FRACTION;
                }
            }
        }
    }
    manualSettingsObj->setData(manualSettingsData);
}
/**
 * @brief Converts a raw Switch Channel value into a Switch position index
 * @param channelNumber channel number to convert
 * @param switchPositions total switch positions
 * @return Switch position index converted from the raw value
 */
quint8 ConfigInputWidget::scaleSwitchChannel(quint8 channelNumber, quint8 switchPositions)
{
    if (channelNumber > (ManualControlSettings::CHANNELMIN_NUMELEM - 1))
        return 0;
    ManualControlSettings::DataFields manualSettingsDataPriv = manualSettingsObj->getData();
    ManualControlCommand::DataFields manualCommandDataPriv = manualCommandObj->getData();

    float valueScaled;
    int chMin = manualSettingsDataPriv.ChannelMin[channelNumber];
    int chMax = manualSettingsDataPriv.ChannelMax[channelNumber];
    int chNeutral = manualSettingsDataPriv.ChannelNeutral[channelNumber];

    int value = manualCommandDataPriv.Channel[channelNumber];
    if ((chMax > chMin && value >= chNeutral) || (chMin > chMax && value <= chNeutral)) {
        if (chMax != chNeutral)
            valueScaled = (float)(value - chNeutral) / (float)(chMax - chNeutral);
        else
            valueScaled = 0;
    } else {
        if (chMin != chNeutral)
            valueScaled = (float)(value - chNeutral) / (float)(chNeutral - chMin);
        else
            valueScaled = 0;
    }

    if (valueScaled < -1.0)
        valueScaled = -1.0;
    else if (valueScaled > 1.0)
        valueScaled = 1.0;

    // Convert channel value into the switch position in the range [0..N-1]
    // This uses the same optimized computation as flight code to be consistent
    quint8 pos = ((qint16)(valueScaled * 256) + 256) * switchPositions >> 9;
    if (pos >= switchPositions)
        pos = switchPositions - 1;
    return pos;
}

void ConfigInputWidget::moveFMSlider()
{
    m_config->fmsSlider->setValue(
        scaleSwitchChannel(ManualControlSettings::CHANNELMIN_FLIGHTMODE,
                           manualSettingsObj->getData().FlightModeNumber));
}

void ConfigInputWidget::updatePositionSlider()
{
    ManualControlSettings::DataFields manualSettingsDataPriv = manualSettingsObj->getData();

    switch (manualSettingsDataPriv.FlightModeNumber) {
    default:
    case 6:
        m_config->fmsModePos6->setEnabled(true);
        Q_FALLTHROUGH();
    case 5:
        m_config->fmsModePos5->setEnabled(true);
        Q_FALLTHROUGH();
    case 4:
        m_config->fmsModePos4->setEnabled(true);
        Q_FALLTHROUGH();
    case 3:
        m_config->fmsModePos3->setEnabled(true);
        Q_FALLTHROUGH();
    case 2:
        m_config->fmsModePos2->setEnabled(true);
        Q_FALLTHROUGH();
    case 1:
        m_config->fmsModePos1->setEnabled(true);
        Q_FALLTHROUGH();
    case 0:
        break;
    }

    switch (manualSettingsDataPriv.FlightModeNumber) {
    case 0:
        m_config->fmsModePos1->setEnabled(false);
        Q_FALLTHROUGH();
    case 1:
        m_config->fmsModePos2->setEnabled(false);
        Q_FALLTHROUGH();
    case 2:
        m_config->fmsModePos3->setEnabled(false);
        Q_FALLTHROUGH();
    case 3:
        m_config->fmsModePos4->setEnabled(false);
        Q_FALLTHROUGH();
    case 4:
        m_config->fmsModePos5->setEnabled(false);
        Q_FALLTHROUGH();
    case 5:
        m_config->fmsModePos6->setEnabled(false);
        Q_FALLTHROUGH();
    case 6:
    default:
        break;
    }
}

void ConfigInputWidget::updateCalibration()
{
    manualCommandData = manualCommandObj->getData();
    for (quint8 i = 0; i < ManualControlSettings::CHANNELMAX_NUMELEM; ++i) {
        if ((!reverse[i] && manualSettingsData.ChannelMin[i] > manualCommandData.Channel[i])
            || (reverse[i] && manualSettingsData.ChannelMin[i] < manualCommandData.Channel[i]))
            manualSettingsData.ChannelMin[i] = manualCommandData.Channel[i];
        if ((!reverse[i] && manualSettingsData.ChannelMax[i] < manualCommandData.Channel[i])
            || (reverse[i] && manualSettingsData.ChannelMax[i] > manualCommandData.Channel[i]))
            manualSettingsData.ChannelMax[i] = manualCommandData.Channel[i];
        manualSettingsData.ChannelNeutral[i] = manualCommandData.Channel[i];
    }

    manualSettingsObj->setData(manualSettingsData);
    manualSettingsObj->updated();
}

void ConfigInputWidget::simpleCalibration(bool enable)
{
    if (enable) {
        m_config->configurationWizard->setEnabled(false);

        QMessageBox msgBox;
        msgBox.setText(tr("Arming Settings are now set to Always Disarmed for your safety."));
        msgBox.setDetailedText(tr("You will have to reconfigure the arming settings manually when "
                                  "the wizard is finished."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();

        manualCommandData = manualCommandObj->getData();

        manualSettingsData = manualSettingsObj->getData();
        manualSettingsData.Arming = ManualControlSettings::ARMING_ALWAYSDISARMED;
        manualSettingsObj->setData(manualSettingsData);

        for (unsigned int i = 0; i < ManualControlCommand::CHANNEL_NUMELEM; i++) {
            reverse[i] = manualSettingsData.ChannelMax[i] < manualSettingsData.ChannelMin[i];
            manualSettingsData.ChannelMin[i] = manualCommandData.Channel[i];
            manualSettingsData.ChannelNeutral[i] = manualCommandData.Channel[i];
            manualSettingsData.ChannelMax[i] = manualCommandData.Channel[i];
        }

        fastMdata();

        connect(manualCommandObj, &UAVObject::objectUnpacked, this,
                &ConfigInputWidget::updateCalibration);
    } else {
        m_config->configurationWizard->setEnabled(true);

        manualCommandData = manualCommandObj->getData();
        manualSettingsData = manualSettingsObj->getData();

        restoreMdata();

        for (unsigned int i = 0; i < ManualControlCommand::CHANNEL_NUMELEM; i++)
            manualSettingsData.ChannelNeutral[i] = manualCommandData.Channel[i];

        // Force switches to middle
        manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNUMBER_FLIGHTMODE] =
            (manualSettingsData.ChannelMax[ManualControlSettings::CHANNELNUMBER_FLIGHTMODE]
             + manualSettingsData.ChannelMin[ManualControlSettings::CHANNELNUMBER_FLIGHTMODE])
            / 2;

        manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNUMBER_ARMING] =
            (manualSettingsData.ChannelMax[ManualControlSettings::CHANNELNUMBER_ARMING]
             + manualSettingsData.ChannelMin[ManualControlSettings::CHANNELNUMBER_ARMING])
            / 2;

        // Force throttle to be near min
        manualSettingsData.ChannelNeutral[ManualControlSettings::CHANNELNEUTRAL_THROTTLE] =
            manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_THROTTLE]
            + (manualSettingsData.ChannelMax[ManualControlSettings::CHANNELMAX_THROTTLE]
               - manualSettingsData.ChannelMin[ManualControlSettings::CHANNELMIN_THROTTLE])
                * THROTTLE_NEUTRAL_FRACTION;

        manualSettingsObj->setData(manualSettingsData);

        disconnect(manualCommandObj, &UAVObject::objectUnpacked, this,
                   &ConfigInputWidget::updateCalibration);
    }
}

void ConfigInputWidget::clearMessages(QWidget *widget, const QString type)
{
    QLabel *lbl = findChild<QLabel *>("lblMessage" + type);
    if (lbl) {
        widget->layout()->removeWidget(lbl);
        delete lbl;
    }
}

void ConfigInputWidget::addMessage(QWidget *target, const QString type, const QString msg)
{
    QLabel *lbl = findChild<QLabel *>("lblMessage" + type);
    if (!lbl) {
        lbl = new QLabel(this);
        lbl->setObjectName("lblMessage" + type);
        lbl->setWordWrap(true);
        target->layout()->addWidget(lbl);
    }

    lbl->setText(lbl->text() + msg + "\n");
}

void ConfigInputWidget::addWarning(QWidget *target, QWidget *cause, const QString msg)
{
    cause->setProperty("hasWarning", true);
    addMessage(target, "Warning", msg);
}

void ConfigInputWidget::clearWarnings(QWidget *target, QWidget *causesParent)
{
    foreach (QWidget *wid, causesParent->findChildren<QFrame *>())
        wid->setProperty("hasWarning", false);
    clearMessages(target, "Warning");
}

void ConfigInputWidget::checkReprojection()
{
    const QString prefix = sender()->objectName().left(9); // this is a little fragile
    QComboBox *rep = findChild<QComboBox *>(prefix + "Rep");
    Q_ASSERT(rep);

    clearMessages(m_config->gbxFMMessages, prefix);

    QStringList axes;
    QStringList allowed_modes;
    if (rep->currentText() == "CameraAngle") {
        axes << "Roll"
             << "Yaw";
        allowed_modes << "AxisLock"
                      << "Rate"
                      << "WeakLevelling"
                      << "AcroPlus"
                      << "LQG";
    } else if (rep->currentText() == "HeadFree") {
        axes << "Roll"
             << "Pitch";
        allowed_modes << "AxisLock"
                      << "Rate"
                      << "WeakLevelling"
                      << "AcroPlus"
                      << "Attitude"
                      << "Horizon"
                      << "LQG"
                      << "AttitudeLQG";
    } else {
        return;
    }

    QString selected_mode;
    foreach (const QString axis_name, axes) {
        QComboBox *axis = findChild<QComboBox *>(prefix + axis_name);
        Q_ASSERT(axis);

        // make sure only allowed modes are used
        if (!allowed_modes.contains(axis->currentText()))
            addMessage(m_config->gbxFMMessages, prefix,
                       QString("Stabilized%0: When using %1 reprojection, only the following "
                               "stabilization modes are allowed on axes %2: %3")
                           .arg(prefix.right(1))
                           .arg(rep->currentText())
                           .arg(axes.join(", "))
                           .arg(allowed_modes.join(", ")));

        // axes must match
        if (selected_mode.length() < 1)
            selected_mode = axis->currentText();
        if (selected_mode != axis->currentText())
            addMessage(m_config->gbxFMMessages, prefix,
                       QString("Stabilized%0: When using %1 reprojection, stabilization modes must "
                               "match for %2 axes!")
                           .arg(prefix.right(1))
                           .arg(rep->currentText())
                           .arg(axes.join(", ")));
    }
}

const ConfigInputWidget::ArmingMethod
ConfigInputWidget::armingMethodFromArmName(const QString &name)
{
    foreach (const ArmingMethod &method, armingMethods) {
        if (method.armName == name)
            return method;
    }
    Q_ASSERT(false);
    // default to always disarmed
    return armingMethods.at(0);
}

const ConfigInputWidget::ArmingMethod
ConfigInputWidget::armingMethodFromUavoOption(const QString &option)
{
    // ignore stuff after plus to make switch arming cases work
    QString opt = option.split('+', QString::SkipEmptyParts).at(0);
    foreach (const ArmingMethod &method, armingMethods) {
        if (method.uavoOption.split('+', QString::SkipEmptyParts).at(0) == opt)
            return method;
    }
    Q_ASSERT(false);
    // default to always disarmed
    return armingMethods.at(0);
}

void ConfigInputWidget::checkArmingConfig()
{
    if (armingConfigUpdating)
        return;

    // clear existing warnings
    clearWarnings(m_config->gbWarnings, m_config->saArmingSettings);

    SystemSettings *sys =
        qobject_cast<SystemSettings *>(getObjectManager()->getObject(SystemSettings::NAME));
    Q_ASSERT(sys);
    const quint8 vehicle = sys->getAirframeType();

    const ArmingMethod arming = armingMethodFromArmName(m_config->cbArmMethod->currentText());
    m_config->lblDisarmMethod->setText((arming.isStick ? tr("throttle low and ") : "")
                                       + arming.disarmName);

    if (lastArmingMethod != arming.method && lastArmingMethod != ARM_INVALID) {
        if (arming.method == ARM_SWITCH) {
            m_config->cbThrottleCheck->setChecked(true);
        }
    }
    lastArmingMethod = arming.method;

    m_config->frStickArmTime->setVisible(arming.isStick);
    m_config->frSwitchArmTime->setVisible(arming.isSwitch);
    m_config->cbSwitchArmTime->setEnabled(true);
    // TODO: consider throttle check for initial arm with always armed
    m_config->frThrottleCheck->setVisible(arming.isSwitch);

    m_config->frStickDisarmTime->setVisible(arming.isStick);
    m_config->frThrottleTimeout->setVisible(!arming.isFixed);
    m_config->frAutoDisarm->setVisible(!arming.isFixed);

    m_config->gbDisarming->setVisible(!arming.isFixed);

    // check hangtime, recommend switch arming if enabled
    bool hangtime = false;
    StabilizationSettings *stabilizationSettings = qobject_cast<StabilizationSettings *>(
        getObjectManager()->getObject(StabilizationSettings::NAME));
    Q_ASSERT(stabilizationSettings);
    if (stabilizationSettings)
        hangtime = stabilizationSettings->getLowPowerStabilizationMaxTime() > 0;

    if (hangtime) {
        m_config->lblRecommendedArm->setText(tr("Switch"));
        if (arming.method != ARM_ALWAYS_DISARMED && !arming.isSwitch)
            addWarning(m_config->gbWarnings, m_config->frArmMethod,
                       tr("Switch arming is recommended when hangtime is enabled."));
    } else {
        switch (vehicle) {
        case SystemSettings::AIRFRAMETYPE_HELICP:
            m_config->lblRecommendedArm->setText(tr("Switch"));
            break;
        case SystemSettings::AIRFRAMETYPE_HEXA:
        case SystemSettings::AIRFRAMETYPE_HEXACOAX:
        case SystemSettings::AIRFRAMETYPE_HEXAX:
        case SystemSettings::AIRFRAMETYPE_OCTO:
        case SystemSettings::AIRFRAMETYPE_OCTOCOAXP:
        case SystemSettings::AIRFRAMETYPE_OCTOCOAXX:
        case SystemSettings::AIRFRAMETYPE_OCTOV:
        case SystemSettings::AIRFRAMETYPE_QUADP:
        case SystemSettings::AIRFRAMETYPE_QUADX:
        case SystemSettings::AIRFRAMETYPE_TRI:
        case SystemSettings::AIRFRAMETYPE_VTOL:
            m_config->lblRecommendedArm->setText(tr("Switch, roll or yaw"));
            break;
        default:
            m_config->lblRecommendedArm->setText(tr("Any"));
        }
    }

    if (!m_config->frThrottleCheck->isHidden() && !m_config->cbThrottleCheck->isChecked())
        addWarning(m_config->gbWarnings, m_config->frThrottleCheck,
                   tr("Your vehicle can be armed while throttle is not low, be careful!"));

    if (!m_config->frFailsafeTimeout->isHidden() && !m_config->frAutoDisarm->isHidden()
        && !m_config->sbFailsafeTimeout->value())
        addWarning(
            m_config->gbWarnings, m_config->frFailsafeTimeout,
            tr("Your vehicle will remain armed indefinitely when receiver is disconnected!"));

    if (!m_config->frThrottleTimeout->isHidden() && !m_config->sbThrottleTimeout->value())
        addWarning(
            m_config->gbWarnings, m_config->frThrottleTimeout,
            tr("Your vehicle will remain armed indefinitely while throttle is low, be careful!"));

    // re-add stylesheet to force it to be recompiled and applied
    m_config->saArmingSettings->setStyleSheet(m_config->saArmingSettings->styleSheet());

    QString armOption = arming.uavoOption;
    switch (arming.method) {
    case ARM_SWITCH:
        if (m_config->cbThrottleCheck->isChecked())
            armOption += "+Throttle";
        break;
    default:
        break;
    }
    if (cbArmingOption->currentText() != armOption)
        cbArmingOption->setCurrentText(armOption);
}

void ConfigInputWidget::timeoutCheckboxChanged()
{
    m_config->sbThrottleTimeout->setEnabled(m_config->cbThrottleTimeout->isChecked());
    if (!m_config->cbThrottleTimeout->isChecked())
        m_config->sbThrottleTimeout->setValue(0);
    else if (!m_config->sbThrottleTimeout->value())
        resetWidgetToDefault(m_config->sbThrottleTimeout);

    m_config->sbFailsafeTimeout->setEnabled(m_config->cbFailsafeTimeout->isChecked());
    if (!m_config->cbFailsafeTimeout->isChecked())
        m_config->sbFailsafeTimeout->setValue(0);
    else if (!m_config->sbFailsafeTimeout->value())
        resetWidgetToDefault(m_config->sbFailsafeTimeout);

    checkArmingConfig();
}

void ConfigInputWidget::updateArmingConfig(UAVObject *obj)
{
    armingConfigUpdating = true;

    ManualControlSettings *man = qobject_cast<ManualControlSettings *>(obj);
    Q_ASSERT(man);
    if (!man)
        return;

    // don't want to mark the widget dirty by any changes from the UAVO
    bool wasDirty = isDirty();

    m_config->saArmingSettings->setEnabled(man->getIsPresentOnHardware());
    m_config->cbThrottleTimeout->setChecked(man->getArmedTimeout() > 0);
    m_config->cbFailsafeTimeout->setChecked(man->getInvalidRXArmedTimeout() > 0);

    const QString &armingOption = man->getField("Arming")->getValue().toString();
    ArmingMethod arming = armingMethodFromUavoOption(armingOption);
    if (m_config->cbArmMethod->currentText() != arming.armName)
        m_config->cbArmMethod->setCurrentText(arming.armName);
    m_config->cbThrottleCheck->setChecked(armingOption.contains("Throttle"));

    setDirty(wasDirty);

    armingConfigUpdating = false;

    checkArmingConfig();
}

void ConfigInputWidget::fillArmingComboBox()
{
    m_config->cbArmMethod->clear();
    foreach (const ArmingMethod &method, armingMethods)
        m_config->cbArmMethod->addItem(method.armName, method.method);
}

void ConfigInputWidget::checkCollective()
{
    auto inputForm = qobject_cast<inputChannelForm *>(sender());
    if (!inputForm) {
        qWarning() << "invalid sender";
        Q_ASSERT(false);
        return;
    }

    clearWarnings(m_config->gbChannelWarnings, inputForm);

    if (!inputForm->assigned())
        return;

    SystemSettings *sys =
        qobject_cast<SystemSettings *>(getObjectManager()->getObject(SystemSettings::NAME));
    Q_ASSERT(sys);
    const quint8 vehicle = sys->getAirframeType();

    if (vehicle != SystemSettings::AIRFRAMETYPE_HELICP) {
        addWarning(m_config->gbChannelWarnings, inputForm,
                   tr("Collective is normally not used for this vehicle type, are you sure?"));
    }
}
