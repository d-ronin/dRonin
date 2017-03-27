/**
 ******************************************************************************
 *
 * @file       IPconnectionplugin.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup IPConnPlugin IP Telemetry Plugin
 * @{
 * @brief IP Connection Plugin impliment telemetry over TCP/IP and UDP/IP
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

#ifndef IPconnectionPLUGIN_H
#define IPconnectionPLUGIN_H

#include "ipconnection_global.h"
#include "ipconnectionoptionspage.h"
#include "ipconnectionconfiguration.h"
#include "coreplugin/iconnection.h"
#include "ipdevice.h"
#include <extensionsystem/iplugin.h>
//#include <QtCore/QSettings>


class QAbstractSocket;
class QTcpSocket;
class QUdpSocket;

class IConnection;
/**
*   Define a connection via the IConnection interface
*   Plugin will add a instance of this class to the pool,
*   so the connection manager can use it.
*/
class IPconnection_EXPORT IPConnection
    : public Core::IConnection
{
    Q_OBJECT
public:
    IPConnection();
    virtual ~IPConnection();

    virtual QList <Core::IDevice*> availableDevices();
    virtual QIODevice *openDevice(Core::IDevice *deviceName);
    virtual void closeDevice(const QString &deviceName);

    virtual QString connectionName() const;
    virtual QString shortName() const;

    IPConnectionOptionsPage *optionsPage() const { return m_optionspage; }

protected slots:
    void onEnumerationChanged();

private:
    void openDevice(QString HostName, int Port, bool UseTCP);
    QAbstractSocket *ipSocket;
    IPConnectionConfiguration *m_config;
    IPConnectionOptionsPage *m_optionspage;
    QList<Core::IDevice *> devices;
    QString errorMsg;
};


class IPconnection_EXPORT IPConnectionPlugin
    : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.dronin.plugins.IPConnection")
public:
    IPConnectionPlugin();
    ~IPConnectionPlugin();

    virtual bool initialize(const QStringList &arguments, QString *error_message);
    virtual void extensionsInitialized();

private:
    IPConnection *m_connection;
};


#endif // IPconnectionPLUGIN_H
