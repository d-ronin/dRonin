/**
 ******************************************************************************
 *
 * @file       configstabilizationwidget.h
 * @author     E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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
 */
#include "configstabilizationwidget.h"
#include "convertmwrate.h"

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


ConfigStabilizationWidget::ConfigStabilizationWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    m_stabilization = new Ui_StabilizationWidget();
    m_stabilization->setupUi(this);


    ExtensionSystem::PluginManager *pm=ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings * settings=pm->getObject<Core::Internal::GeneralSettings>();
    if(!settings->useExpertMode() || true)
        m_stabilization->saveStabilizationToRAM_6->setVisible(false);
    m_stabilization->saveStabilizationToRAM_6->setVisible(true);

    

    autoLoadWidgets();
    realtimeUpdates=new QTimer(this);
    connect(m_stabilization->realTimeUpdates_6,SIGNAL(stateChanged(int)),this,SLOT(realtimeUpdatesSlot(int)));
    connect(realtimeUpdates,SIGNAL(timeout()),this,SLOT(apply()));

    connect(m_stabilization->checkBox_7,SIGNAL(stateChanged(int)),this,SLOT(linkCheckBoxes(int)));
    connect(m_stabilization->checkBox_2,SIGNAL(stateChanged(int)),this,SLOT(linkCheckBoxes(int)));
    connect(m_stabilization->checkBox_8,SIGNAL(stateChanged(int)),this,SLOT(linkCheckBoxes(int)));
    connect(m_stabilization->checkBox_3,SIGNAL(stateChanged(int)),this,SLOT(linkCheckBoxes(int)));

    connect(this,SIGNAL(widgetContentsChanged(QWidget*)),this,SLOT(processLinkedWidgets(QWidget*)));

    connect(m_stabilization->calculateMW, SIGNAL(clicked()), this, SLOT(showMWRateConvertDialog()));

    disableMouseWheelEvents();

    connect(this,SIGNAL(autoPilotConnected()),this,SLOT(applyRateLimits()));


    // -----------------------------
    // Expo Plots
    //------------------------------

    m_stabilization->rateStickExpoPlot->init(ExpoCurve::RateCurve, 0);
    connect(m_stabilization->rateRollExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->ratePitchExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->rateYawExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->fullStickRateRoll, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->fullStickRatePitch, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->fullStickRateYaw, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));

    m_stabilization->attitudeStickExpoPlot->init(ExpoCurve::AttitudeCurve, 0);
    connect(m_stabilization->attitudeRollExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->attitudePitchExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->attitudeYawExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->rateRollKp_3, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->ratePitchKp_4, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->rateYawKp_3, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));

    /* The init value for horizontransisition of 85% / 0.85 as used in  here is copied/ the same as in the defined in /flight/Modules/Stabilization/stabilization.c
     * Please be aware of changes that are made there. */
    m_stabilization->horizonStickExpoPlot->init(ExpoCurve::HorizonCurve, 85);
    connect(m_stabilization->horizonRollExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->horizonPitchExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));
    connect(m_stabilization->horizonYawExpo, SIGNAL(valueChanged(double)), this, SLOT(showExpoPlot()));

    // set the flags for all Expo Plots, so that they get updatet after initialization
    update_exp.RateRoll = true;
    update_exp.RatePitch = true;
    update_exp.RateYaw = true;
    update_exp.AttitudeRoll = true;
    update_exp.AttitudePitch = true;
    update_exp.AttitudeYaw = true;
    update_exp.HorizonAttitudeRoll = true;
    update_exp.HorizonAttitudePitch = true;
    update_exp.HorizonAttitudeYaw = true;
    update_exp.HorizonRateRoll = true;
    update_exp.HorizonRatePitch = true;
    update_exp.HorizonRateYaw = true;

    //  update the Expo Plots
    showExpoPlot();

}

ConfigStabilizationWidget::~ConfigStabilizationWidget()
{
    // Do nothing
}

