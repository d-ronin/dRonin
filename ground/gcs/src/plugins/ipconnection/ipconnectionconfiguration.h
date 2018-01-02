/**
 ******************************************************************************
 *
 * @file       IPconnectionconfiguration.h
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

#ifndef IPconnectionCONFIGURATION_H
#define IPconnectionCONFIGURATION_H

#include <coreplugin/iuavgadgetconfiguration.h>
#include <QString>
#include <QSettings>
#include <QVector>
#include <QMetaType>

using namespace Core;

class IPConnectionConfiguration : public IUAVGadgetConfiguration
{
    Q_OBJECT

public:
    explicit IPConnectionConfiguration(QString classId, QSettings *qSettings = nullptr,
                                       QObject *parent = nullptr);

    virtual ~IPConnectionConfiguration();
    void saveConfig() const;
    void readConfig();
    IUAVGadgetConfiguration *clone();

    enum Protocol {
        ProtocolTcp,
        ProtocolUdp,
    };

    struct Host
    {
        Protocol protocol = ProtocolTcp;
        QString hostname = "localhost";
        int port = 9000;

        inline bool operator==(const Host &rhs) const
        {
            return protocol == rhs.protocol && port == rhs.port && hostname == rhs.hostname;
        }
    };

    QVector<Host> &hosts() { return m_hosts; }
    void setHosts(QVector<Host> &hosts) { m_hosts = hosts; }

private:
    QVector<Host> m_hosts;
};

Q_DECLARE_METATYPE(IPConnectionConfiguration::Protocol)

#endif // IPconnectionCONFIGURATION_H
