/**
 ******************************************************************************
 *
 * @file       uploadergadgetwidget.cpp
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
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

#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

#include "uploadergadgetwidget.h"
#include "firmwareiapobj.h"
#include "fileutils.h"
#include "coreplugin/icore.h"
#include <coreplugin/modemanager.h>
#include "rawhid/rawhidplugin.h"
#include "../../../../../build/ground/gcs/gcsversioninfo.h"

using namespace uploader;

/**
 * @brief Class constructor, sets signal to slot connections,
 * creates actions, creates utility classes instances, etc
 */
UploaderGadgetWidget::UploaderGadgetWidget(QWidget *parent):QWidget(parent),
    telemetryConnected(false), iapPresent(false), iapUpdated(false)
{
    m_widget = new Ui_UploaderWidget();
    m_widget->setupUi(this);
    setUploaderStatus(uploader::DISCONNECTED);
    m_widget->partitionBrowserTW->setSelectionMode(QAbstractItemView::SingleSelection);
    m_widget->partitionBrowserTW->setSelectionBehavior(QAbstractItemView::SelectRows);

    // these widgets will be set to retain their position in the layout even when hidden
    QList<QWidget *> hideable = {m_widget->progressBar};
    foreach (QWidget *widget, hideable) {
        QSizePolicy sp = widget->sizePolicy();
        sp.setRetainSizeWhenHidden(true);
        widget->setSizePolicy(sp);
    }

    //Setup partition manager actions
    QAction *action = new QAction("Save to file",this);
    connect(action, SIGNAL(triggered()), this, SLOT(onPartitionSave()));
    m_widget->partitionBrowserTW->addAction(action);
    action = new QAction("Flash from file",this);
    connect(action, SIGNAL(triggered()), this, SLOT(onPartitionFlash()));
    m_widget->partitionBrowserTW->addAction(action);
    action = new QAction("Erase",this);
    connect(action, SIGNAL(triggered()), this, SLOT(onPartitionErase()));
    m_widget->partitionBrowserTW->addAction(action);
    m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::ActionsContextMenu);

    //Clear widgets to defaults
    FirmwareOnDeviceClear(true);
    FirmwareLoadedClear(true);
    PartitionBrowserClear();
    DeviceInformationClear();

    pm = ExtensionSystem::PluginManager::instance();
    telMngr = pm->getObject<TelemetryManager>();
    utilMngr = pm->getObject<UAVObjectUtilManager>();

    UAVObjectManager *obm = pm->getObject<UAVObjectManager>();
    connect(telMngr, SIGNAL(connected()), this, SLOT(onAutopilotConnect()));
    connect(telMngr, SIGNAL(disconnected()), this, SLOT(onAutopilotDisconnect()));
    firmwareIap = FirmwareIAPObj::GetInstance(obm);

    connect(firmwareIap, SIGNAL(presentOnHardwareChanged(UAVDataObject*)), this, SLOT(onIAPPresentChanged(UAVDataObject*)));
    connect(firmwareIap, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(onIAPUpdated()), Qt::UniqueConnection);

    //Connect button signals to slots
    connect(m_widget->openButton, SIGNAL(clicked()), this, SLOT(onLoadFirmwareButtonClick()));
    connect(m_widget->rescueButton, SIGNAL(clicked()), this, SLOT(onRescueButtonClick()));
    connect(m_widget->flashButton, SIGNAL(clicked()), this, SLOT(onFlashButtonClick()));
    connect(m_widget->bootButton, SIGNAL(clicked()), this, SLOT(onBootButtonClick()));
    connect(m_widget->safeBootButton, SIGNAL(clicked()), this, SLOT(onBootButtonClick()));
    connect(m_widget->exportConfigButton, SIGNAL(clicked()), this, SLOT(onRescueButtonClick()));

    connect(m_widget->pbHelp, SIGNAL(clicked()),this,SLOT(openHelp()));
    Core::BoardManager* brdMgr = Core::ICore::instance()->boardManager();

    //Setup usb discovery signals for boards in bl state
    usbFilterBL = new USBSignalFilter(brdMgr->getKnownVendorIDs(),-1,-1,USBMonitor::Bootloader);
    usbFilterUP = new USBSignalFilter(brdMgr->getKnownVendorIDs(),-1,-1,USBMonitor::Upgrader);
    connect(usbFilterBL, SIGNAL(deviceRemoved()), this, SLOT(onBootloaderRemoved()));

    connect(usbFilterBL, SIGNAL(deviceDiscovered()), this, SLOT(onBootloaderDetected()), Qt::UniqueConnection);

    connect(usbFilterUP, SIGNAL(deviceRemoved()), this, SLOT(onBootloaderRemoved()));

    connect(usbFilterUP, SIGNAL(deviceDiscovered()), this, SLOT(onBootloaderDetected()), Qt::UniqueConnection);

    conMngr = Core::ICore::instance()->connectionManager();
    connect(conMngr, SIGNAL(availableDevicesChanged(QLinkedList<Core::DevListItem>)), this, SLOT(onAvailableDevicesChanged(QLinkedList<Core::DevListItem>)));
    // Check if a board is already in bootloader state when the GCS starts
    foreach (int i, brdMgr->getKnownVendorIDs()) {
        if(USBMonitor::instance()->availableDevices(i, -1, -1, USBMonitor::Bootloader).length() > 0)
        {
            setUploaderStatus(uploader::RESCUING);
            onBootloaderDetected();
            break;
        }
    }
}

/**
 * @brief Class destructor, nothing needs to be destructed ATM
 */
UploaderGadgetWidget::~UploaderGadgetWidget()
{

}

/**
 * @brief Configure the board to use an receiver input type on a port number
 * @param type the type of receiver to use
 * @param port_num which input port to configure (board specific numbering)
 * @return true if successfully configured or false otherwise
 */

/**
 * @brief Hides or unhides the firmware on device information set
 * when hidden it presents an information not available string
 * @param clear set to true to hide information and false to unhide
 */
void UploaderGadgetWidget::FirmwareOnDeviceClear(bool clear)
{
    QRegExp rx("*OD_lbl");
    rx.setPatternSyntax(QRegExp::Wildcard);
    foreach(QLabel *l, findChildren<QLabel *>(rx))
    {
        if(clear)
            l->clear();
        l->setVisible(!clear);
    }
    m_widget->firmwareOnDevicePic->setVisible(!clear);
    m_widget->noInfoOnDevice->setVisible(clear);
    m_widget->onDeviceGB->setEnabled(!clear);
}

/**
 * @brief Hides or unhides the firmware loaded (from file) information set
 * when hidden it presents an information not available string
 * @param clear set to true to hide information and false to unhide
 */
