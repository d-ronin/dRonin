/**
 ******************************************************************************
 *
 * @file       outputcalibrationutil.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @see        The GNU Public License (GPL) Version 3
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SetupWizard Setup Wizard
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

#ifndef OUTPUTCALIBRATIONUTIL_H
#define OUTPUTCALIBRATIONUTIL_H

#include "actuatorcommand.h"
#include "uavobject.h"
#include "uavobjectmanager.h"
#include "vehicleconfigurationsource.h"
#include <QList>
#include <QObject>

class OutputCalibrationUtil : public QObject
{
    Q_OBJECT
public:
    explicit OutputCalibrationUtil(QObject *parent = 0);
    ~OutputCalibrationUtil();

signals:

public slots:
    void startChannelOutput(quint16 channel, quint16 safeValue);
    void stopChannelOutput();

    void setChannelOutputValue(quint16 value);

private:
    qint16 m_outputChannel;
    quint16 m_safeValue;
    UAVObject::Metadata m_savedActuatorCommandMetadata;
    ActuatorCommand::DataFields m_savedActuatorCommandData;
    UAVObjectManager *m_uavObjectManager;
};

#endif // OUTPUTCALIBRATIONUTIL_H