void ConfigStabilizationWidget::realtimeUpdatesSlot(int value)
{
    m_stabilization->realTimeUpdates_6->setCheckState((Qt::CheckState)value);
    if(value==Qt::Checked && !realtimeUpdates->isActive())
        realtimeUpdates->start(300);
    else if(value==Qt::Unchecked)
        realtimeUpdates->stop();
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
        if(widget== m_stabilization->RateRollKp_2)
        {
            m_stabilization->RatePitchKp->setValue(m_stabilization->RateRollKp_2->value());
        }
        else if(widget== m_stabilization->RateRollKi_2)
        {
            m_stabilization->RatePitchKi->setValue(m_stabilization->RateRollKi_2->value());
        }
        else if(widget== m_stabilization->RateRollILimit_2)
        {
            m_stabilization->RatePitchILimit->setValue(m_stabilization->RateRollILimit_2->value());
        }
        else if(widget== m_stabilization->RatePitchKp)
        {
            m_stabilization->RateRollKp_2->setValue(m_stabilization->RatePitchKp->value());
        }
        else if(widget== m_stabilization->RatePitchKi)
        {
            m_stabilization->RateRollKi_2->setValue(m_stabilization->RatePitchKi->value());
        }
        else if(widget== m_stabilization->RatePitchILimit)
        {
            m_stabilization->RateRollILimit_2->setValue(m_stabilization->RatePitchILimit->value());
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
            m_stabilization->AttitudePitchKp_2->setValue(m_stabilization->AttitudeRollKp->value());
        }
        else if(widget== m_stabilization->AttitudeRollKi)
        {
            m_stabilization->AttitudePitchKi_2->setValue(m_stabilization->AttitudeRollKi->value());
        }
        else if(widget== m_stabilization->AttitudeRollILimit)
        {
            m_stabilization->AttitudePitchILimit_2->setValue(m_stabilization->AttitudeRollILimit->value());
        }
        else if(widget== m_stabilization->AttitudePitchKp_2)
        {
            m_stabilization->AttitudeRollKp->setValue(m_stabilization->AttitudePitchKp_2->value());
        }
        else if(widget== m_stabilization->AttitudePitchKi_2)
        {
            m_stabilization->AttitudeRollKi->setValue(m_stabilization->AttitudePitchKi_2->value());
        }
        else if(widget== m_stabilization->AttitudePitchILimit_2)
        {
            m_stabilization->AttitudeRollILimit->setValue(m_stabilization->AttitudePitchILimit_2->value());
        }
    }

    // sync the multiwii rate settings
    if (m_stabilization->cb_linkMwRollPitch->checkState()==Qt::Checked) {
        if (widget == m_stabilization->MWRatePitchKp)
            m_stabilization->MWRateRollKp->setValue(m_stabilization->MWRatePitchKp->value());
        else if (widget == m_stabilization->MWRateRollKp)
            m_stabilization->MWRatePitchKp->setValue(m_stabilization->MWRateRollKp->value());
        else if (widget == m_stabilization->MWRatePitchKi)
            m_stabilization->MWRateRollKi->setValue(m_stabilization->MWRatePitchKi->value());
        else if (widget == m_stabilization->MWRateRollKi)
            m_stabilization->MWRatePitchKi->setValue(m_stabilization->MWRateRollKi->value());
        else if (widget == m_stabilization->MWRatePitchKd)
            m_stabilization->MWRateRollKd->setValue(m_stabilization->MWRatePitchKd->value());
        else if (widget == m_stabilization->MWRateRollKd)
            m_stabilization->MWRatePitchKd->setValue(m_stabilization->MWRateRollKd->value());
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

void ConfigStabilizationWidget::showMWRateConvertDialog()
{
    ConvertMWRate *dialog = new ConvertMWRate(this);

    connect(dialog, SIGNAL(accepted()), this, SLOT(applyMWRateConvertDialog()));
    dialog->exec();
}

void ConfigStabilizationWidget::applyMWRateConvertDialog()
{
    ConvertMWRate *dialog = dynamic_cast<ConvertMWRate *>(sender());
    if (dialog) {
        m_stabilization->MWRateRollKp->setValue(dialog->getRollKp());
        m_stabilization->MWRateRollKi->setValue(dialog->getRollKi());
        m_stabilization->MWRateRollKd->setValue(dialog->getRollKd());
        m_stabilization->MWRatePitchKp->setValue(dialog->getPitchKp());
        m_stabilization->MWRatePitchKi->setValue(dialog->getPitchKi());
        m_stabilization->MWRatePitchKd->setValue(dialog->getPitchKd());
        m_stabilization->MWRateYawKp->setValue(dialog->getYawKp());
        m_stabilization->MWRateYawKi->setValue(dialog->getYawKi());
        m_stabilization->MWRateYawKd->setValue(dialog->getYawKd());
    }
}


/**
 * @brief ConfigStabilizationWidget::showExpoPlot() Gets the data from the data fileds in UI, and calls the corresponding functions to plot the data.
 * Can be used as a slot or normal function
 * Tests if its called as a Slot from a Signal or not, checks the flags, reads the corresponding data and calls the plot functions
 */
void ConfigStabilizationWidget::showExpoPlot()
{
    // test if this function is called from a Signal of one of the edit fields in UI (Spin Boxes)
    if(QObject* obj = sender()) {
        // set the flags to update the plots that rely on the changed data
        if ( (obj == m_stabilization->horizonRollExpo) || (obj == m_stabilization->rateRollKp_3) ) {
           update_exp.HorizonAttitudeRoll = true;
           if ( (obj == m_stabilization->horizonRollExpo) ) {
            update_exp.HorizonRateRoll = true;
           }
           if ( (obj == m_stabilization->rateRollKp_3) ) {
            update_exp.AttitudeRoll = true;
           }
        }

        else if ( (obj == m_stabilization->horizonPitchExpo) || (obj == m_stabilization->ratePitchKp_4) ) {
           update_exp.HorizonAttitudePitch = true;
           if ( (obj == m_stabilization->horizonPitchExpo) ) {
            update_exp.HorizonRatePitch = true;
           }
           if ( (obj == m_stabilization->ratePitchKp_4) ) {
               update_exp.AttitudePitch = true;
           }
        }

        else if ( (obj == m_stabilization->horizonYawExpo) || (obj == m_stabilization->rateYawKp_3) ) {
           update_exp.HorizonAttitudeYaw = true;
           if ( (obj == m_stabilization->horizonYawExpo) ) {
            update_exp.HorizonRateYaw = true;
           }
           if ( (obj == m_stabilization->rateYawKp_3) ) {
            update_exp.AttitudeYaw = true;
           }
        }

        else if ( (obj == m_stabilization->rateRollExpo) || (obj == m_stabilization->fullStickRateRoll) ) {
           update_exp.RateRoll = true;
           if ( (obj == m_stabilization->fullStickRateRoll) ) {
            update_exp.HorizonRateRoll = true;
           }
        }

        else if ( (obj == m_stabilization->ratePitchExpo) || (obj == m_stabilization->fullStickRatePitch) ) {
           update_exp.RatePitch = true;
           if ( (obj == m_stabilization->fullStickRatePitch) ) {
            update_exp.HorizonRatePitch = true;
           }
        }

        else if ( (obj == m_stabilization->rateYawExpo) || (obj == m_stabilization->fullStickRateYaw) ) {
           update_exp.RateYaw = true;

           if ( (obj == m_stabilization->fullStickRateYaw) ) {
            update_exp.HorizonRateYaw = true;
           }
        }
        else if ( (obj == m_stabilization->attitudeRollExpo) ) {
           update_exp.AttitudeRoll = true;
        }

        else if ( (obj == m_stabilization->attitudePitchExpo) ) {
           update_exp.AttitudePitch = true;
        }

        else if ( (obj == m_stabilization->attitudeYawExpo) ) {
           update_exp.AttitudeYaw = true;
        }
    }

    // update the Plots with latest data if the correspopnding flag is set
    // Horizon Attitude Curves
    if (update_exp.HorizonAttitudeRoll) {
      m_stabilization->horizonStickExpoPlot->plotDataRoll(m_stabilization->horizonRollExpo->value(), m_stabilization->rateRollKp_3->value(), ExpoCurve::Y_Left);
      update_exp.HorizonAttitudeRoll = false;
    }
    if (update_exp.HorizonAttitudePitch) {
      m_stabilization->horizonStickExpoPlot->plotDataPitch(m_stabilization->horizonPitchExpo->value(), m_stabilization->ratePitchKp_4->value(), ExpoCurve::Y_Left);
      update_exp.HorizonAttitudePitch = false;
    }
    if (update_exp.HorizonAttitudeYaw) {
      m_stabilization->horizonStickExpoPlot->plotDataYaw(m_stabilization->horizonYawExpo->value(), m_stabilization->rateYawKp_3->value(), ExpoCurve::Y_Left);
      update_exp.HorizonAttitudeYaw = false;
    }
    // Horizon Rate Curves
    if (update_exp.HorizonRateRoll) {
      m_stabilization->horizonStickExpoPlot->plotDataRoll(m_stabilization->horizonRollExpo->value(), m_stabilization->fullStickRateRoll->value(), ExpoCurve::Y_Right);
      update_exp.HorizonRateRoll = false;
    }
    if (update_exp.HorizonRatePitch) {
      m_stabilization->horizonStickExpoPlot->plotDataPitch(m_stabilization->horizonPitchExpo->value(), m_stabilization->fullStickRatePitch->value(), ExpoCurve::Y_Right);
      update_exp.HorizonRatePitch = false;
    }
    if (update_exp.HorizonRateYaw) {
      m_stabilization->horizonStickExpoPlot->plotDataYaw(m_stabilization->horizonYawExpo->value(), m_stabilization->fullStickRateYaw->value(), ExpoCurve::Y_Right);
      update_exp.HorizonRateYaw = false;
    }
    // Rate Curves
    if (update_exp.RateRoll) {
      m_stabilization->rateStickExpoPlot->plotDataRoll(m_stabilization->rateRollExpo->value(), m_stabilization->fullStickRateRoll->value(), ExpoCurve::Y_Left);
      update_exp.RateRoll = false;
    }
    if (update_exp.RatePitch) {
      m_stabilization->rateStickExpoPlot->plotDataPitch(m_stabilization->ratePitchExpo->value(), m_stabilization->fullStickRatePitch->value(), ExpoCurve::Y_Left);
      update_exp.RatePitch = false;
    }
    if (update_exp.RateYaw) {
      m_stabilization->rateStickExpoPlot->plotDataYaw(m_stabilization->rateYawExpo->value(), m_stabilization->fullStickRateYaw->value(), ExpoCurve::Y_Left);
      update_exp.RateYaw = false;
    }
    // Attitude Curves
    if (update_exp.AttitudeRoll) {
      m_stabilization->attitudeStickExpoPlot->plotDataRoll(m_stabilization->attitudeRollExpo->value(), m_stabilization->rateRollKp_3->value(), ExpoCurve::Y_Left);
      update_exp.RateRoll = false;
    }
    if (update_exp.AttitudePitch) {
      m_stabilization->attitudeStickExpoPlot->plotDataPitch(m_stabilization->attitudePitchExpo->value(), m_stabilization->ratePitchKp_4->value(), ExpoCurve::Y_Left);
      update_exp.RatePitch = false;
    }
    if (update_exp.AttitudeYaw) {
      m_stabilization->attitudeStickExpoPlot->plotDataYaw(m_stabilization->attitudeYawExpo->value(), m_stabilization->rateYawKp_3->value(), ExpoCurve::Y_Left);
      update_exp.RateYaw = false;
    }
}