void UploaderGadgetWidget::FirmwareLoadedClear(bool clear)
{
    QRegExp rx("*LD_lbl");
    rx.setPatternSyntax(QRegExp::Wildcard);
    foreach(QLabel *l, findChildren<QLabel *>(rx))
    {
        if(clear)
            l->clear();
        l->setVisible(!clear);
    }
    m_widget->firmwareLoadedPic->setVisible(!clear);
    m_widget->noFirmwareLoaded->setVisible(clear);
    m_widget->loadedGB->setEnabled(!clear);
    if(clear)
        m_widget->userDefined_LD_lbl->clear();
    m_widget->userDefined_LD_lbl->setVisible(!clear);
}

/**
 * @brief Clears and disables the partition browser table
 */
void UploaderGadgetWidget::PartitionBrowserClear()
{
    m_widget->partitionBrowserTW->clearContents();
    m_widget->partitionBrowserGB->setEnabled(false);
}

void UploaderGadgetWidget::DeviceInformationClear()
{
    m_widget->deviceInformationMainLayout->setVisible(false);
    m_widget->deviceInformationNoInfo->setVisible(true);
    m_widget->boardName_lbl->setVisible(false);
    currentBoard.board = NULL;
}

/**
 * @brief Fills out the device information widgets
 * @param board structure describing the connected hardware
 */
void UploaderGadgetWidget::DeviceInformationUpdate(deviceInfo board)
{
    if(!board.board)
        return;
    currentBoard = board;
    m_widget->boardName_lbl->setText(board.board->boardDescription());
    m_widget->devName_lbl->setText(board.board->getBoardNameFromID(board.board->getBoardType() << 8));
    m_widget->hwRev_lbl->setText(board.hw_revision);
    m_widget->blVer_lbl->setText(board.bl_version);
    m_widget->maxCode_lbl->setText(board.max_code_size);
    m_widget->baroCap_lbl->setVisible(board.board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_BAROS));
    m_widget->accCap_lbl->setVisible(board.board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_ACCELS));
    m_widget->gyroCap_lbl->setVisible(board.board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_GYROS));
    m_widget->magCap_lbl->setVisible(board.board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_MAGS));
    m_widget->radioCap_lbl->setVisible(board.board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_RADIO));
    m_widget->osdCap_lbl->setVisible(board.board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_OSD));
    m_widget->deviceInformationMainLayout->setVisible(true);
    m_widget->deviceInformationNoInfo->setVisible(false);
    m_widget->boardPic->setPixmap(board.board->getBoardPicture().scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    FirmwareLoadedUpdate(loadedFile);
}

/**
 * @brief Fills out the firmware on device information widgets
 * @param firmware structure describing the firmware
 * @param crc firmware crc
 */
void UploaderGadgetWidget::FirmwareOnDeviceUpdate(deviceDescriptorStruct firmware, QString crc)
{
    m_widget->builtForOD_lbl->setText(Core::IBoardType::getBoardNameFromID(firmware.boardID()));
    m_widget->crcOD_lbl->setText(crc);
    m_widget->gitHashOD_lbl->setText(firmware.gitHash);
    m_widget->firmwareDateOD_lbl->setText(firmware.gitDate);
    m_widget->firmwareTagOD_lbl->setText(firmware.gitTag);
    m_widget->uavosSHA_OD_lbl->setText(firmware.uavoHash.toHex().toUpper());
    m_widget->userDefined_OD_lbl->setText(firmware.userDefined);
    if(firmware.certified)
    {
        QPixmap pix = QPixmap(QString(":uploader/images/application-certificate.svg"));
        m_widget->firmwareOnDevicePic->setPixmap(pix);
        m_widget->firmwareOnDevicePic->setToolTip(tr("Tagged officially released firmware build"));
    }
    else
    {
        QPixmap pix = QPixmap(QString(":uploader/images/warning.svg"));
        m_widget->firmwareOnDevicePic->setPixmap(pix);
        m_widget->firmwareOnDevicePic->setToolTip(tr("Custom Firmware Build"));
    }
    FirmwareOnDeviceClear(false);
}

/**
 * @brief Fills out the loaded firmware (from file) information widgets
 * @param firmwareArray array containing the loaded firmware
 */
void UploaderGadgetWidget::FirmwareLoadedUpdate(QByteArray firmwareArray)
{
    if(firmwareArray.isEmpty())
        return;
    deviceDescriptorStruct firmware;
    if(!utilMngr->descriptionToStructure(firmwareArray.right(100), firmware))
    {
        setStatusInfo(tr("Error parsing file metadata"), uploader::STATUSICON_FAIL);
        QPixmap pix = QPixmap(QString(":uploader/images/error.svg"));
        m_widget->firmwareLoadedPic->setPixmap(pix);
        m_widget->firmwareLoadedPic->setToolTip(tr("Error unrecognized file format, proceed at your own risk"));
        m_widget->gitHashLD_lbl->setText(tr("Error unrecognized file format, proceed at your own risk"));
        FirmwareLoadedClear(false);
        m_widget->userDefined_LD_lbl->setVisible(false);
        return;
    }
    setStatusInfo(tr("File loaded successfully"), uploader::STATUSICON_INFO);
    m_widget->builtForLD_lbl->setText(Core::IBoardType::getBoardNameFromID(firmware.boardID()));
    bool ok;
    currentBoard.max_code_size.toLong(&ok);
    if(!ok)
        m_widget->crcLD_lbl->setText("Not Available");
    else if (firmwareArray.length() > currentBoard.max_code_size.toLong()) {
        m_widget->crcLD_lbl->setText("Not Available");
    } else {
        quint32 crc = dfu.CRCFromQBArray(firmwareArray, currentBoard.max_code_size.toLong());
        m_widget->crcLD_lbl->setText(QString::number(crc));
    }
    m_widget->gitHashLD_lbl->setText(firmware.gitHash);
    m_widget->firmwareDateLD_lbl->setText(firmware.gitDate);
    m_widget->firmwareTagLD_lbl->setText(firmware.gitTag);
    m_widget->uavosSHA_LD_lbl->setText(firmware.uavoHash.toHex().toUpper());
    m_widget->userDefined_LD_lbl->setText(firmware.userDefined);
    if(currentBoard.board && (currentBoard.board->getBoardType() != firmware.boardType))
    {
        QPixmap pix = QPixmap(QString(":uploader/images/error.svg"));
        m_widget->firmwareLoadedPic->setPixmap(pix);
        m_widget->firmwareLoadedPic->setToolTip(tr("Error firmware loaded firmware doesn't match connected board"));
    }
    else if(firmware.certified)
    {
        QPixmap pix = QPixmap(QString(":uploader/images/application-certificate.svg"));
        m_widget->firmwareLoadedPic->setPixmap(pix);
        m_widget->firmwareLoadedPic->setToolTip(tr("Tagged officially released firmware build"));
    }
    else
    {
        QPixmap pix = QPixmap(QString(":uploader/images/warning.svg"));
        m_widget->firmwareLoadedPic->setPixmap(pix);
        m_widget->firmwareLoadedPic->setToolTip(tr("Custom Firmware Build"));
    }
    FirmwareLoadedClear(false);
}

