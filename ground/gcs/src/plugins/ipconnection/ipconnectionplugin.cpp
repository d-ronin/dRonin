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
    m_config = new IPconnectionConfiguration("IP Network Telemetry", NULL, this);
    m_config->restoresettings();

    m_optionspage = new IPconnectionOptionsPage(m_config,this);

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
    QList <Core::IDevice*> list;
    if (m_config->HostName().length()>1)
        dev.setDisplayName(m_config->HostName());
    else
        dev.setDisplayName("Unconfigured");
    dev.setName(m_config->HostName());
    //we only have one "device" as defined by the configuration m_config
    list.append(&dev);

    return list;
}

void IPConnection::openDevice(QString HostName, int Port, bool UseTCP)
{
    const int Timeout = 5 * 1000;

    if (UseTCP) {
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
    if((HostName.length()==0)||(Port<1)){
        errorMsg = "Please configure Host and Port options before opening the connection";

    } else {
        //try to connect...
        ipSocket->connectToHost(HostName, Port);

        //in blocking mode so we wait for the connection to succeed
        if (ipSocket->waitForConnected(Timeout)) {
            return;
        }

        // tell user what went wrong
        errorMsg = ipSocket->errorString ();

        delete ipSocket;

        ipSocket = NULL;
    }
}

QIODevice *IPConnection::openDevice(Core::IDevice *deviceName)
{
    Q_UNUSED(deviceName)

    QString HostName;
    int Port;
    bool UseTCP;
    QMessageBox msgBox;

    //get the configuration info
    HostName = m_config->HostName();
    Port = m_config->Port();
    UseTCP = m_config->UseTCP();

    if (ipSocket){
        ipSocket->close();
        delete ipSocket;
        ipSocket = NULL;
    }

    openDevice(HostName, Port, UseTCP);

    if(ipSocket == NULL)
    {
        msgBox.setText((const QString )errorMsg);
        msgBox.exec();
    }

    return ipSocket;
}

void IPConnection::closeDevice(const QString &)
{
    if (ipSocket){
        ipSocket->close();
        delete ipSocket;
        ipSocket = NULL;
    }
}


QString IPConnection::connectionName()
{
    return QString("Network telemetry port");
}

QString IPConnection::shortName()
{
    if (m_config->UseTCP()) {
        return QString("TCP");
    } else {
        return QString("UDP");
    }
}


IPconnectionPlugin::IPconnectionPlugin()
{//no change from serial plugin
}

IPconnectionPlugin::~IPconnectionPlugin()
{//manually remove the options page object
    removeObject(m_connection->Optionspage());
}

void IPconnectionPlugin::extensionsInitialized()
{
    addAutoReleasedObject(m_connection);
}

bool IPconnectionPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);
    m_connection = new IPConnection();
    //must manage this registration of child object ourselves
    //if we use an autorelease here it causes the GCS to crash
    //as it is deleting objects as the app closes...
    addObject(m_connection->Optionspage());

    return true;
}
