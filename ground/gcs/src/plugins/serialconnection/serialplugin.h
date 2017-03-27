/**
 ******************************************************************************
 *
 * @file       serialplugin.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SerialPlugin Serial Connection Plugin
 * @{
 * @brief Impliments serial connection to the flight hardware for Telemetry
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

#ifndef SERIALPLUGIN_H
#define SERIALPLUGIN_H

//#include "serial_global.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "coreplugin/iconnection.h"
#include <extensionsystem/iplugin.h>
#include "serialpluginconfiguration.h"
#include "serialpluginoptionspage.h"
#include <QTimer>

class IConnection;
class QSerialPortInfo;
class SerialConnection;

/**
*   Define a connection via the IConnection interface
*   Plugin will add a instance of this class to the pool,
*   so the connection manager can use it.
*/
class SerialConnection
    : public Core::IConnection
{
    Q_OBJECT
public:
    SerialConnection();
    virtual ~SerialConnection();

    virtual QList <Core::IDevice*> availableDevices();
    virtual QIODevice *openDevice(Core::IDevice *deviceName);
    virtual void closeDevice(const QString &deviceName);

    virtual QString connectionName();
    virtual QString shortName();
    virtual void suspendPolling();
    virtual void resumePolling();
    virtual bool reconnect() { return m_config->reconnect(); }
    bool deviceOpened() {return m_deviceOpened;}
    SerialPluginConfiguration * Config() const { return m_config; }
    SerialPluginOptionsPage * Optionspage() const { return m_optionspage; }

private:
    QSerialPort*  serialHandle;
    bool enablePolling;
    SerialPluginConfiguration *m_config;
    SerialPluginOptionsPage *m_optionspage;

private slots:
    void periodic();

protected slots:
    void onEnumerationChanged();

protected:
    bool m_deviceOpened;

    QTimer periodicTimer;
};

//class SERIAL_EXPORT SerialPlugin
class SerialPlugin
    : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.dronin.plugins.Serial")

public:
    SerialPlugin();
    ~SerialPlugin();

    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void extensionsInitialized();
private:
    SerialConnection *m_connection;
};


#endif // SERIALPLUGIN_H