/**
 * @brief slot called by the telemetryManager when the autopilot connects
 */
void UploaderGadgetWidget::onAutopilotConnect()
{
    telemetryConnected = true;
    CheckAutopilotReady();
}

/**
 * @brief slot called by the telemetryManager when the autopilot disconnects
 */
void UploaderGadgetWidget::onAutopilotDisconnect()
{
    FirmwareOnDeviceClear(true);
    DeviceInformationClear();
    PartitionBrowserClear();
    telemetryConnected = false;
    iapPresent = false;
    iapUpdated = false;
    if( (getUploaderStatus() == uploader::RESCUING) || (getUploaderStatus() == uploader::HALTING) || (getUploaderStatus() == uploader::BOOTING))
        return;
    setUploaderStatus(uploader::DISCONNECTED);
    setStatusInfo(tr("Telemetry disconnected"), uploader::STATUSICON_INFO);
}

/**
 * @brief Called when the autopilot is ready to be queried for information
 * regarding running firmware and hardware specs
 */
void UploaderGadgetWidget::onAutopilotReady()
{
    setUploaderStatus(uploader::CONNECTED_TO_TELEMETRY);
    setStatusInfo(tr("Telemetry connected"), uploader::STATUSICON_INFO);
    deviceInfo board;
    board.bl_version = tr("Not available");
    board.board = utilMngr->getBoardType();
    board.cpu_serial = utilMngr->getBoardCPUSerial().toHex();
    board.hw_revision = QString::number(utilMngr->getBoardRevision());
    board.max_code_size = tr("Not available");
    DeviceInformationUpdate(board);
    deviceDescriptorStruct device;
    if(utilMngr->getBoardDescriptionStruct(device)) {
        FirmwareOnDeviceUpdate(device, QString::number(utilMngr->getFirmwareCRC()));
        if (FirmwareCheckForUpdate(device)) {
            Core::ModeManager::instance()->activateModeByWorkspaceName("Firmware");
            m_widget->rescueButton->click();
        }
    }
    emit newBoardSeen(board, device);
}

/**
 * @brief slot called by the iap object when the hardware reports it
 * has it
 */
void UploaderGadgetWidget::onIAPPresentChanged(UAVDataObject *obj)
{
    iapPresent = obj->getIsPresentOnHardware();
    if(obj->getIsPresentOnHardware())
        CheckAutopilotReady();
}

/**
 * @brief slot called by the iap object when it is updated
 */
void UploaderGadgetWidget::onIAPUpdated()
{
    if( (getUploaderStatus() == uploader::RESCUING) || (getUploaderStatus() == uploader::HALTING) || (getUploaderStatus() == uploader::BOOTING) )
        return;
    iapUpdated = true;
    CheckAutopilotReady();
}

/**
 * @brief slot called when the user presses the load firmware button
 * It loads the file and calls the information display functions
 */
void UploaderGadgetWidget::onLoadFirmwareButtonClick()
{
    QString board;
    if(currentBoard.board)
        board = currentBoard.board->shortName().toLower();
    QString filename = LoadFirmwareFileDialog(board);
    if(filename.isEmpty())
    {
        setStatusInfo(tr("Invalid Filename"), uploader::STATUSICON_FAIL);
        return;
    }
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        setStatusInfo(tr("Could not open file"), uploader::STATUSICON_FAIL);
        return;
    }
    loadedFile = file.readAll();

    FirmwareLoadedClear(true);
    FirmwareLoadedUpdate(loadedFile);
    setUploaderStatus(getUploaderStatus());
}

/**
 * @brief slot called when the user presses the flash firmware button
 * It start a non blocking firmware upload
 */
void UploaderGadgetWidget::onFlashButtonClick()
{
    setStatusInfo("",uploader::STATUSICON_RUNNING);
    previousStatus = uploaderStatus;
    setUploaderStatus(uploader::UPLOADING_FW);
    onStatusUpdate(QString("Starting upload..."), 0); // set progress bar to 0 while erasing
    connect(&dfu, SIGNAL(operationProgress(QString,int)), this, SLOT(onStatusUpdate(QString, int)));
    connect(&dfu, SIGNAL(uploadFinished(tl_dfu::Status)), this, SLOT(onUploadFinish(tl_dfu::Status)));
    dfu.UploadPartitionThreaded(loadedFile, DFU_PARTITION_FW, currentBoard.max_code_size.toInt());
}

void UploaderGadgetWidget::haltOrReset(bool halting)
{
    lastConnectedTelemetryDevice = conMngr->getCurrentDevice().device.data()->getName();
    if(!firmwareIap->getIsPresentOnHardware())
        return;

    if (halting) {
        setUploaderStatus(uploader::HALTING);
    } else {
        setUploaderStatus(uploader::BOOTING);
    }

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    firmwareIap->setBoardRevision(0);
    firmwareIap->setBoardType(0);
    connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(firmwareIap,SIGNAL(transactionCompleted(UAVObject*,bool)), &loop, SLOT(quit()));
    int magicValue = 1122;
    int magicStep = 1111;
    for(int i = 0; i < 3; ++i)
    {
        //Firmware IAP module specifies that the timing between iap commands must be
        //between 500 and 5000ms
        timeout.start(600);
        loop.exec();
        firmwareIap->setCommand(magicValue);
        magicValue += magicStep;
        if((!halting) && (magicValue == 3344))
            magicValue = 4455;

        setStatusInfo(QString(tr("Sending IAP Step %0").arg(i + 1)), uploader::STATUSICON_INFO);
        firmwareIap->updated();
        timeout.start(1000);
        loop.exec();
        if(!timeout.isActive())
        {
            setStatusInfo(QString(tr("Sending IAP Step %0 failed").arg(i + 1)), uploader::STATUSICON_FAIL);
            return;
        }
        timeout.stop();
    }

    if(conMngr->getCurrentDevice().connection->shortName() == "USB")
    {
        conMngr->disconnectDevice();
        timeout.start(200);
        loop.exec();
        conMngr->suspendPolling();
        onRescueTimer(true);
    }
    else
    {
        setUploaderStatus(uploader::DISCONNECTED);
        conMngr->disconnectDevice();
    }
}

