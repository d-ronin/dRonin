/**
 ******************************************************************************
 *
 * @file       configgadgetwidget.cpp
 * @author     E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "configgadgetwidget.h"

#include "configvehicletypewidget.h"
#include "configinputwidget.h"
#include "configoutputwidget.h"
#include "configstabilizationwidget.h"
#include "configosdwidget.h"
#include "configradiowidget.h"
#include "configmodulewidget.h"
#include "configautotunewidget.h"
#include "configtxpidwidget.h"
#include "configattitudewidget.h"
#include "defaulthwsettingswidget.h"
#include "uavobjectutil/uavobjectutilmanager.h"

#include <QDebug>
#include <QMessageBox>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>

ConfigGadgetWidget::ConfigGadgetWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    ftw = new MyTabbedStackWidget(this, true, true);
    ftw->setIconSize(64);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(ftw);
    setLayout(layout);

    help = nullptr;
    chunk = 0;
    lastTabIndex = ConfigGadgetWidget::hardware;

    QTimer::singleShot(500, this, &ConfigGadgetWidget::deferredLoader);
}

void ConfigGadgetWidget::deferredLoader()
{
    QIcon *icon;
    QWidget *qwd;

    switch (chunk) {
    case 0:
        // *********************
        icon = new QIcon();
        icon->addFile(":/configgadget/images/hardware_normal.png", QSize(), QIcon::Normal,
                      QIcon::Off);
        icon->addFile(":/configgadget/images/hardware_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new QLabel(tr("<p>No recognized board detected. Hardware tab will refresh once a "
                            "known board is detected.</p>"),
                         this);
        qobject_cast<QLabel *>(qwd)->setWordWrap(true);
        ftw->insertTab(ConfigGadgetWidget::hardware, qwd, *icon, QString(tr("Hardware")));
        break;

    case 1:
        ftw->setCurrentIndex(ConfigGadgetWidget::hardware);
        break;

    case 2:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/vehicle_normal.png", QSize(), QIcon::Normal,
                      QIcon::Off);
        icon->addFile(":/configgadget/images/vehicle_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigVehicleTypeWidget(this);
        ftw->insertTab(ConfigGadgetWidget::aircraft, qwd, *icon, QString("Vehicle"));
        break;

    case 3:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/input_normal.png", QSize(), QIcon::Normal, QIcon::Off);
        icon->addFile(":/configgadget/images/input_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigInputWidget(this);
        ftw->insertTab(ConfigGadgetWidget::input, qwd, *icon, QString("Input"));
        break;

    case 4:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/output_normal.png", QSize(), QIcon::Normal,
                      QIcon::Off);
        icon->addFile(":/configgadget/images/output_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigOutputWidget(this);
        ftw->insertTab(ConfigGadgetWidget::output, qwd, *icon, QString("Output"));
        break;

    case 5:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/ins_normal.png", QSize(), QIcon::Normal, QIcon::Off);
        icon->addFile(":/configgadget/images/ins_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigAttitudeWidget(this);
        ftw->insertTab(ConfigGadgetWidget::sensors, qwd, *icon, QString("Attitude"));
        break;

    case 6:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/stabilization_normal.png", QSize(), QIcon::Normal,
                      QIcon::Off);
        icon->addFile(":/configgadget/images/stabilization_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigStabilizationWidget(this);
        ftw->insertTab(ConfigGadgetWidget::stabilization, qwd, *icon, QString("Stabilization"));
        break;

    case 7:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/modules_normal.png", QSize(), QIcon::Normal,
                      QIcon::Off);
        icon->addFile(":/configgadget/images/modules_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigModuleWidget(this);
        ftw->insertTab(ConfigGadgetWidget::modules, qwd, *icon, QString("Modules"));
        break;

    case 8:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/autotune_normal.png", QSize(), QIcon::Normal,
                      QIcon::Off);
        icon->addFile(":/configgadget/images/autotune_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigAutotuneWidget(this);
        ftw->insertTab(ConfigGadgetWidget::autotune, qwd, *icon, QString("Autotune"));
        break;

    case 9:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/txpid_normal.png", QSize(), QIcon::Normal, QIcon::Off);
        icon->addFile(":/configgadget/images/txpid_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigTxPIDWidget(this);
        ftw->insertTab(ConfigGadgetWidget::txpid, qwd, *icon, QString("TxPID"));
        break;

    case 10:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/osd_normal.png", QSize(), QIcon::Normal, QIcon::Off);
        icon->addFile(":/configgadget/images/osd_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigOsdWidget(this);
        ftw->insertTab(ConfigGadgetWidget::osd, qwd, *icon, QString("OSD"));
        // Hide OSD if not applicable, else show
        ftw->setHidden(ConfigGadgetWidget::osd, true);
        break;

    case 11:
        icon = new QIcon();
        icon->addFile(":/configgadget/images/radio_normal.png", QSize(), QIcon::Normal, QIcon::Off);
        icon->addFile(":/configgadget/images/radio_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        qwd = new ConfigRadioWidget(this);
        ftw->insertTab(ConfigGadgetWidget::radio, qwd, *icon, QString("Radio"));
        // Hide Radio if not applicable, else show
        ftw->setHidden(ConfigGadgetWidget::radio, true);
        break;

    case 12:
    default:
        // *********************
        // Listen to autopilot connection events

        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        TelemetryManager *telMngr = pm->getObject<TelemetryManager>();
        connect(telMngr, &TelemetryManager::connected, this,
                &ConfigGadgetWidget::onAutopilotConnect);
        connect(telMngr, &TelemetryManager::disconnected, this,
                &ConfigGadgetWidget::onAutopilotDisconnect);

        // And check whether by any chance we are not already connected
        if (telMngr->isConnected()) {
            onAutopilotConnect();
        }

        connect(ftw, &MyTabbedStackWidget::currentAboutToShow, this,
                &ConfigGadgetWidget::tabAboutToChange); //,Qt::BlockingQueuedConnection);
        return;
    }

    chunk++;

    QTimer::singleShot(0, this, &ConfigGadgetWidget::deferredLoader);
}

void ConfigGadgetWidget::paintEvent(QPaintEvent *event)
{
    (void)event;

    if (chunk < 12) {
        // jumpstart loading.
        deferredLoader();
        deferredLoader();
    }
}

ConfigGadgetWidget::~ConfigGadgetWidget()
{
    // TODO: properly delete all the tabs in ftw before exiting
}

void ConfigGadgetWidget::startInputWizard()
{
    ftw->setCurrentIndex(ConfigGadgetWidget::input);
    ConfigInputWidget *inputWidget =
        dynamic_cast<ConfigInputWidget *>(ftw->getWidget(ConfigGadgetWidget::input));
    Q_ASSERT(inputWidget);
    inputWidget->startInputWizard();
}

void ConfigGadgetWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void ConfigGadgetWidget::onAutopilotDisconnect()
{
    lastTabIndex = ftw->currentIndex();

    ftw->setCurrentIndex(ConfigGadgetWidget::hardware);
    ftw->removeTab(ConfigGadgetWidget::hardware);

    QIcon *icon = new QIcon();
    icon->addFile(":/configgadget/images/hardware_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/hardware_selected.png", QSize(), QIcon::Selected,
                  QIcon::Off);
    QLabel *qwd = new QLabel(tr("<p>No recognized board detected. Hardware tab will refresh once a "
                                "known board is detected.</p>"),
                             this);
    qwd->setWordWrap(true);
    ftw->insertTab(ConfigGadgetWidget::hardware, qwd, *icon, QString("Hardware"));
    ftw->setCurrentIndex(ConfigGadgetWidget::hardware);

    emit autopilotDisconnected();
}

void ConfigGadgetWidget::onAutopilotConnect()
{
    QIcon *icon;
    QWidget *qwd;

    bool hasOSD = false;
    bool hasRadio = false;

    int index = ftw->currentIndex();

    if (index != ConfigGadgetWidget::hardware) {
        lastTabIndex = index;
    }

    // First of all, check what Board type we are talking to, and
    // if necessary, remove/add tabs in the config gadget:
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr = pm->getObject<UAVObjectUtilManager>();
    if (utilMngr) {
        ftw->setCurrentIndex(ConfigGadgetWidget::hardware);
        ftw->removeTab(ConfigGadgetWidget::hardware);

        // If the board provides a custom configuration widget then use it,
        // otherwise use the default which populates the fields from the
        // hardware UAVO
        bool valid_board = false;
        Core::IBoardType *board = utilMngr->getBoardType();
        if (board) {
            qwd = board->getBoardConfiguration();
            if (qwd) {
                valid_board = true;
            } else {
                UAVObject *settingsObj =
                    utilMngr->getObjectManager()->getObject(board->getHwUAVO());
                if (settingsObj) {
                    qwd = new DefaultHwSettingsWidget(settingsObj, this);
                    valid_board = true;
                }
            }

            if (board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_OSD)) {
                hasOSD = true;
            } else if (lastTabIndex == ConfigGadgetWidget::osd) {
                lastTabIndex = ConfigGadgetWidget::hardware;
            }

            if (board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_RADIO)) {
                hasRadio = true;
            } else if (lastTabIndex == ConfigGadgetWidget::radio) {
                lastTabIndex = ConfigGadgetWidget::hardware;
            }
        }

        if (!valid_board) {
            QLabel *txt = new QLabel(tr("<p>Board detected, but of unknown type. This could be "
                                        "because either your GCS or firmware is out of date.</p>"),
                                     this);
            txt->setWordWrap(true);
            qwd = txt;
        }

        icon = new QIcon();
        icon->addFile(":/configgadget/images/hardware_normal.png", QSize(), QIcon::Normal,
                      QIcon::Off);
        icon->addFile(":/configgadget/images/hardware_selected.png", QSize(), QIcon::Selected,
                      QIcon::Off);
        ftw->insertTab(ConfigGadgetWidget::hardware, qwd, *icon, QString(tr("Hardware")));
        ftw->setCurrentIndex(ConfigGadgetWidget::hardware);
    }

    //! Remove and recreate the attitude widget to refresh board capabilities
    ftw->removeTab(ConfigGadgetWidget::sensors);
    icon = new QIcon();
    icon->addFile(":/configgadget/images/ins_normal.png", QSize(), QIcon::Normal, QIcon::Off);
    icon->addFile(":/configgadget/images/ins_selected.png", QSize(), QIcon::Selected, QIcon::Off);
    qwd = new ConfigAttitudeWidget(this);
    ftw->insertTab(ConfigGadgetWidget::sensors, qwd, *icon, QString("Attitude"));

    // Hide OSD if not applicable, else show
    ftw->setHidden(ConfigGadgetWidget::osd, !hasOSD);
    ftw->setHidden(ConfigGadgetWidget::radio, !hasRadio);

    ftw->setCurrentIndex(lastTabIndex);

    emit autopilotConnected();
}

void ConfigGadgetWidget::changeTab(int i)
{
    ftw->setCurrentIndex(i);
}

void ConfigGadgetWidget::tabAboutToChange(int i, bool *proceed)
{
    Q_UNUSED(i);
    *proceed = true;
    ConfigTaskWidget *wid = qobject_cast<ConfigTaskWidget *>(ftw->currentWidget());
    if (!wid) {
        return;
    }

    wid->tabSwitchingAway();

    // Check if widget is dirty (i.e. has unsaved changes)
    if (wid->isDirty() && wid->isAutopilotConnected()) {
        int ans = QMessageBox::warning(this, tr("Unsaved changes"),
                                       tr("The tab you are leaving has unsaved changes,"
                                          "if you proceed they may be lost."
                                          "Do you still want to proceed?"),
                                       QMessageBox::Yes, QMessageBox::No);
        if (ans == QMessageBox::No) {
            *proceed = false;
        } else {
            wid->setDirty(false);
        }
    }
}
