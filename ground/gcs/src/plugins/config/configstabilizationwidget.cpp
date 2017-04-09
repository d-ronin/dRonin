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
 * with this program; if not, see <http://www.gnu.org/licenses/>
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

ConfigStabilizationWidget::ConfigStabilizationWidget(QWidget *parent)
    : ConfigTaskWidget(parent)
    , manualControlSettings(nullptr)
{
    m_stabilization = new Ui_StabilizationWidget();
    m_stabilization->setupUi(this);

    m_stabilization->tabWidget->setCurrentIndex(0);

    updateInProgress = false;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings *settings = pm->getObject<Core::Internal::GeneralSettings>();

    if (!settings->useExpertMode())
        m_stabilization->saveStabilizationToRAM_6->setVisible(false);

    // display switch arming not selected warning when hangtime enabled
    connect(m_stabilization->sbHangtimeDuration,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::hangtimeDurationChanged);
    manualControlSettings = getObjectManager()->getObject(ManualControlSettings::NAME);
    if (manualControlSettings)
        connect(manualControlSettings, &UAVObject::objectUpdated, this,
                &ConfigStabilizationWidget::hangtimeDurationChanged);
    connect(m_stabilization->gbHangtime, &QGroupBox::toggled, this,
            &ConfigStabilizationWidget::hangtimeToggle);

    autoLoadWidgets();

    connect(m_stabilization->checkBox_7, &QCheckBox::stateChanged, this,
            &ConfigStabilizationWidget::linkCheckBoxes);
    connect(m_stabilization->checkBox_2, &QCheckBox::stateChanged, this,
            &ConfigStabilizationWidget::linkCheckBoxes);
    connect(m_stabilization->checkBox_8, &QCheckBox::stateChanged, this,
            &ConfigStabilizationWidget::linkCheckBoxes);
    connect(m_stabilization->checkBox_3, &QCheckBox::stateChanged, this,
            &ConfigStabilizationWidget::linkCheckBoxes);

    connect(m_stabilization->cbLinkRateRollYaw, &QCheckBox::stateChanged, this,
            &ConfigStabilizationWidget::ratesLink);
    connect(m_stabilization->cbLinkRateRollPitch, &QCheckBox::stateChanged, this,
            &ConfigStabilizationWidget::ratesLink);

    connect(m_stabilization->sliderLTRoll, &QAbstractSlider::valueChanged,
            m_stabilization->rateRollLT, &QSpinBox::setValue);
    connect(m_stabilization->rateRollLT, QOverload<int>::of(&QSpinBox::valueChanged),
            m_stabilization->sliderLTRoll, &QSlider::setValue);
    connect(m_stabilization->sliderLTPitch, &QAbstractSlider::valueChanged,
            m_stabilization->ratePitchLT, &QSpinBox::setValue);
    connect(m_stabilization->ratePitchLT, QOverload<int>::of(&QSpinBox::valueChanged),
            m_stabilization->sliderLTPitch, &QSlider::setValue);
    connect(m_stabilization->sliderLTYaw, &QAbstractSlider::valueChanged,
            m_stabilization->rateYawLT, &QSpinBox::setValue);
    connect(m_stabilization->rateYawLT, QOverload<int>::of(&QSpinBox::valueChanged),
            m_stabilization->sliderLTYaw, &QSlider::setValue);

    connect(m_stabilization->fullStickRateRoll,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::setMaximums);
    connect(m_stabilization->fullStickRatePitch,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::setMaximums);
    connect(m_stabilization->fullStickRateYaw, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigStabilizationWidget::setMaximums);
    connect(m_stabilization->centerStickRateRoll, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::derivedValuesChanged);
    connect(m_stabilization->centerStickRatePitch, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConfigStabilizationWidget::derivedValuesChanged);
    connect(m_stabilization->centerStickRateYaw, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::derivedValuesChanged);
    connect(m_stabilization->rateRollLT, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::derivedValuesChanged);
    connect(m_stabilization->ratePitchLT, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::derivedValuesChanged);
    connect(m_stabilization->rateYawLT, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::derivedValuesChanged);

    connect(m_stabilization->sliderCRateRoll, &QAbstractSlider::valueChanged,
            m_stabilization->centerStickRateRoll, &QSpinBox::setValue);
    connect(m_stabilization->centerStickRateRoll, QOverload<int>::of(&QSpinBox::valueChanged),
            m_stabilization->sliderCRateRoll, &QSlider::setValue);
    connect(m_stabilization->sliderCRatePitch, &QAbstractSlider::valueChanged,
            m_stabilization->centerStickRatePitch, &QSpinBox::setValue);
    connect(m_stabilization->centerStickRatePitch, QOverload<int>::of(&QSpinBox::valueChanged),
            m_stabilization->sliderCRatePitch, &QSlider::setValue);
    connect(m_stabilization->sliderCRateYaw, &QAbstractSlider::valueChanged,
            m_stabilization->centerStickRateYaw, &QSpinBox::setValue);
    connect(m_stabilization->centerStickRateYaw, QOverload<int>::of(&QSpinBox::valueChanged),
            m_stabilization->sliderCRateYaw, &QSlider::setValue);

    connect(m_stabilization->rateRollExpo, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigStabilizationWidget::sourceValuesChanged);
    connect(m_stabilization->ratePitchExpo, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigStabilizationWidget::sourceValuesChanged);
    connect(m_stabilization->rateYawExpo, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigStabilizationWidget::sourceValuesChanged);

    connect(m_stabilization->rateRollExponent, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigStabilizationWidget::sourceValuesChanged);
    connect(m_stabilization->ratePitchExponent,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &ConfigStabilizationWidget::sourceValuesChanged);
    connect(m_stabilization->rateYawExponent, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ConfigStabilizationWidget::sourceValuesChanged);

    connect(this, &ConfigTaskWidget::widgetContentsChanged, this,
            &ConfigStabilizationWidget::processLinkedWidgets);

    disableMouseWheelEvents();

    connect(this, &ConfigTaskWidget::autoPilotConnected, this,
            &ConfigStabilizationWidget::applyRateLimits);

    connect(this, &ConfigTaskWidget::autoPilotConnected, this,
            &ConfigStabilizationWidget::enableDerivedControls);
    connect(this, &ConfigTaskWidget::autoPilotDisconnected, this,
            &ConfigStabilizationWidget::disableDerivedControls);

    disableDerivedControls();
}