/**
 * @brief slot called when the user presses the rescue button
 * It enables the bootloader detection and starts a detection timer
 */
void UploaderGadgetWidget::onRescueButtonClick()
{
    if (telMngr->isConnected()) {
        // This means "halt" to enter the loader.
        haltOrReset(true);

        return;
    }

    // Otherwise, this means "begin rescue".
    conMngr->suspendPolling();
    setUploaderStatus(uploader::RESCUING);
    setStatusInfo(tr("Please connect the board with USB with no external power applied"), uploader::STATUSICON_INFO);
    onRescueTimer(true);
}

/**
 * @brief slot called when the user selects the Export Config button.
 * It retrieves the setting partition and sends it to the cloud, trading it
 * for an XML configuration file.
 */
void UploaderGadgetWidget::onExportButtonClick()
{
    if (telMngr->isConnected()) {
        /* XXX select the UAV-oriented export thing */

        return;
    }

    /* XXX make sure there's a setting partition */
    /* XXX get confirmation from user that using the cloud service is OK */
    /* XXX pull down settings partition to ram */
    /* XXX post to cloud service */
    /* XXX save dialog for XML config */
}

/**
 * @brief slot called by the USBSignalFilter when it detects a board in bootloader state
 * Fills out partition info, device info, and firmware info
 */
void UploaderGadgetWidget::onBootloaderDetected()
{
    Core::BoardManager* brdMgr = Core::ICore::instance()->boardManager();
    QList<USBPortInfo> devices;

    // If we are already connected to a bootloader, skip
    switch (uploaderStatus) {
    case uploader::BL_FROM_HALT:
    case uploader::BL_FROM_RESCUE:
        return;
    default:
        break;
    }

    bool inUpgrader = false;

    foreach(int vendorID, brdMgr->getKnownVendorIDs()) {
        QList<USBPortInfo> upgraderDevs = USBMonitor::instance()->availableDevices(vendorID,-1,-1,USBMonitor::Upgrader);

        if (upgraderDevs.length() > 0) {
            inUpgrader = true;
            devices.append(upgraderDevs);
        }

        devices.append(USBMonitor::instance()->availableDevices(vendorID,-1,-1,USBMonitor::Bootloader));
    }
    if(devices.length() > 1)
    {
        setStatusInfo(tr("More than one device was detected in bootloader state"), uploader::STATUSICON_INFO);
        return;
    }
    else if(devices.length() == 0)
    {
        setStatusInfo("No devices in bootloader state detected", uploader::STATUSICON_FAIL);
        return;
    }
    if(dfu.OpenBootloaderComs(devices.first()))
    {
        tl_dfu::device dev = dfu.findCapabilities();

        if (!inUpgrader) {
            switch (uploaderStatus) {
            case uploader::HALTING:
            case uploader::RESCUING:
                break;
            case uploader::DISCONNECTED:
            {
                QByteArray description = dfu.DownloadDescriptionAsByteArray(dev.SizeOfDesc);
                // look for completed bootloader update (last 2 chars of TlFw string are nulled)
                if (QString(description.left(4)) == "Tl")
                    break;

                deviceDescriptorStruct descStructure;
                if (UAVObjectUtilManager::descriptionToStructure(description, descStructure)) {
                    if (FirmwareCheckForUpdate(descStructure)) {
                        Core::ModeManager::instance()->activateModeByWorkspaceName("Firmware");
                        break;
                    }
                }
            }
            // fall through to default
            default:
                dfu.JumpToApp(false);
                dfu.CloseBootloaderComs();
                return;
            }
        }

        //Bootloader has new cap extensions, query partitions and fill out browser
        if(dev.CapExt)
        {
            QTableWidgetItem *label;
            QTableWidgetItem *size;
            int index = 0;
            QString name;
            m_widget->partitionBrowserGB->setEnabled(true);
            m_widget->partitionBrowserTW->setRowCount(dev.PartitionSizes.length());
            foreach (int i, dev.PartitionSizes) {
                switch (index) {
                case DFU_PARTITION_FW:
                    name = "Firmware";
                    break;
                case DFU_PARTITION_DESC:
                    name = "Metadata";
                    break;
                case DFU_PARTITION_BL:
                    name = "Bootloader";
                    break;
                case DFU_PARTITION_SETTINGS:
                    name = "Settings";
                    break;
                case DFU_PARTITION_WAYPOINTS:
                    name = "Waypoints";
                    break;
                case DFU_PARTITION_LOG:
                    name = "Log";
                    break;
                default:
                    name = QString::number(index);
                    break;
                }
                label = new QTableWidgetItem(name);
                size = new QTableWidgetItem(QString::number(i));
                size->setTextAlignment(Qt::AlignRight);
                m_widget->partitionBrowserTW->setItem(index, 0, label);
                m_widget->partitionBrowserTW->setItem(index, 1, size);
                ++index;
            }
        }
        deviceInfo info;
        QList <Core::IBoardType *> boards = pm->getObjects<Core::IBoardType>();
        foreach (Core::IBoardType *board, boards) {
            if (board->getBoardType() == (dev.ID>>8))
            {
                info.board = board;
                break;
            }
        }
        info.bl_version = QString::number(dev.BL_Version, 16);
        info.cpu_serial = "Not Available";
        info.hw_revision = QString::number(dev.HW_Rev);
        info.max_code_size = QString::number(dev.SizeOfCode);
        QByteArray description = dfu.DownloadDescriptionAsByteArray(dev.SizeOfDesc);
        deviceDescriptorStruct descStructure;
        if(!UAVObjectUtilManager::descriptionToStructure(description, descStructure))
            setStatusInfo(tr("Could not parse firmware metadata"), uploader::STATUSICON_INFO);
        else
        {
            FirmwareOnDeviceUpdate(descStructure, QString::number((dev.FW_CRC)));
        }
        DeviceInformationUpdate(info);
        switch (uploaderStatus) {
        case uploader::HALTING:
            setUploaderStatus(uploader::BL_FROM_HALT);
            break;
        case uploader::DISCONNECTED:
        case uploader::RESCUING:
            setUploaderStatus(uploader::BL_FROM_RESCUE);
            break;
        default:
            break;
        }

        if (!inUpgrader) {
            setStatusInfo(tr("Connection to bootloader successful"), uploader::STATUSICON_OK);

            if (FirmwareLoadFromFile(getFirmwarePathForBoard(info.board->shortName()))) {
                setStatusInfo(tr("Ready to flash firmware"), uploader::STATUSICON_OK);
                this->activateWindow();
                m_widget->flashButton->setFocus();
            }
        } else {
            setStatusInfo(tr("Connected to upgrader-loader"), uploader::STATUSICON_OK);
        }

        emit bootloaderDetected();
    }
    else
    {
        setStatusInfo(tr("Could not open coms with the bootloader"), uploader::STATUSICON_FAIL);
        setUploaderStatus((uploader::DISCONNECTED));
        conMngr->resumePolling();
    }
}

