/**
 ******************************************************************************
 *
 * @file       uavsettingsimportexportmanager.h
 * @author     (C) 2011 The OpenPilot Team, http://www.openpilot.org
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVSettingsImportExport UAVSettings Import/Export Plugin
 * @{
 * @brief UAVSettings Import/Export Plugin
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
#ifndef UAVSETTINGSIMPORTEXPORTFACTORY_H
#define UAVSETTINGSIMPORTEXPORTFACTORY_H
#include "uavsettingsimportexport_global.h"
#include "uavobjectutil/uavobjectutilmanager.h"
#include "../../../../../build/ground/gcs/gcsversioninfo.h"

class QDomNode;

class UAVSETTINGSIMPORTEXPORT_EXPORT UAVSettingsImportExportManager : public QObject
{
    Q_OBJECT

public:
    UAVSettingsImportExportManager(QObject *parent = 0);
    ~UAVSettingsImportExportManager();
    static bool updateObject(UAVObject *obj, QDomNode * node);

    bool importUAVSettings(const QByteArray &settings, bool quiet = false);

public slots:
    void importUAVSettings();
    void exportUAVSettings();
    void exportUAVData();

private:
    enum storedData { Settings, Data, Both };
    QString createXMLDocument(const enum storedData, const bool fullExport);

signals:
    void importAboutToBegin();
    void importEnded();

};

#endif // UAVSETTINGSIMPORTEXPORTFACTORY_H
