/**
 ******************************************************************************
 *
 * @file       serialplugin.cpp
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

#include "serialplugin.h"
#include "serialdevice.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>

#include <QtCore/QtPlugin>
#include <QMainWindow>
#include <coreplugin/icore.h>
#include <QDebug>
#include <QFontMetrics>

SerialConnection::SerialConnection()
    : enablePolling(true),
      m_deviceOpened(false)
{
    serialHandle = NULL;
    m_config = new SerialPluginConfiguration("Serial Telemetry", NULL, this);
    m_config->restoresettings();

    m_optionspage = new SerialPluginOptionsPage(m_config,this);

    connect(&periodicTimer, SIGNAL(timeout()), this, SLOT(periodic()));
    periodicTimer.start(1000);
}

SerialConnection::~SerialConnection()
{
}

void SerialConnection::onEnumerationChanged()
{
}

bool sortPorts(const QSerialPortInfo &s1, const QSerialPortInfo &s2)
{
    return s1.portName() < s2.portName();
}

void SerialConnection::periodic()
{
    if (!this->deviceOpened()) {
        QList <Core::IDevice*> newDev = this->availableDevices();

        // Ignore the output, as now availableDevices signals!
    }
}

QList <IDevice *> SerialConnection::availableDevices()
{
    static QList <Core::IDevice*> m_available_device_list;
    if (enablePolling) {
        bool changed = false;

        QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

        //sort the list by port number (nice idea from PT_Dreamer :))
        qSort(ports.begin(), ports.end(),sortPorts);
        bool port_exists;
        foreach(QSerialPortInfo port, ports) {
            port_exists = false;
            foreach(IDevice *device, m_available_device_list) {
                if(device->getName() == port.portName()) {
                    port_exists = true;
                    break;
                }
            }
            if(!port_exists) {
                SerialDevice* d = new SerialDevice();
                QStringList disp;
                if (port.description().length()) {
                    QFontMetrics font((QFont()));
                    disp.append(font.elidedText(port.description(), Qt::ElideRight, 150));
                }
                disp.append(port.portName());
                d->setDisplayName(disp.join(" - "));
                d->setName(port.portName());
                m_available_device_list.append(d);

                changed = true;
            }
        }
        foreach(IDevice *device,m_available_device_list) {
            port_exists = false;
            foreach(QSerialPortInfo port, ports) {
                if(device->getName() == port.portName()) {
                    port_exists = true;
                    break;
                }
            }
            if(!port_exists)
            {
                m_available_device_list.removeOne(device);
                device->deleteLater();

                changed = true;
            }
        }

        if (changed) {
            emit availableDevChanged(this);
        }
    }

    return m_available_device_list;
}

QIODevice *SerialConnection::openDevice(IDevice *deviceName)
{
    if (serialHandle){
        closeDevice(deviceName->getName());
    }
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    foreach(QSerialPortInfo port, ports) {
        if (port.portName() == deviceName->getName()) {
            //we need to handle port settings here...

            serialHandle = new QSerialPort(port);
            if (serialHandle->open(QIODevice::ReadWrite)) {
                 if (serialHandle->setBaudRate(m_config->speed().toInt())
	                    && serialHandle->setDataBits(QSerialPort::Data8)
	                    && serialHandle->setParity(QSerialPort::NoParity)
 	                    && serialHandle->setStopBits(QSerialPort::OneStop)
 	                    && serialHandle->setFlowControl(QSerialPort::NoFlowControl)) {
                            m_deviceOpened = true;
                 }
            }
            return serialHandle;
        }
    }
    return NULL;
}

void SerialConnection::closeDevice(const QString &deviceName)
{
    Q_UNUSED(deviceName);
    //we have to delete the serial connection we created
    if (serialHandle){
        serialHandle->deleteLater();
        serialHandle = NULL;
        m_deviceOpened = false;
    }
}


QString SerialConnection::connectionName()
{
    return QString("Serial port");
}

QString SerialConnection::shortName()
{
    return QString("Serial");
}

/**
 Tells the Serial plugin to stop polling for serial devices
 */
void SerialConnection::suspendPolling()
{
    enablePolling = false;
}

/**
 Tells the Serial plugin to resume polling for serial devices
 */
void SerialConnection::resumePolling()
{
    enablePolling = true;
}

SerialPlugin::SerialPlugin()
{
}

SerialPlugin::~SerialPlugin()
{
    removeObject(m_connection->Optionspage());
}

void SerialPlugin::extensionsInitialized()
{
    addAutoReleasedObject(m_connection);
}

bool SerialPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);
    m_connection = new SerialConnection();
    //must manage this registration of child object ourselves
    //if we use an autorelease here it causes the GCS to crash
    //as it is deleting objects as the app closes...
    addObject(m_connection->Optionspage());
    return true;
}