/**
 * @brief slot called by the USBSignalFilter when it detects a board in bootloader state as been removed
 */
void UploaderGadgetWidget::onBootloaderRemoved()
{
    conMngr->resumePolling();
    setStatusInfo(tr("Bootloader disconnection detected"), uploader::STATUSICON_INFO);
    DeviceInformationClear();
    FirmwareOnDeviceClear(true);
    PartitionBrowserClear();
    setUploaderStatus(uploader::DISCONNECTED);
    dfu.disconnect();
}

/**
 * @brief slot called by a timer created by itself or by the onRescueButton
 * @param start true if the rescue timer is to be started
 */
void UploaderGadgetWidget::onRescueTimer(bool start)
{
    static int progress;
    static QTimer timer;
    if((sender() == usbFilterBL) || (sender() == usbFilterUP))
    {
        timer.stop();
        m_widget->progressBar->setValue(0);
        disconnect(usbFilterBL, SIGNAL(deviceDiscovered()), this, SLOT(onRescueTimer()));
        disconnect(usbFilterUP, SIGNAL(deviceDiscovered()), this, SLOT(onRescueTimer()));
        rescueFinish(true);
        return;
    }
    if(start)
    {
        setStatusInfo(tr("Trying to detect bootloader"), uploader::STATUSICON_INFO);
        progress = 100;
        connect(&timer, SIGNAL(timeout()), this, SLOT(onRescueTimer()),Qt::UniqueConnection);
        timer.start(200);
        connect(usbFilterBL, SIGNAL(deviceDiscovered()), this, SLOT(onRescueTimer()), Qt::UniqueConnection);
        connect(usbFilterUP, SIGNAL(deviceDiscovered()), this, SLOT(onRescueTimer()), Qt::UniqueConnection);
        emit rescueTimer(0);
    }
    else
    {
        progress -= 1;
        emit rescueTimer(100 - progress);
    }
    m_widget->progressBar->setValue(progress);
    if(progress == 0)
    {
        disconnect(usbFilterBL, SIGNAL(deviceDiscovered()), this, SLOT(onRescueTimer()));
        disconnect(usbFilterUP, SIGNAL(deviceDiscovered()), this, SLOT(onRescueTimer()));
        timer.disconnect();
        setStatusInfo(tr("Failed to detect bootloader"), uploader::STATUSICON_FAIL);
        setUploaderStatus(uploader::DISCONNECTED);
        emit rescueFinish(false);
        conMngr->resumePolling();
    }
}

/**
 * @brief slot called by the DFUObject to update operations status
 * @param text text corresponding to the status
 * @param progress 0 to 100 corresponding the the current operation progress
 */
void UploaderGadgetWidget::onStatusUpdate(QString text, int progress)
{
    if(!text.isEmpty())
        setStatusInfo(text, uploader::STATUSICON_RUNNING);
    if(progress != -1)
        m_widget->progressBar->setValue(progress);
    if( (getUploaderStatus() == uploader::UPLOADING_FW) || (getUploaderStatus() == uploader::UPLOADING_DESC) )
    {
        emit uploadProgress(getUploaderStatus(), progress);
    }
}

/**
 * @brief slot called the DFUObject when an upload operation finishes
 * @param stat upload result
 */
void UploaderGadgetWidget::onUploadFinish(Status stat)
{
    switch (uploaderStatus) {
    case uploader::UPLOADING_FW:
        if(stat == Last_operation_Success)
        {
            setStatusInfo(tr("Firmware upload success"), uploader::STATUSICON_OK);
            if (loadedFile.right(100).startsWith("TlFw") || loadedFile.right(100).startsWith("OpFw")) {
                tempArray.clear();
                tempArray.append(loadedFile.right(100));
                tempArray.chop(20);
                QString user("                    ");
                user = user.replace(0, m_widget->userDefined_LD_lbl->text().length(), m_widget->userDefined_LD_lbl->text());
                tempArray.append(user.toLatin1());
                setStatusInfo(tr("Starting firmware metadata upload"), uploader::STATUSICON_INFO);
                dfu.UploadPartitionThreaded(tempArray, DFU_PARTITION_DESC, 100);
                setUploaderStatus(uploader::UPLOADING_DESC);
            }
            else
            {
                setUploaderStatus(previousStatus);
                dfu.disconnect();
            }
        }
        else
        {
            setUploaderStatus(previousStatus);
            setStatusInfo(tr("Firmware upload failed"), uploader::STATUSICON_FAIL);
            dfu.disconnect();
            lastUploadResult = false;
            uploadFinish(false);
        }
        break;
    case uploader::UPLOADING_DESC:
        if(stat == Last_operation_Success)
        {
            setStatusInfo(tr("Firmware and firmware metadata upload success"), uploader::STATUSICON_OK);
            lastUploadResult = true;
            // uploaded succeeded so we can assume the loaded file is on the board
            deviceDescriptorStruct descStructure;
            if (UAVObjectUtilManager::descriptionToStructure(tempArray, descStructure)) {
                quint32 crc = dfu.CRCFromQBArray(loadedFile, currentBoard.max_code_size.toLong());
                FirmwareOnDeviceUpdate(descStructure, QString::number(crc));
            }
            this->activateWindow();
            m_widget->bootButton->setFocus();
            emit uploadFinish(true);
        }
        else
        {
            setStatusInfo(tr("Firmware metadata upload failed"), uploader::STATUSICON_FAIL);
            lastUploadResult = false;
            uploadFinish(false);
        }
        dfu.disconnect();
        setUploaderStatus(previousStatus);
        break;
    case uploader::UPLOADING_PARTITION:
        if(stat == Last_operation_Success)
            setStatusInfo(tr("Partition upload success"), uploader::STATUSICON_OK);
        else
            setStatusInfo(tr("Partition upload failed"), uploader::STATUSICON_FAIL);
        dfu.disconnect();
        setUploaderStatus(previousStatus);
        break;
    default:
        break;
    }
}

/**
 * @brief slot called the DFUObject when a download operation finishes
 * @param result true if the download was successfull
 */
