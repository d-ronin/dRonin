/**
 ******************************************************************************
 * @file       configosdwidget.cpp
 * @brief      Configure the OSD
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
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

#include "configosdwidget.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>

#include "onscreendisplaysettings.h"
#include "onscreendisplaypagesettings.h"

#include "manualcontrolcommand.h"
#include "manualcontrolsettings.h"

#include "ui_osdpage.h"

ConfigOsdWidget::ConfigOsdWidget(QWidget *parent)
    : ConfigTaskWidget(parent)
{
    ui = new Ui::Osd();
    ui->setupUi(this);

    osdSettingsObj = OnScreenDisplaySettings::GetInstance(getObjectManager());
    manualCommandObj = ManualControlCommand::GetInstance(getObjectManager());
    manualSettingsObj = ManualControlSettings::GetInstance(getObjectManager());

    // connect signals to set/get custom OSD text
    connect(ui->applyButton, &QAbstractButton::clicked, this, &ConfigOsdWidget::setCustomText);
    connect(ui->saveButton, &QAbstractButton::clicked, this, &ConfigOsdWidget::setCustomText);
    connect(ui->reloadButton, &QAbstractButton::clicked, this, &ConfigOsdWidget::getCustomText);
    connect(osdSettingsObj, &OnScreenDisplaySettings::CustomText_0Changed, this,
            &ConfigOsdWidget::getCustomText);

    // setup the OSD widgets
    connect(ManualControlCommand::GetInstance(getObjectManager()), &UAVObject::objectUpdated, this,
            &ConfigOsdWidget::movePageSlider);
    connect(OnScreenDisplaySettings::GetInstance(getObjectManager()), &UAVObject::objectUpdated,
            this, &ConfigOsdWidget::updatePositionSlider);

    // Setup OSD pages
    pages[0] = ui->osdPage1;
    pages[1] = ui->osdPage2;
    pages[2] = ui->osdPage3;
    pages[3] = ui->osdPage4;

    ui_pages[0] = new Ui::OsdPage();
    setupOsdPage(ui_pages[0], ui->osdPage1, QStringLiteral("OnScreenDisplayPageSettings"));

    ui_pages[0]->copyButton1->setText("Copy Page 2");
    ui_pages[0]->copyButton2->setText("Copy Page 3");
    ui_pages[0]->copyButton3->setText("Copy Page 4");

    connect(ui_pages[0]->copyButton1, &QAbstractButton::released, this, [this]() {
        copyOsdPage(0, 1);
    });
    connect(ui_pages[0]->copyButton2, &QAbstractButton::released, this, [this]() {
        copyOsdPage(0, 2);
    });
    connect(ui_pages[0]->copyButton3, &QAbstractButton::released, this, [this]() {
        copyOsdPage(0, 3);
    });

    ui_pages[1] = new Ui::OsdPage();
    setupOsdPage(ui_pages[1], ui->osdPage2, QStringLiteral("OnScreenDisplayPageSettings2"));

    ui_pages[1]->copyButton1->setText("Copy Page 1");
    ui_pages[1]->copyButton2->setText("Copy Page 3");
    ui_pages[1]->copyButton3->setText("Copy Page 4");

    connect(ui_pages[1]->copyButton1, &QAbstractButton::released, this, [this]() {
        copyOsdPage(1, 0);
    });
    connect(ui_pages[1]->copyButton2, &QAbstractButton::released, this, [this]() {
        copyOsdPage(1, 2);
    });
    connect(ui_pages[1]->copyButton3, &QAbstractButton::released, this, [this]() {
        copyOsdPage(1, 3);
    });

    ui_pages[2] = new Ui::OsdPage();
    setupOsdPage(ui_pages[2], ui->osdPage3, QStringLiteral("OnScreenDisplayPageSettings3"));

    ui_pages[2]->copyButton1->setText("Copy Page 1");
    ui_pages[2]->copyButton2->setText("Copy Page 2");
    ui_pages[2]->copyButton3->setText("Copy Page 4");

    connect(ui_pages[2]->copyButton1, &QAbstractButton::released, this, [this]() {
        copyOsdPage(2, 0);
    });
    connect(ui_pages[2]->copyButton2, &QAbstractButton::released, this, [this]() {
        copyOsdPage(2, 1);
    });
    connect(ui_pages[2]->copyButton3, &QAbstractButton::released, this, [this]() {
        copyOsdPage(2, 3);
    });

    ui_pages[3] = new Ui::OsdPage();
    setupOsdPage(ui_pages[3], ui->osdPage4, QStringLiteral("OnScreenDisplayPageSettings4"));

    ui_pages[3]->copyButton1->setText("Copy Page 1");
    ui_pages[3]->copyButton2->setText("Copy Page 2");
    ui_pages[3]->copyButton3->setText("Copy Page 3");

    connect(ui_pages[3]->copyButton1, &QAbstractButton::released, this, [this]() {
        copyOsdPage(3, 0);
    });
    connect(ui_pages[3]->copyButton2, &QAbstractButton::released, this, [this]() {
        copyOsdPage(3, 1);
    });
    connect(ui_pages[3]->copyButton3, &QAbstractButton::released, this, [this]() {
        copyOsdPage(3, 2);
    });

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();

    // Refresh widget contents
    refreshWidgetsValues();

    // Prevent mouse wheel from changing values
    disableMouseWheelEvents();
}

ConfigOsdWidget::~ConfigOsdWidget()
{
    for (auto p : ui_pages)
        delete p;
    delete ui;
}

/**
 * @brief Converts a raw Switch Channel value into a Switch position index
 * @param channelNumber channel number to convert
 * @param switchPositions total switch positions
 * @return Switch position index converted from the raw value
 */
