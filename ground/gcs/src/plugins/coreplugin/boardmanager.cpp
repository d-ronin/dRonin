/**
 ******************************************************************************
 *
 * @file       boardmanager.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 *
 *   The board manager listens for the arrival or "Board Type" objects, which are
 * instanciated by board plugins. Any plugin that needs to interface with a board
 * at a low level - Serial, USB, etc - can query the Board Manager to understand
 * how to to it. At the moment, the board manager provides information to the RawHID
 * USB library to help it detect boards that are supported. On the roadmap are the
 * following features:
 *    - Provide information on bootloader support - supported or not, what protocol
 *    - Provide information on telemetry protocol to use for that particular board
 *    - Provide board description: textual, icon, board image
 *    - Provide hardware configuration description
 *    - Enable board detection routines over 'generic' links such as serial or network
 *    - Provide board configuration panels to be used by the config plugin
 *
 *
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

#include "boardmanager.h"
#include <aggregation/aggregate.h>
#include <extensionsystem/pluginmanager.h>

#include <QDebug>

namespace Core {

BoardManager::BoardManager()
{
}

BoardManager::~BoardManager()
{
}

void BoardManager::init()
{
    // register to the plugin manager so we can receive
    // new connection object from plugins
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject *)),
                     this, SLOT(objectAdded(QObject *)));
    QObject::connect(ExtensionSystem::PluginManager::instance(),
                     SIGNAL(aboutToRemoveObject(QObject *)), this,
                     SLOT(aboutToRemoveObject(QObject *)));
}

/**
 * @brief BoardManager::getKnownVendorIDs
 *   Note: the list is deduplicated, each known VendorID appears
 *         only once.
 * @return list of known vendor IDs
 */
QList<int> BoardManager::getKnownVendorIDs()
{
    QSet<int> res;
    for (const auto board : qAsConst(m_boardTypesList)) {
        auto vids = board->getVendorIDs();
        for (const auto vid : vids)
            res.insert(vid);
    }
    return res.toList();
}

/**
*   Slot called when a plugin added an object to the core pool
*   We want to check whether this object is a BoardType and if so,
*   register it in our list
*/
void BoardManager::objectAdded(QObject *obj)
{
    // Check if a plugin added a board type object to the pool
    IBoardType *board = Aggregation::query<IBoardType>(obj);
    if (!board)
        return;

    // Keep track of the registration
    m_boardTypesList.append(board);
}

/**
 * @brief BoardManager::aboutToRemoveObject
 * @param obj
 */
void BoardManager::aboutToRemoveObject(QObject *obj)
{
    // Check if a plugin removed a board type from the pool
    IBoardType *board = Aggregation::query<IBoardType>(obj);
    if (!board)
        return;

    if (m_boardTypesList.contains(board))
        m_boardTypesList.removeAt(m_boardTypesList.indexOf(board));
}

IBoardType *BoardManager::getBoard(int type)
{
    foreach (IBoardType *board, m_boardTypesList) {
        if (board->getBoardType() == type) {
            return board;
        }
    }
    return Q_NULLPTR;
}

QList<IBoardType::USBInfo> BoardManager::getKnownFirmwareUSBInfo()
{
    QList<IBoardType::USBInfo> res;
    for (const auto board : qAsConst(m_boardTypesList)) {
        const auto infos = board->firmwareUSBInfo();
        for (const auto &info : qAsConst(infos))
            res.append(info);
    }
    return res;
}

QList<IBoardType::USBInfo> BoardManager::getKnownBootloaderUSBInfo()
{
    QList<IBoardType::USBInfo> res;
    for (const auto board : qAsConst(m_boardTypesList)) {
        const auto infos = board->bootloaderUSBInfo();
        for (const auto &info : qAsConst(infos))
            res.append(info);
    }
    return res;
}

} // Core
