/**
 ******************************************************************************
 *
 * @file       IPconnectionconfiguration.cpp
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
 */

#include "ipconnectionconfiguration.h"
#include <coreplugin/icore.h>

IPConnectionConfiguration::IPConnectionConfiguration(QString classId, QSettings* qSettings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent)
{
    Q_UNUSED(qSettings);

    qRegisterMetaType<IPConnectionConfiguration::Protocol>();
}

IPConnectionConfiguration::~IPConnectionConfiguration()
{
}

IUAVGadgetConfiguration *IPConnectionConfiguration::clone()
{
    IPConnectionConfiguration *m = new IPConnectionConfiguration(this->classId());
    m->setHosts(m_hosts);
    return m;
}

/**
 * Saves a configuration.
 *
 */
void IPConnectionConfiguration::saveConfig() const
{
    auto settings = Core::ICore::instance()->settings();

    settings->beginGroup(QLatin1String("IPConnection"));
    settings->beginWriteArray("Hosts");

    for (int i = 0; i < m_hosts.count(); i++) {
        settings->setArrayIndex(i);
        settings->setValue("hostname", m_hosts.at(i).hostname);
        settings->setValue("port", m_hosts.at(i).port);
        settings->setValue("protocol", m_hosts.at(i).protocol);
    }
    settings->endArray();
    settings->endGroup();
}


void IPConnectionConfiguration::readConfig()
{
    auto settings = Core::ICore::instance()->settings();

    m_hosts.clear();
    settings->beginGroup(QLatin1String("IPConnection"));
    int elements = qMax<int>(settings->beginReadArray("Hosts"), 1);
    for (int i = 0; i < elements; i++) {
        settings->setArrayIndex(i);
        Host host;
        host.hostname = settings->value("hostname", "localhost").toString();
        host.port = settings->value("port", 9000).toInt();
        host.protocol = static_cast<Protocol>(settings->value("protocol", ProtocolTcp).toInt());
        m_hosts.append(host);
    }
    settings->endArray();
    settings->endGroup();
}

/**
 * @}
 * @}
 */