quint8 ConfigOsdWidget::scaleSwitchChannel(quint8 channelNumber, quint8 switchPositions)
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

void ConfigOsdWidget::movePageSlider()
{
    OnScreenDisplaySettings::DataFields onScreenDisplaySettingsDataPriv = osdSettingsObj->getData();

    switch (onScreenDisplaySettingsDataPriv.PageSwitch) {
    case OnScreenDisplaySettings::PAGESWITCH_ACCESSORY0:
        ui->osdPageSlider->setValue(scaleSwitchChannel(ManualControlSettings::CHANNELMIN_ACCESSORY0,
                                                       onScreenDisplaySettingsDataPriv.NumPages));
        break;
    case OnScreenDisplaySettings::PAGESWITCH_ACCESSORY1:
        ui->osdPageSlider->setValue(scaleSwitchChannel(ManualControlSettings::CHANNELMIN_ACCESSORY1,
                                                       onScreenDisplaySettingsDataPriv.NumPages));
        break;
    case OnScreenDisplaySettings::PAGESWITCH_ACCESSORY2:
        ui->osdPageSlider->setValue(scaleSwitchChannel(ManualControlSettings::CHANNELMIN_ACCESSORY2,
                                                       onScreenDisplaySettingsDataPriv.NumPages));
        break;
    }
}

void ConfigOsdWidget::updatePositionSlider()
{
    OnScreenDisplaySettings::DataFields onScreenDisplaySettingsDataPriv = osdSettingsObj->getData();

    switch (onScreenDisplaySettingsDataPriv.NumPages) {
    default:
    case 6:
        ui->osdPagePos6->setEnabled(true);
        Q_FALLTHROUGH();
    case 5:
        ui->osdPagePos5->setEnabled(true);
        Q_FALLTHROUGH();
    case 4:
        ui->osdPagePos4->setEnabled(true);
        Q_FALLTHROUGH();
    case 3:
        ui->osdPagePos3->setEnabled(true);
        Q_FALLTHROUGH();
    case 2:
        ui->osdPagePos2->setEnabled(true);
        Q_FALLTHROUGH();
    case 1:
        ui->osdPagePos1->setEnabled(true);
        Q_FALLTHROUGH();
    case 0:
        break;
    }

    switch (onScreenDisplaySettingsDataPriv.NumPages) {
    case 0:
        ui->osdPagePos1->setEnabled(false);
        Q_FALLTHROUGH();
    case 1:
        ui->osdPagePos2->setEnabled(false);
        Q_FALLTHROUGH();
    case 2:
        ui->osdPagePos3->setEnabled(false);
        Q_FALLTHROUGH();
    case 3:
        ui->osdPagePos4->setEnabled(false);
        Q_FALLTHROUGH();
    case 4:
        ui->osdPagePos5->setEnabled(false);
        Q_FALLTHROUGH();
    case 5:
        ui->osdPagePos6->setEnabled(false);
        Q_FALLTHROUGH();
    case 6:
    default:
        break;
    }
}

void ConfigOsdWidget::setCustomText()
{
    const QString text = ui->le_custom_text->displayText();
    unsigned int n_string = text.size();

    for (unsigned int i = 0; i < OnScreenDisplaySettings::CUSTOMTEXT_NUMELEM; ++i) {
        if (i < n_string) {
            osdSettingsObj->setCustomText(i, (quint8)(text.data()[i].toLatin1()));
        } else {
            osdSettingsObj->setCustomText(i, 0);
        }
    }
}

void ConfigOsdWidget::getCustomText()
{
    char text[OnScreenDisplaySettings::CUSTOMTEXT_NUMELEM];

    for (unsigned int i = 0; i < OnScreenDisplaySettings::CUSTOMTEXT_NUMELEM; ++i) {
        text[i] = osdSettingsObj->getCustomText(i);
    }
    QString q_text = QString::fromLatin1(text, OnScreenDisplaySettings::CUSTOMTEXT_NUMELEM);
    ui->le_custom_text->setText(q_text);
}

void ConfigOsdWidget::setupOsdPage(Ui::OsdPage *page, QWidget *page_widget, const QString &objName)
{
    page->setupUi(page_widget);
    const QString nameRel = QStringLiteral("objname:%0").arg(objName);

    for (const auto wid : page_widget->findChildren<QWidget *>()) {
        const auto rel = wid->property("objrelation");
        if (!rel.isValid())
            continue;
        auto relList = rel.toStringList();
        for (const auto &s : qAsConst(relList)) {
            if (s.startsWith(QStringLiteral("fieldname:"))) {
                relList.append(nameRel);
                wid->setProperty("objrelation", relList);
                break;
            }
        }
    }
}

void ConfigOsdWidget::copyOsdPage(int to, int from)
{
    for (const auto checkbox : pages[from]->findChildren<QCheckBox *>()) {
        auto cb_to = pages[to]->findChild<QCheckBox *>(checkbox->objectName());
        if (cb_to) {
            cb_to->setChecked(checkbox->checkState());
        }
    }

    for (const auto spinbox : pages[from]->findChildren<QSpinBox *>()) {
        auto sb_to = pages[to]->findChild<QSpinBox *>(spinbox->objectName());
        if (sb_to) {
            sb_to->setValue(spinbox->value());
        }
    }

    for (const auto combobox : pages[from]->findChildren<QComboBox *>()) {
        auto cmb_to = pages[to]->findChild<QComboBox *>(combobox->objectName());
        if (cmb_to) {
            cmb_to->setCurrentIndex(combobox->currentIndex());
        }
    }
}

void ConfigOsdWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void ConfigOsdWidget::enableControls(bool enable)
{
    Q_UNUSED(enable);
}

/**
 * @}
 * @}
 */