void ConfigStabilizationWidget::setDerivedControlsEnabled(bool enable)
{
    m_stabilization->sliderLTRoll->setEnabled(enable);
    m_stabilization->rateRollLT->setEnabled(enable);
    m_stabilization->sliderLTPitch->setEnabled(enable);
    m_stabilization->ratePitchLT->setEnabled(enable);
    m_stabilization->sliderLTYaw->setEnabled(enable);
    m_stabilization->rateYawLT->setEnabled(enable);
    m_stabilization->sliderCRateRoll->setEnabled(enable);
    m_stabilization->centerStickRateRoll->setEnabled(enable);
    m_stabilization->sliderCRatePitch->setEnabled(enable);
    m_stabilization->centerStickRatePitch->setEnabled(enable);
    m_stabilization->sliderCRateYaw->setEnabled(enable);
    m_stabilization->centerStickRateYaw->setEnabled(enable);
}

void ConfigStabilizationWidget::enableDerivedControls()
{
    setDerivedControlsEnabled(true);
}

void ConfigStabilizationWidget::disableDerivedControls()
{
    setDerivedControlsEnabled(false);
}

ConfigStabilizationWidget::~ConfigStabilizationWidget()
{
    // Do nothing
}

void ConfigStabilizationWidget::ratesLink(int value)
{
    Q_UNUSED(value);

    bool hideYaw = m_stabilization->cbLinkRateRollYaw->isChecked();
    bool hidePitch = m_stabilization->cbLinkRateRollPitch->isChecked();

    m_stabilization->lblYawRate->setHidden(hideYaw);
    m_stabilization->sliderFullStickRateYaw->setHidden(hideYaw);
    m_stabilization->sliderLTYaw->setHidden(hideYaw);
    m_stabilization->sliderCRateYaw->setHidden(hideYaw);
    m_stabilization->fullStickRateYaw->setHidden(hideYaw);
    m_stabilization->rateYawLT->setHidden(hideYaw);
    m_stabilization->centerStickRateYaw->setHidden(hideYaw);
    m_stabilization->sliderRateYawExpo->setHidden(hideYaw);
    m_stabilization->rateYawExpo->setHidden(hideYaw);
    m_stabilization->sliderExponentYaw->setHidden(hideYaw);
    m_stabilization->rateYawExponent->setHidden(hideYaw);

    m_stabilization->lblPitchRate->setHidden(hidePitch);
    m_stabilization->sliderFullStickRatePitch->setHidden(hidePitch);
    m_stabilization->sliderLTPitch->setHidden(hidePitch);
    m_stabilization->sliderCRatePitch->setHidden(hidePitch);
    m_stabilization->fullStickRatePitch->setHidden(hidePitch);
    m_stabilization->ratePitchLT->setHidden(hidePitch);
    m_stabilization->centerStickRatePitch->setHidden(hidePitch);
    m_stabilization->sliderRatePitchExpo->setHidden(hidePitch);
    m_stabilization->ratePitchExpo->setHidden(hidePitch);
    m_stabilization->sliderExponentPitch->setHidden(hidePitch);
    m_stabilization->ratePitchExponent->setHidden(hidePitch);

    QString rollLbl = QString(tr("Roll"));

    if (hidePitch) {
        rollLbl += tr(" & Pitch");
    }

    if (hideYaw) {
        rollLbl += tr(" & Yaw");
    }

    m_stabilization->lblRollRate->setText(rollLbl);

    sourceValuesChanged();
}

