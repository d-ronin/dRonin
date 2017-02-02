/**
 ******************************************************************************
 *
 * @file       uploadergadgetfactory.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup  Uploader Uploader Plugin
 * @{
 * @brief The Tau Labs uploader plugin factory
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

#ifndef UPLOADERGADGETFACTORY_H
#define UPLOADERGADGETFACTORY_H

#include <coreplugin/iuavgadgetfactory.h>
#include "uploader_global.h"
#include "uavobjectutil/devicedescriptorstruct.h"

namespace Core {
class IUAVGadget;
class IUAVGadgetFactory;
}

using namespace Core;

namespace uploader {

class UPLOADER_EXPORT UploaderGadgetFactory : public Core::IUAVGadgetFactory
{
    Q_OBJECT
public:
    UploaderGadgetFactory(QObject *parent = 0);
    ~UploaderGadgetFactory();

    Core::IUAVGadget *createGadget(QWidget *parent);
    IUAVGadgetConfiguration *createConfiguration(QSettings *qSettings);
private:
signals:
    void newBoardSeen(deviceInfo board, deviceDescriptorStruct device);
};

}
#endif // UPLOADERGADGETFACTORY_H