void UploaderGadgetWidget::onDownloadFinish(bool result)
{
    switch (uploaderStatus) {
    case uploader::DOWNLOADING_PARTITION:
        dfu.disconnect();
        if(result)
        {
            setStatusInfo(tr("Partition download success"), uploader::STATUSICON_OK);
            QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),QDir::homePath(),"*.bin");
            if(filename.isEmpty())
            {
                setStatusInfo(tr("Error, empty filename"), uploader::STATUSICON_FAIL);
                setUploaderStatus(previousStatus);
                return;
            }

            if(!filename.endsWith(".bin",Qt::CaseInsensitive))
                filename.append(".bin");
            QFile file(filename);
            if(file.open(QIODevice::WriteOnly))
            {
                file.write(tempArray);
                file.close();
                setStatusInfo(tr("Partition written to file"), uploader::STATUSICON_OK);
            }
            else
                setStatusInfo(tr("Error could not open file for save"), uploader::STATUSICON_FAIL);
        }
        else
            setStatusInfo(tr("Partition download failed"), uploader::STATUSICON_FAIL);
        setUploaderStatus(previousStatus);
        break;
    default:
        break;
    }

}

/**
 * @brief slot called when the user clicks save on the partition browser
 */
void UploaderGadgetWidget::onPartitionSave()
{
    if(!CheckInBootloaderState())
        return;
    int index = m_widget->partitionBrowserTW->selectedItems().first()->row();
    int size = m_widget->partitionBrowserTW->item(index,1)->text().toInt();

    setStatusInfo("",uploader::STATUSICON_RUNNING);
    previousStatus = uploaderStatus;
    setUploaderStatus(uploader::DOWNLOADING_PARTITION);
    connect(&dfu, SIGNAL(operationProgress(QString,int)), this, SLOT(onStatusUpdate(QString, int)));
    connect(&dfu, SIGNAL(downloadFinished(bool)), this, SLOT(onDownloadFinish(bool)));
    tempArray.clear();
    dfu.DownloadPartitionThreaded(&tempArray, (dfu_partition_label)index, size);
}

/**
 * @brief slot called when the user clicks flash on the partition browser
 */