void ConfigStabilizationWidget::linkCheckBoxes(int value)
{
    if (sender() == m_stabilization->checkBox_7)
        m_stabilization->checkBox_3->setCheckState((Qt::CheckState)value);
    else if (sender() == m_stabilization->checkBox_3)
        m_stabilization->checkBox_7->setCheckState((Qt::CheckState)value);
    else if (sender() == m_stabilization->checkBox_8)
        m_stabilization->checkBox_2->setCheckState((Qt::CheckState)value);
    else if (sender() == m_stabilization->checkBox_2)
        m_stabilization->checkBox_8->setCheckState((Qt::CheckState)value);
}

void ConfigStabilizationWidget::setMaximums()
{
    m_stabilization->centerStickRateRoll->setMaximum(m_stabilization->fullStickRateRoll->value());
    m_stabilization->centerStickRatePitch->setMaximum(m_stabilization->fullStickRatePitch->value());
    m_stabilization->centerStickRateYaw->setMaximum(m_stabilization->fullStickRateYaw->value());

    m_stabilization->sliderCRateRoll->setMaximum(m_stabilization->fullStickRateRoll->value());
    m_stabilization->sliderCRatePitch->setMaximum(m_stabilization->fullStickRatePitch->value());
    m_stabilization->sliderCRateYaw->setMaximum(m_stabilization->fullStickRateYaw->value());

    derivedValuesChanged();
    updateGraphs();
}

void ConfigStabilizationWidget::sourceValuesChanged()
{
    if (updateInProgress) {
        return;
    }

    updateInProgress = true;

    bool hideYaw = m_stabilization->cbLinkRateRollYaw->isChecked();
    bool hidePitch = m_stabilization->cbLinkRateRollPitch->isChecked();

    if (hideYaw) {
        m_stabilization->fullStickRateYaw->setValue(m_stabilization->fullStickRateRoll->value());
        m_stabilization->rateYawExpo->setValue(m_stabilization->rateRollExpo->value());
        m_stabilization->rateYawExponent->setValue(m_stabilization->rateRollExponent->value());
    }

    if (hidePitch) {
        m_stabilization->fullStickRatePitch->setValue(m_stabilization->fullStickRateRoll->value());
        m_stabilization->ratePitchExpo->setValue(m_stabilization->rateRollExpo->value());
        m_stabilization->ratePitchExponent->setValue(m_stabilization->rateRollExponent->value());
    }

    /* invert 'derived' math / operations */
    m_stabilization->centerStickRateRoll->setValue(m_stabilization->fullStickRateRoll->value()
                                                   * (100 - m_stabilization->rateRollExpo->value())
                                                   / 100);
    m_stabilization->centerStickRatePitch->setValue(
        m_stabilization->fullStickRatePitch->value()
        * (100 - m_stabilization->ratePitchExpo->value()) / 100);
    m_stabilization->centerStickRateYaw->setValue(m_stabilization->fullStickRateYaw->value()
                                                  * (100 - m_stabilization->rateYawExpo->value())
                                                  / 100);

    m_stabilization->rateRollLT->setValue(
        100 * exp(log(0.01) / m_stabilization->rateRollExponent->value()));
    m_stabilization->ratePitchLT->setValue(
        100 * exp(log(0.01) / m_stabilization->ratePitchExponent->value()));
    m_stabilization->rateYawLT->setValue(
        100 * exp(log(0.01) / m_stabilization->rateYawExponent->value()));

    updateInProgress = false;

    updateGraphs();
}

