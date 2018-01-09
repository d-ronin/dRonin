/**
 ******************************************************************************
 *
 * @file       configgadgetoptionspage.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update settings in the firmware
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
#ifndef CONFIGGADGETOPTIONSPAGE_H
#define CONFIGGADGETOPTIONSPAGE_H

#include "coreplugin/dialogs/ioptionspage.h"

namespace Core {
class IUAVGadgetConfiguration;
}
using namespace Core;

class ConfigGadgetConfiguration;

class ConfigGadgetOptionsPage : public IOptionsPage
{
public:
    Q_OBJECT
public:
    explicit ConfigGadgetOptionsPage(ConfigGadgetConfiguration *config, QObject *parent = nullptr);
    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
};

#endif // CONFIGGADGETOPTIONSPAGE_H