void UploaderGadgetWidget::onPartitionFlash()
{
    if(!CheckInBootloaderState())
        return;
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),QDir::homePath(),"*.bin");
    if(filename.isEmpty())
    {
        setStatusInfo(tr("Error, empty filename"), uploader::STATUSICON_FAIL);
        return;
    }
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        setStatusInfo(tr("Could not open file for read"), uploader::STATUSICON_FAIL);
        return;
    }
    tempArray.clear();
    tempArray = file.readAll();

    int index = m_widget->partitionBrowserTW->selectedItems().first()->row();
    int size = m_widget->partitionBrowserTW->item(index,1)->text().toInt();

    if(tempArray.length() > size)
    {
        setStatusInfo(tr("File bigger than partition size"), uploader::STATUSICON_FAIL);
    }
    if(QMessageBox::warning(this, tr("Warning"), tr("Are you sure you want to flash the selected partition?"),QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
        return;
    setStatusInfo("",uploader::STATUSICON_RUNNING);
    previousStatus = uploaderStatus;
    setUploaderStatus(uploader::UPLOADING_PARTITION);
    connect(&dfu, SIGNAL(operationProgress(QString,int)), this, SLOT(onStatusUpdate(QString, int)));
    connect(&dfu, SIGNAL(uploadFinished(tl_dfu::Status)), this, SLOT(onUploadFinish(tl_dfu::Status)));
    dfu.UploadPartitionThreaded(tempArray, (dfu_partition_label)index, size);
}

/**
 * @brief slot called when the user clicks erase on the partition browser
 */
void UploaderGadgetWidget::onPartitionErase()
{
    int index = m_widget->partitionBrowserTW->selectedItems().first()->row();
    if(QMessageBox::warning(this, tr("Warning"), tr("Are you sure you want erase the selected partition?"),QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
        return;
    if(dfu.WipePartition((dfu_partition_label)index))
        setStatusInfo(tr("Partition erased"), uploader::STATUSICON_OK);
    else
        setStatusInfo(tr("Partition erasure failed"), uploader::STATUSICON_FAIL);
}

/**
 * @brief slot called when the user clicks the boot button
 * Atempts to Boot the board and starts a timeout timer
 */
void UploaderGadgetWidget::onBootButtonClick()
{
    bool safeboot = (sender() == m_widget->safeBootButton);

    if (telMngr->isConnected()) {
        haltOrReset(false);
        return;
    }

    if(!CheckInBootloaderState())
        return;

    dfu.JumpToApp(safeboot);
    dfu.CloseBootloaderComs();
}

/**
 * @brief Checks if all conditions are met for the autopilot to be ready for required operations
 */
void UploaderGadgetWidget::CheckAutopilotReady()
{
    if(telemetryConnected && iapPresent && iapUpdated)
    {
        onAutopilotReady();
        utilMngr->versionMatchCheck();
    }
}

/**
 * @brief Check if the board is in bootloader state
 * @return true if in bootloader state
 */
bool UploaderGadgetWidget::CheckInBootloaderState()
{
    if( (uploaderStatus != uploader::BL_FROM_HALT) && (uploaderStatus != uploader::BL_FROM_RESCUE) )
    {
        setStatusInfo(tr("Cannot perform the selected operation if not in bootloader state"), uploader::STATUSICON_FAIL);
        return false;
    }
    else
        return true;
}

/**
 *Opens an open file dialog pointing to the file which corresponds to the connected board if possible
 */
QString UploaderGadgetWidget::LoadFirmwareFileDialog(QString boardName)
{
    QFileDialog::Options options;
    QString selectedFilter;
    boardName = boardName.toLower();

    QString fwDirectoryStr = getFirmwarePathForBoard(boardName);

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select firmware file"),
                                                    fwDirectoryStr,
                                                    tr("Firmware (fw_*.tlfw);;"
                                                       "Bootloader Update (bu_*.tlfw);;"
                                                       "All (*.tlfw *.opfw *.bin)"),
                                                    &selectedFilter,
                                                    options);
    return fileName;
}

/**
 * @brief Updates the status message
 * @param str string containing the text which describes the current status
 * @param ic icon which describes the current status
 */
void UploaderGadgetWidget::setStatusInfo(QString str, uploader::StatusIcon ic)
{
    QPixmap px;
    m_widget->statusLabel->setText(str);
    switch (ic) {
    case uploader::STATUSICON_RUNNING:
        px.load(QString(":/uploader/images/system-run.svg"));
        break;
    case uploader::STATUSICON_OK:
        px.load(QString(":/uploader/images/dialog-apply.svg"));
        break;
    case uploader::STATUSICON_FAIL:
        px.load(QString(":/uploader/images/process-stop.svg"));
        break;
    default:
        px.load(QString(":/uploader/images/gtk-info.svg"));
    }
    m_widget->statusPic->setPixmap(px);
}

/**
 * @brief slot by connectionManager when new devices arrive
 * Used to reconnect the boards after booting if autoconnect is disabled
 */
void UploaderGadgetWidget::onAvailableDevicesChanged(QLinkedList<Core::DevListItem> devList)
{
    if(conMngr->getAutoconnect() || conMngr->isConnected() || lastConnectedTelemetryDevice.isEmpty())
        return;
    foreach (Core::DevListItem item, devList) {
        if(item.device.data()->getName() == lastConnectedTelemetryDevice)
        {
            conMngr->connectDevice(item);
        }
    }
}

/**
 * @brief Returns the current uploader status
 */
uploader::UploaderStatus UploaderGadgetWidget::getUploaderStatus() const
{
    return uploaderStatus;
}

/**
 * @brief Sets the current uploader status
 * Enables, disables, hides, unhides widgets according to the new status
 * @param value value of the new status
 */
void UploaderGadgetWidget::setUploaderStatus(const uploader::UploaderStatus &value)
{
    uploaderStatus = value;
    switch (uploaderStatus) {
    case uploader::DISCONNECTED:
        m_widget->progressBar->setVisible(false);

        m_widget->rescueButton->setEnabled(true);
        m_widget->rescueButton->setText(tr("Rescue"));
        m_widget->bootButton->setText(tr("Boot"));

        m_widget->openButton->setEnabled(true);
        m_widget->bootButton->setEnabled(false);
        m_widget->safeBootButton->setEnabled(false);
        m_widget->flashButton->setEnabled(false);
        m_widget->exportConfigButton->setEnabled(false);
        m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::NoContextMenu);
        break;
    case uploader::HALTING:
        m_widget->progressBar->setVisible(false);

        m_widget->rescueButton->setText(tr("Enter bootloader"));
        m_widget->bootButton->setText(tr("Boot"));

        m_widget->rescueButton->setEnabled(false);
        m_widget->openButton->setEnabled(true);
        m_widget->bootButton->setEnabled(false);
        m_widget->safeBootButton->setEnabled(false);
        m_widget->flashButton->setEnabled(false);
        m_widget->exportConfigButton->setEnabled(false);
        m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::NoContextMenu);
        break;
    case uploader::RESCUING:
        m_widget->progressBar->setVisible(true);

        m_widget->rescueButton->setText(tr("Rescue"));
        m_widget->bootButton->setText(tr("Boot"));

        m_widget->rescueButton->setEnabled(false);
        m_widget->openButton->setEnabled(true);
        m_widget->bootButton->setEnabled(false);
        m_widget->safeBootButton->setEnabled(false);
        m_widget->flashButton->setEnabled(false);
        m_widget->exportConfigButton->setEnabled(false);
        m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::NoContextMenu);
        break;
    case uploader::BL_FROM_HALT:
    case uploader::BL_FROM_RESCUE:
        m_widget->progressBar->setVisible(false);

        m_widget->rescueButton->setText(tr("Rescue"));
        m_widget->bootButton->setText(tr("Boot"));

        m_widget->rescueButton->setEnabled(false);
        m_widget->openButton->setEnabled(true);
        m_widget->bootButton->setEnabled(true);
        m_widget->safeBootButton->setEnabled(true);
        if(!loadedFile.isEmpty())
            m_widget->flashButton->setEnabled(true);
        else
            m_widget->flashButton->setEnabled(false);

        // XXX: needs to be conditional on presence of setting partition
        m_widget->exportConfigButton->setEnabled(true);

        m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::ActionsContextMenu);
        break;
    case uploader::CONNECTED_TO_TELEMETRY:
        m_widget->progressBar->setVisible(false);

        m_widget->rescueButton->setText(tr("Enter bootloader"));
        m_widget->bootButton->setText(tr("Reboot"));

        m_widget->rescueButton->setEnabled(true);
        m_widget->openButton->setEnabled(true);
        m_widget->bootButton->setEnabled(true);
        m_widget->safeBootButton->setEnabled(false);
        m_widget->flashButton->setEnabled(false);
        m_widget->exportConfigButton->setEnabled(true);

        m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::NoContextMenu);
        break;
    case uploader::UPLOADING_FW:
    case uploader::UPLOADING_DESC:
    case uploader::DOWNLOADING_PARTITION:
    case uploader::UPLOADING_PARTITION:
    case uploader::DOWNLOADING_PARTITION_BUNDLE:
    case uploader::UPLOADING_PARTITION_BUNDLE:
        m_widget->progressBar->setVisible(true);

        m_widget->rescueButton->setEnabled(false);
        m_widget->openButton->setEnabled(false);
        m_widget->bootButton->setEnabled(false);
        m_widget->safeBootButton->setEnabled(false);
        m_widget->flashButton->setEnabled(false);
        m_widget->exportConfigButton->setEnabled(false);
        m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::NoContextMenu);
        break;
    case uploader::BOOTING:
        m_widget->progressBar->setVisible(false);

        m_widget->rescueButton->setText(tr("Enter bootloader"));
        m_widget->bootButton->setText(tr("Reboot"));

        m_widget->openButton->setEnabled(false);
        m_widget->bootButton->setEnabled(false);
        m_widget->safeBootButton->setEnabled(false);
        m_widget->flashButton->setEnabled(false);
        m_widget->exportConfigButton->setEnabled(false);
        m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::NoContextMenu);
    default:
        break;
    }
}

//! Check that we can find the board firmware
bool UploaderGadgetWidget::autoUpdateCapable()
{
    QString board;
    if(currentBoard.board)
        board = currentBoard.board->shortName().toLower();
    return QDir(QFileInfo(getFirmwarePathForBoard(board)).absolutePath()).exists();
}

/**
 * @brief Slot indirecly called by the wizzard plugin to autoupdate the board with the
 * embedded firmware
 * @return true if the update was successful
 */
