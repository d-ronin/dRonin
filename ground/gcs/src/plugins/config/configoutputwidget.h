/**
 ******************************************************************************
 *
 * @file       configoutputwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Servo output configuration panel for the config gadget
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
#ifndef CONFIGOUTPUTWIDGET_H
#define CONFIGOUTPUTWIDGET_H

#include "ui_output.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "uavobjectutilmanager.h"
#include "cfg_vehicletypes/vehicleconfig.h"
#include <QWidget>
#include <QList>

class Ui_OutputWidget;
class OutputChannelForm;

class ConfigOutputWidget : public ConfigTaskWidget
{
    Q_OBJECT

public:
    ConfigOutputWidget(QWidget *parent = 0);
    ~ConfigOutputWidget();

private:
    enum SpecialOutputRates {
        RATE_SYNCPWM = 0,
        RATE_DSHOT300 = 65532,
        RATE_DSHOT600 = 65533,
        RATE_DSHOT1200 = 65534,
    };

    Ui_OutputWidget *m_config;

    QList<QSlider> sliders;

    void updateChannelInSlider(QSlider *slider, QLabel *min, QLabel *max, QCheckBox *rev,
                               int value);

    void assignChannel(UAVDataObject *obj, QString str);
    OutputChannelForm *getOutputChannelForm(const int index) const;
    int mccDataRate;

    //! List of dropdowns for the timer rate
    QList<QComboBox *> rateList;
    //! List of dropdowns for the timer resolution
    QList<QComboBox *> resList;
    //! List of timer grouping labels
    QList<QLabel *> lblList;

    // For naming custom rates and OneShot
    QString timerFreqToString(quint32) const;
    quint32 timerStringToFreq(QString) const;

    UAVObject::Metadata accInitialData;

    virtual void tabSwitchingAway();

private slots:
    void stopTests();
    virtual void refreshWidgetsValues(UAVObject *obj = NULL);
    void updateObjectsFromWidgets();
    void runChannelTests(bool state);
    void sendChannelTest(int index, int value);
    void startESCCalibration();
    void openHelp();
    void do_SetDirty();
    void assignOutputChannels(UAVObject *obj);
    void refreshWidgetRanges();

protected:
    void enableControls(bool enable);
};

#endif
