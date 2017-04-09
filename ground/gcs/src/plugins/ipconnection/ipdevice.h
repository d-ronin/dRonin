/**
 ******************************************************************************
 *
 * @file       ipdevice.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup CorePlugin Core Plugin
 * @{
 * @brief The Core GCS plugin
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
#ifndef IPDEVICE_H
#define IPDEVICE_H

#include <coreplugin/idevice.h>
#include "ipconnectionconfiguration.h"

/**
 * @brief The IPDevice class
 *
 */

class IPDevice : public Core::IDevice
{
    Q_OBJECT

public:
    IPDevice();

    IPConnectionConfiguration::Host host() const { return m_host; }
    void setHost(IPConnectionConfiguration::Host host) { m_host = host; }

private:
    IPConnectionConfiguration::Host m_host;
};

#endif // IPDEVICE_H
