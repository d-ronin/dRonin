/**
 ******************************************************************************
 *
 * @file       IPconnectionplugin.cpp
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

#include "ipconnectionplugin.h"


#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>

#include <QtCore/QtPlugin>
#include <QMainWindow>
#include <QMessageBox>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

#include <QDebug>

IPConnection * connection = 0;

IPConnection::IPConnection()
{
    ipSocket = NULL;
    //create all our objects
    m_config = new IPConnectionConfiguration("IP Network Telemetry", NULL, this);
    m_config->readConfig();

    m_optionspage = new IPConnectionOptionsPage(m_config,this);

    //just signal whenever we have a device event...
    QMainWindow *mw = Core::ICore::instance()->mainWindow();
    QObject::connect(mw, SIGNAL(deviceChange()),
                     this, SLOT(onEnumerationChanged()));
    QObject::connect(m_optionspage, SIGNAL(availableDevChanged()),
                     this, SLOT(onEnumerationChanged()));
}

IPConnection::~IPConnection()
{//clean up our resources...
    if (ipSocket) {
        ipSocket->close();
        delete(ipSocket);
    }
}

void IPConnection::onEnumerationChanged()
{
    emit availableDevChanged(this);
}

QList<IDevice *> IPConnection::availableDevices()
{
    const auto &newHosts = m_config->hosts();

    // add new devices
    for (const auto &host : newHosts) {
        bool found = false;
        for (auto dev : devices) {
            if (static_cast<const IPDevice *>(dev)->host() == host) {
                found = true;
                break;
            }
        }
        if (!found) {
            auto dev = new IPDevice();
            QString name = QString("%0://%1:%2")
                    .arg(host.protocol == IPConnectionConfiguration::ProtocolTcp ? "tcp" : "udp")
                    .arg(host.hostname)
                    .arg(host.port);
            dev->setDisplayName(name);
            dev->setName(name);
            dev->setHost(host);
            devices.append(dev);  // TODO: memory leak, seems like an awkward interface..?
        }
    }

    // clear out removed devices
    for (int i = 0; i < devices.length(); ) {
        if (!newHosts.contains(static_cast<const IPDevice *>(devices.at(i))->host())) {
            devices.at(i)->deleteLater();
            devices.removeAt(i);
        } else {
            i++;
        }
    }

    return devices;
}

QIODevice *IPConnection::openDevice(Core::IDevice *device)
{
    auto *dev = qobject_cast<IPDevice *>(device);
    if (!dev)
        return nullptr;

    const int timeout = 5 * 1000;

    if (dev->host().protocol == IPConnectionConfiguration::ProtocolTcp) {
        ipSocket = new QTcpSocket(this);
    } else {
        ipSocket = new QUdpSocket(this);
    }

    // Disable Nagle algorithm to try and get data promptly rather than
    // minimize packets.
    ipSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // Allow reuse of ports, so if we're running the simulator and GCS on
    // the same host, and simulator crashes leaving ports in FIN_WAIT, we
    // can restart simulator immediately.
    ipSocket->bind(0, QAbstractSocket::ShareAddress);

    //do sanity check on hostname and port...
    if (!dev->host().hostname.length() || dev->host().port < 1 || dev->host().port > 65535){
        errorMsg = tr("Please configure host and port options before opening the connection");
    } else {
        // try to connect...
        ipSocket->connectToHost(dev->host().hostname, static_cast<quint16>(dev->host().port));

        // in blocking mode so we wait for the connection to succeed
        if (ipSocket->waitForConnected(timeout)) {
            return ipSocket;
        }

        // tell user what went wrong
        errorMsg = ipSocket->errorString();

        delete ipSocket;
        ipSocket = nullptr;

        QMessageBox msgBox(QMessageBox::Critical, tr("Connection Failed"), errorMsg);
        msgBox.exec();
    }

    return nullptr;
}

void IPConnection::closeDevice(const QString &)
{
    if (ipSocket){
        ipSocket->close();
        delete ipSocket;
        ipSocket = NULL;
    }
}


QString IPConnection::connectionName() const
{
    return QString("Network telemetry port");
}

QString IPConnection::shortName() const
{
    return tr("IP");
}


IPConnectionPlugin::IPConnectionPlugin()
{//no change from serial plugin
}

IPConnectionPlugin::~IPConnectionPlugin()
{//manually remove the options page object
    removeObject(m_connection->optionsPage());
}

void IPConnectionPlugin::extensionsInitialized()
{
    addAutoReleasedObject(m_connection);
}

bool IPConnectionPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);
    m_connection = new IPConnection();
    //must manage this registration of child object ourselves
    //if we use an autorelease here it causes the GCS to crash
    //as it is deleting objects as the app closes...
    addObject(m_connection->optionsPage());

    return true;
}

/*void IPConnectionPlugin::readConfig(QSettings *settings, Core::UAVConfigInfo *configInfo)
{

}

void IPConnectionPlugin::saveConfig(QSettings *settings, Core::UAVConfigInfo *configInfo)
{

}*/
