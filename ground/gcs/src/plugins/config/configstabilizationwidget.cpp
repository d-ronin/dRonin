/**
 ******************************************************************************
 *
 * @file       configstabilizationwidget.h
 * @author     E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "configstabilizationwidget.h"
#include "manualcontrolsettings.h"

#include <QDebug>
#include <QStringList>
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QList>


#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>


ConfigStabilizationWidget::ConfigStabilizationWidget(QWidget *parent) : ConfigTaskWidget(parent), manualControlSettings(nullptr)
{
    m_stabilization = new Ui_StabilizationWidget();
    m_stabilization->setupUi(this);


    ExtensionSystem::PluginManager *pm=ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings * settings=pm->getObject<Core::Internal::GeneralSettings>();

    if (!settings->useExpertMode())
        m_stabilization->saveStabilizationToRAM_6->setVisible(false);

    // display switch arming not selected warning when hangtime enabled
    connect(m_stabilization->sbHangtimeDuration, SIGNAL(valueChanged(double)), this, SLOT(hangtimeDurationChanged()));
    manualControlSettings = getObjectManager()->getObject(ManualControlSettings::NAME);
    if (manualControlSettings)
        connect(manualControlSettings, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(hangtimeDurationChanged()));
    connect(m_stabilization->gbHangtime, SIGNAL(toggled(bool)), this, SLOT(hangtimeToggle(bool)));


    autoLoadWidgets();

    connect(m_stabilization->checkBox_7,SIGNAL(stateChanged(int)),this,SLOT(linkCheckBoxes(int)));
    connect(m_stabilization->checkBox_2,SIGNAL(stateChanged(int)),this,SLOT(linkCheckBoxes(int)));
    connect(m_stabilization->checkBox_8,SIGNAL(stateChanged(int)),this,SLOT(linkCheckBoxes(int)));
    connect(m_stabilization->checkBox_3,SIGNAL(stateChanged(int)),this,SLOT(linkCheckBoxes(int)));

    connect(this,SIGNAL(widgetContentsChanged(QWidget*)),this,SLOT(processLinkedWidgets(QWidget*)));

    disableMouseWheelEvents();

    connect(this,SIGNAL(autoPilotConnected()),this,SLOT(applyRateLimits()));


}

ConfigStabilizationWidget::~ConfigStabilizationWidget()
{
    // Do nothing
}

void ConfigStabilizationWidget::linkCheckBoxes(int value)
{
    if(sender()== m_stabilization->checkBox_7)
        m_stabilization->checkBox_3->setCheckState((Qt::CheckState)value);
    else if(sender()== m_stabilization->checkBox_3)
        m_stabilization->checkBox_7->setCheckState((Qt::CheckState)value);
    else if(sender()== m_stabilization->checkBox_8)
        m_stabilization->checkBox_2->setCheckState((Qt::CheckState)value);
    else if(sender()== m_stabilization->checkBox_2)
        m_stabilization->checkBox_8->setCheckState((Qt::CheckState)value);
}

void ConfigStabilizationWidget::processLinkedWidgets(QWidget * widget)
{
    if(m_stabilization->checkBox_7->checkState()==Qt::Checked)
    {
        if(widget== m_stabilization->RateRollKp)
        {
            m_stabilization->RatePitchKp->setValue(m_stabilization->RateRollKp->value());
        }
        else if(widget== m_stabilization->RateRollKi)
        {
            m_stabilization->RatePitchKi->setValue(m_stabilization->RateRollKi->value());
        }
        else if(widget== m_stabilization->RateRollILimit)
        {
            m_stabilization->RatePitchILimit->setValue(m_stabilization->RateRollILimit->value());
        }
        else if(widget== m_stabilization->RatePitchKp)
        {
            m_stabilization->RateRollKp->setValue(m_stabilization->RatePitchKp->value());
        }
        else if(widget== m_stabilization->RatePitchKi)
        {
            m_stabilization->RateRollKi->setValue(m_stabilization->RatePitchKi->value());
        }
        else if(widget== m_stabilization->RatePitchILimit)
        {
            m_stabilization->RateRollILimit->setValue(m_stabilization->RatePitchILimit->value());
        }
        else if(widget== m_stabilization->RollRateKd)
        {
            m_stabilization->PitchRateKd->setValue(m_stabilization->RollRateKd->value());
        }
        else if(widget== m_stabilization->PitchRateKd)
        {
            m_stabilization->RollRateKd->setValue(m_stabilization->PitchRateKd->value());
        }
    }
    if(m_stabilization->checkBox_8->checkState()==Qt::Checked)
    {
        if(widget== m_stabilization->AttitudeRollKp)
        {
            m_stabilization->AttitudePitchKp->setValue(m_stabilization->AttitudeRollKp->value());
        }
        else if(widget== m_stabilization->AttitudeRollKi)
        {
            m_stabilization->AttitudePitchKi->setValue(m_stabilization->AttitudeRollKi->value());
        }
        else if(widget== m_stabilization->AttitudeRollILimit)
        {
            m_stabilization->AttitudePitchILimit->setValue(m_stabilization->AttitudeRollILimit->value());
        }
        else if(widget== m_stabilization->AttitudePitchKp)
        {
            m_stabilization->AttitudeRollKp->setValue(m_stabilization->AttitudePitchKp->value());
        }
        else if(widget== m_stabilization->AttitudePitchKi)
        {
            m_stabilization->AttitudeRollKi->setValue(m_stabilization->AttitudePitchKi->value());
        }
        else if(widget== m_stabilization->AttitudePitchILimit)
        {
            m_stabilization->AttitudeRollILimit->setValue(m_stabilization->AttitudePitchILimit->value());
        }
    }
}

void ConfigStabilizationWidget::applyRateLimits()
{
    Core::IBoardType *board = getObjectUtilManager()->getBoardType();

    double maxRate = 500; // Default to slowest across boards
    if (board)
        maxRate = board->queryMaxGyroRate() * 0.85;

    m_stabilization->fullStickRateRoll->setMaximum(maxRate);
    m_stabilization->fullStickRatePitch->setMaximum(maxRate);
    m_stabilization->fullStickRateYaw->setMaximum(maxRate);
}

void ConfigStabilizationWidget::hangtimeDurationChanged()
{
    bool warn = m_stabilization->sbHangtimeDuration->value() > 0.0;

    if (warn && !m_stabilization->gbHangtime->isChecked())
        m_stabilization->gbHangtime->setChecked(true);
    else if (!warn && m_stabilization->gbHangtime->isChecked())
        m_stabilization->gbHangtime->setChecked(false);

    if (manualControlSettings) {
        UAVObjectField *field = manualControlSettings->getField("Arming");
        if (field) {
            const QString option = field->getValue().toString();
            warn &= !option.startsWith("Switch") && option != "Always Disarmed";
        }
    }
    m_stabilization->lblSwitchArmingWarning->setVisible(warn);
}

void ConfigStabilizationWidget::hangtimeToggle(bool enabled)
{
    if (!enabled)
        m_stabilization->sbHangtimeDuration->setValue(0.0); // 0.0 is disabled
    else if(m_stabilization->sbHangtimeDuration->value() == 0.0)
        m_stabilization->sbHangtimeDuration->setValue(2.5); // default duration in s
}