bool UploaderGadgetWidget::autoUpdate()
{
    Core::BoardManager* brdMgr = Core::ICore::instance()->boardManager();
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    bool stillLooking = true;
    while(stillLooking)
    {
        stillLooking = false;
        foreach(int vendorID, brdMgr->getKnownVendorIDs()) {
            stillLooking |= (USBMonitor::instance()->availableDevices(vendorID,-1,-1,-1).length()>0);
        }
        emit autoUpdateSignal(WAITING_DISCONNECT,QVariant());
        QTimer::singleShot(500, &loop, SLOT(quit()));
        loop.exec();
    }
    emit autoUpdateSignal(WAITING_CONNECT,0);
    connect(this, SIGNAL(rescueTimer(int)), this, SLOT(onAutoUpdateCount(int)));
    connect(this, SIGNAL(rescueFinish(bool)), &loop, SLOT(quit()));
    onRescueButtonClick();
    loop.exec();
    disconnect(this, SIGNAL(rescueTimer(int)), this, SLOT(onAutoUpdateCount(int)));
    disconnect(this, SIGNAL(rescueFinish(bool)), &loop, SLOT(quit()));
    if(getUploaderStatus() != uploader::BL_FROM_RESCUE)
    {
        emit autoUpdateSignal(FAILURE,QVariant());
        return false;
    }

    // Find firmware file in resources based on board name
    QString m_filename;
    QString board;
    if(currentBoard.board)
        board = currentBoard.board->shortName().toLower();
    else
    {
        emit autoUpdateSignal(FAILURE,QVariant());
        return false;
    }
    m_filename = getFirmwarePathForBoard(board);
    if(!QFile::exists(m_filename))
    {
        emit autoUpdateSignal(FAILURE_FILENOTFOUND,QVariant());
        return false;
    }
    QFile m_file(m_filename);
    if (!m_file.open(QIODevice::ReadOnly)) {
        emit autoUpdateSignal(FAILURE,QVariant());
        return false;
    }
    loadedFile = m_file.readAll();
    FirmwareLoadedClear(true);
    FirmwareLoadedUpdate(loadedFile);
    setUploaderStatus(getUploaderStatus());

    onFlashButtonClick();

    connect(this, SIGNAL(uploadProgress(UploaderStatus,QVariant)), this, SIGNAL(autoUpdateSignal(UploaderStatus,QVariant)));
    connect(this, SIGNAL(uploadFinish(bool)), &loop, SLOT(quit()));
    loop.exec();
    disconnect(this, SIGNAL(uploadProgress(UploaderStatus,QVariant)), this, SIGNAL(autoUpdateSignal(UploaderStatus,QVariant)));
    disconnect(this, SIGNAL(uploadFinish(bool)), &loop, SLOT(quit()));
    onBootButtonClick();
    if(!lastUploadResult)
    {
        emit autoUpdateSignal(FAILURE, QVariant());
        return false;
    }
    emit autoUpdateSignal(SUCCESS, QVariant());
    return true;
}

/**
 * @brief Adapts uploader logic to wizzard plugin logic
 */
void UploaderGadgetWidget::onAutoUpdateCount(int i)
{
    emit autoUpdateSignal(WAITING_CONNECT, i);
}

/**
 * @brief Opens the plugin help page on the default browser
 * TODO ADD SPECIFIC NG PAGE TO THE WIKI
 */
void UploaderGadgetWidget::openHelp()
{
    QDesktopServices::openUrl( QUrl("https://github.com/d-ronin/dRonin/wiki/OnlineHelp:-Uploader-Plugin", QUrl::StrictMode) );
}

QString UploaderGadgetWidget::getFirmwarePathForBoard(QString(boardName))
{
    QDir fwDirectory;
    QString fwDirectoryStr;
    boardName = boardName.toLower();

#ifdef Q_OS_WIN
    #ifdef FIRMWARE_RELEASE_CONFIG
        fwDirectoryStr = QCoreApplication::applicationDirPath();
        fwDirectory = QDir(fwDirectoryStr);
        fwDirectory.cdUp();
        fwDirectory.cd("firmware");
        fwDirectoryStr = fwDirectory.absolutePath();
    #else
        fwDirectoryStr = QCoreApplication::applicationDirPath();
        fwDirectory = QDir(fwDirectoryStr);
        fwDirectory.cd("../../..");
        fwDirectoryStr = fwDirectory.absolutePath();
        fwDirectoryStr += "/fw_" + boardName;
    #endif // FIRMWARE_RELEASE_CONFIG
#elif defined Q_OS_LINUX
    #ifdef FIRMWARE_RELEASE_CONFIG
        fwDirectory = QDir("/usr/local/" GCS_PROJECT_BRANDING_PRETTY "/firmware");
        fwDirectoryStr = fwDirectory.absolutePath();
    #else
        fwDirectoryStr = QCoreApplication::applicationDirPath();
        fwDirectory = QDir(fwDirectoryStr);
        fwDirectory.cd("../../..");
        fwDirectoryStr = fwDirectory.absolutePath();
        fwDirectoryStr += "/fw_" + boardName;
    #endif // FIRMWARE_RELEASE_CONFIG
    fwDirectoryStr += "/fw_" + boardName + ".tlfw";
#elif defined Q_OS_MAC
    #ifdef FIRMWARE_RELEASE_CONFIG
        fwDirectoryStr = QCoreApplication::applicationDirPath();
        fwDirectory = QDir(fwDirectoryStr);
        fwDirectory.cd("../Resources/firmware");
        fwDirectoryStr = fwDirectory.absolutePath();
    #else
        fwDirectoryStr = QCoreApplication::applicationDirPath();
        fwDirectory = QDir(fwDirectoryStr);
        fwDirectory.cd("../../../../../..");
        fwDirectoryStr = fwDirectory.absolutePath();
        fwDirectoryStr += "/fw_" + boardName;
    #endif // FIRMWARE_RELEASE_CONFIG
    fwDirectoryStr += "/fw_"+ boardName +".tlfw";
#endif

    return fwDirectoryStr;
}

bool UploaderGadgetWidget::FirmwareLoadFromFile(QString filename)
{
    return FirmwareLoadFromFile(QFileInfo(filename));
}

bool UploaderGadgetWidget::FirmwareLoadFromFile(QFileInfo filename)
{
    if (!filename.exists())
        return false;

    QFile file(filename.filePath());
    if(!file.open(QIODevice::ReadOnly))
        return false;
    loadedFile = file.readAll();

    FirmwareLoadedClear(true);
    FirmwareLoadedUpdate(loadedFile);
    setUploaderStatus(getUploaderStatus());

    return true;
}

bool UploaderGadgetWidget::FirmwareCheckForUpdate(deviceDescriptorStruct device)
{
    const QString gcsRev(GCS_REVISION);
    if (gcsRev.contains(':')) {
        QString gcsShort = gcsRev.mid(gcsRev.indexOf(':') + 1, 8);
        if ((gcsShort != device.gitHash) && (ignoredRev != gcsShort)) {
            QMessageBox msgBox;
            msgBox.setText(tr("The firmware version on your board does not match this version of GCS."));
            msgBox.setInformativeText(tr("Do you want to upgrade the firmware to a compatible version?"));
            msgBox.setDetailedText(QString("Firmware git hash: %1\nGCS git hash: %2").arg(device.gitHash).arg(gcsShort));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Ignore);
            msgBox.setDefaultButton(QMessageBox::Yes);

            int val = msgBox.exec();

            if (val == QMessageBox::Yes)
                return true;
            else if (val == QMessageBox::Ignore)
                ignoredRev = gcsShort;
        }
    }
    return false;
}