void ConfigStabilizationWidget::derivedValuesChanged()
{
    if (updateInProgress) {
        return;
    }

    updateInProgress = true;

    bool hideYaw = m_stabilization->cbLinkRateRollYaw->isChecked();
    bool hidePitch = m_stabilization->cbLinkRateRollPitch->isChecked();

    if (hideYaw) {
        m_stabilization->fullStickRateYaw->setValue(m_stabilization->fullStickRateRoll->value());
        m_stabilization->centerStickRateYaw->setValue(
            m_stabilization->centerStickRateRoll->value());
        m_stabilization->rateYawLT->setValue(m_stabilization->rateRollLT->value());
    }

    if (hidePitch) {
        m_stabilization->fullStickRatePitch->setValue(m_stabilization->fullStickRateRoll->value());
        m_stabilization->centerStickRatePitch->setValue(
            m_stabilization->centerStickRateRoll->value());
        m_stabilization->ratePitchLT->setValue(m_stabilization->rateRollLT->value());
    }

    m_stabilization->rateRollExpo->setValue(100
                                            - m_stabilization->centerStickRateRoll->value() * 100
                                                / m_stabilization->fullStickRateRoll->value());
    m_stabilization->ratePitchExpo->setValue(100
                                             - m_stabilization->centerStickRatePitch->value() * 100
                                                 / m_stabilization->fullStickRatePitch->value());
    m_stabilization->rateYawExpo->setValue(100
                                           - m_stabilization->centerStickRateYaw->value() * 100
                                               / m_stabilization->fullStickRateYaw->value());

    m_stabilization->rateRollExponent->setValue(
        log(0.01) / log((m_stabilization->rateRollLT->value() / 100.0f)));
    m_stabilization->ratePitchExponent->setValue(
        log(0.01) / log((m_stabilization->ratePitchLT->value() / 100.0f)));
    m_stabilization->rateYawExponent->setValue(
        log(0.01) / log((m_stabilization->rateYawLT->value() / 100.0f)));

    updateInProgress = false;

    updateGraphs();
}

void ConfigStabilizationWidget::updateGraphs()
{
    m_stabilization->expoPlot->plotDataRoll(m_stabilization->rateRollExpo->value(),
                                            m_stabilization->fullStickRateRoll->value(),
                                            m_stabilization->rateRollExponent->value() * 10.0);
    m_stabilization->expoPlot->plotDataPitch(m_stabilization->ratePitchExpo->value(),
                                             m_stabilization->fullStickRatePitch->value(),
                                             m_stabilization->ratePitchExponent->value() * 10.0);
    m_stabilization->expoPlot->plotDataYaw(m_stabilization->rateYawExpo->value(),
                                           m_stabilization->fullStickRateYaw->value(),
                                           m_stabilization->rateYawExponent->value() * 10.0);
}

void ConfigStabilizationWidget::processLinkedWidgets(QWidget *widget)
{
    if (m_stabilization->checkBox_7->checkState() == Qt::Checked) {
        if (widget == m_stabilization->RateRollKp) {
            m_stabilization->RatePitchKp->setValue(m_stabilization->RateRollKp->value());
        } else if (widget == m_stabilization->RateRollKi) {
            m_stabilization->RatePitchKi->setValue(m_stabilization->RateRollKi->value());
        } else if (widget == m_stabilization->RateRollILimit) {
            m_stabilization->RatePitchILimit->setValue(m_stabilization->RateRollILimit->value());
        } else if (widget == m_stabilization->RatePitchKp) {
            m_stabilization->RateRollKp->setValue(m_stabilization->RatePitchKp->value());
        } else if (widget == m_stabilization->RatePitchKi) {
            m_stabilization->RateRollKi->setValue(m_stabilization->RatePitchKi->value());
        } else if (widget == m_stabilization->RatePitchILimit) {
            m_stabilization->RateRollILimit->setValue(m_stabilization->RatePitchILimit->value());
        } else if (widget == m_stabilization->RollRateKd) {
            m_stabilization->PitchRateKd->setValue(m_stabilization->RollRateKd->value());
        } else if (widget == m_stabilization->PitchRateKd) {
            m_stabilization->RollRateKd->setValue(m_stabilization->PitchRateKd->value());
        }
    }
    if (m_stabilization->checkBox_8->checkState() == Qt::Checked) {
        if (widget == m_stabilization->AttitudeRollKp) {
            m_stabilization->AttitudePitchKp->setValue(m_stabilization->AttitudeRollKp->value());
        } else if (widget == m_stabilization->AttitudeRollKi) {
            m_stabilization->AttitudePitchKi->setValue(m_stabilization->AttitudeRollKi->value());
        } else if (widget == m_stabilization->AttitudeRollILimit) {
            m_stabilization->AttitudePitchILimit->setValue(
                m_stabilization->AttitudeRollILimit->value());
        } else if (widget == m_stabilization->AttitudePitchKp) {
            m_stabilization->AttitudeRollKp->setValue(m_stabilization->AttitudePitchKp->value());
        } else if (widget == m_stabilization->AttitudePitchKi) {
            m_stabilization->AttitudeRollKi->setValue(m_stabilization->AttitudePitchKi->value());
        } else if (widget == m_stabilization->AttitudePitchILimit) {
            m_stabilization->AttitudeRollILimit->setValue(
                m_stabilization->AttitudePitchILimit->value());
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
    else if (m_stabilization->sbHangtimeDuration->value() == 0.0)
        m_stabilization->sbHangtimeDuration->setValue(2.5); // default duration in s
}
