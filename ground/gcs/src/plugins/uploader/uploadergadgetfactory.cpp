/**
 ******************************************************************************
 *
 * @file       uploadergadgetfactory.cpp
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

#include "uploadergadgetfactory.h"
#include "uploadergadget.h"
#include <coreplugin/iuavgadget.h>
#include "uploadergadgetwidget.h"

using namespace uploader;

UploaderGadgetFactory::UploaderGadgetFactory(QObject *parent) :
    IUAVGadgetFactory(QString("Uploader"), tr("Uploader"), parent)
{
    setSingleConfigurationGadgetTrue();
}

UploaderGadgetFactory::~UploaderGadgetFactory()
{
}

Core::IUAVGadget* UploaderGadgetFactory::createGadget(QWidget *parent)
{
    UploaderGadgetWidget* gadgetWidget = new UploaderGadgetWidget(parent);
    connect(gadgetWidget, SIGNAL(newBoardSeen(deviceInfo,deviceDescriptorStruct)), this, SIGNAL(newBoardSeen(deviceInfo,deviceDescriptorStruct)));
    return new UploaderGadget(QString("Uploader"), gadgetWidget, parent);
}

IUAVGadgetConfiguration *UploaderGadgetFactory::createConfiguration(QSettings *qSettings)
{
    Q_UNUSED(qSettings);
    return NULL;
}
