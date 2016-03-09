/**
 ******************************************************************************
 *
 * @file       uploadergadgetwidget.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup  Uploader Uploader Plugin
 * @{
 * @brief The Tau Labs uploader plugin main widget
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef UPLOADERGADGETWIDGET_H
#define UPLOADERGADGETWIDGET_H

#include <QPointer>
#include "ui_uploader.h"
#include "tl_dfu.h"
#include <coreplugin/iboardtype.h>
#include "uploader_global.h"
#include "devicedescriptorstruct.h"
#include "uavobjectutilmanager.h"
#include "uavtalk/telemetrymanager.h"
#include "coreplugin/connectionmanager.h"

using namespace tl_dfu;

namespace uploader {

typedef enum { STATUSICON_OK, STATUSICON_RUNNING, STATUSICON_FAIL, STATUSICON_INFO} StatusIcon;

class UPLOADER_EXPORT UploaderGadgetWidget : public QWidget
{
    Q_OBJECT

    struct partitionStruc
    {
        QByteArray partitionData;
        int partitionNumber;
    };

public:
    UploaderGadgetWidget(QWidget *parent = 0);
    ~UploaderGadgetWidget();
    bool autoUpdateCapable();
public slots:
    bool autoUpdate();
signals:
    void bootloaderDetected();
    void rescueTimer(int);
    void rescueFinish(bool);
    void uploadFinish(bool);
    void uploadProgress(UploaderStatus, QVariant);
    void autoUpdateSignal(UploaderStatus, QVariant);
    void newBoardSeen(deviceInfo board, deviceDescriptorStruct device);
private slots:
    void onAutopilotConnect();
    void onAutopilotDisconnect();
    void onAutopilotReady();
    void onIAPPresentChanged(UAVDataObject*);
    void onIAPUpdated();
    void onLoadFirmwareButtonClick();
    void onFlashButtonClick();
    void onRescueButtonClick();
    void onExportButtonClick();
    void onBootloaderDetected();
    void onBootloaderRemoved();
    void onRescueTimer(bool start = false);
    void onStatusUpdate(QString, int);
    void onUploadFinish(tl_dfu::Status);
    void onDownloadSaveFinish(bool);
    void onPartitionSave();
    void onPartitionFlash();
    void onPartitionErase();
    void onBootButtonClick();
    void onAutoUpdateCount(int i);
    void openHelp();
    void onAvailableDevicesChanged(QLinkedList<Core::DevListItem>);
private:
    void FirmwareOnDeviceClear(bool clear);
    void FirmwareLoadedClear(bool clear);
    void PartitionBrowserClear();
    void DeviceInformationClear();
    void DeviceInformationUpdate(deviceInfo board);
    void FirmwareOnDeviceUpdate(deviceDescriptorStruct firmware, QString crc);
    void FirmwareLoadedUpdate(QByteArray firmwareArray);
    QString LoadFirmwareFileDialog(QString);
    uploader::UploaderStatus getUploaderStatus() const;
    void setUploaderStatus(const uploader::UploaderStatus &value);
    void CheckAutopilotReady();
    bool CheckInBootloaderState();
    void setStatusInfo(QString str, uploader::StatusIcon ic);
    QString getFirmwarePathForBoard(QString boardName);
    bool FirmwareLoadFromFile(QString filename);
    bool FirmwareLoadFromFile(QFileInfo filename);
    bool FirmwareCheckForUpdate(deviceDescriptorStruct device);
    void triggerPartitionDownload(int index);
    void haltOrReset(bool halting);

    Ui_UploaderWidget *m_widget;
    bool telemetryConnected;
    bool iapPresent;
    bool iapUpdated;
    UAVObjectUtilManager* utilMngr;
    QByteArray loadedFile;
    DFUObject dfu;
    ExtensionSystem::PluginManager *pm;
    USBSignalFilter *usbFilterBL;
    USBSignalFilter *usbFilterUP;
    TelemetryManager* telMngr;
    Core::ConnectionManager *conMngr;
    FirmwareIAPObj *firmwareIap;
    deviceInfo currentBoard;
    QString lastConnectedTelemetryDevice;
    QString ignoredRev;

    uploader::UploaderStatus uploaderStatus;
    uploader::UploaderStatus previousStatus;
    QByteArray tempArray;
    bool lastUploadResult;
};
}
#endif // UPLOADERGADGETWIDGET_H
