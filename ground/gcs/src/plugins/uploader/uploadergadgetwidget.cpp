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
#include <QHttpPart>
#include <QHttpMultiPart>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include "uploadergadgetwidget.h"
#include "firmwareiapobj.h"
#include "coreplugin/icore.h"
#include <coreplugin/modemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include "rawhid/rawhidplugin.h"
#include "../../../../../build/ground/gcs/gcsversioninfo.h"

using namespace uploader;

/**
 * @brief Class constructor, sets signal to slot connections,
 * creates actions, creates utility classes instances, etc
 */
UploaderGadgetWidget::UploaderGadgetWidget(QWidget *parent):QWidget(parent),
    telemetryConnected(false), iapUpdated(false)
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
    importMngr =  pm->getObject<UAVSettingsImportExportManager>();

    netMngr = new QNetworkAccessManager(this);

    UAVObjectManager *obm = pm->getObject<UAVObjectManager>();
    connect(telMngr, SIGNAL(connected()), this, SLOT(onAutopilotConnect()));
    connect(telMngr, SIGNAL(disconnected()), this, SLOT(onAutopilotDisconnect()));
    firmwareIap = FirmwareIAPObj::GetInstance(obm);

    connect(firmwareIap, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(onIAPUpdated()), Qt::UniqueConnection);

    //Connect button signals to slots
    connect(m_widget->openButton, SIGNAL(clicked()), this, SLOT(onLoadFirmwareButtonClick()));
    connect(m_widget->rescueButton, SIGNAL(clicked()), this, SLOT(onRescueButtonClick()));
    connect(m_widget->flashButton, SIGNAL(clicked()), this, SLOT(onFlashButtonClick()));
    connect(m_widget->bootButton, SIGNAL(clicked()), this, SLOT(onBootButtonClick()));
    connect(m_widget->safeBootButton, SIGNAL(clicked()), this, SLOT(onBootButtonClick()));
    connect(m_widget->exportConfigButton, SIGNAL(clicked()), this, SLOT(onExportButtonClick()));

    connect(m_widget->pbHelp, SIGNAL(clicked()),this,SLOT(openHelp()));
    Core::BoardManager* brdMgr = Core::ICore::instance()->boardManager();

    //Setup usb discovery signals for boards in bl state
    usbFilterBL = new USBSignalFilter(brdMgr->getKnownVendorIDs(),-1,-1,USBMonitor::Bootloader);
    usbFilterUP = new USBSignalFilter(brdMgr->getKnownVendorIDs(),-1,-1,USBMonitor::Upgrader);

    connect(usbFilterBL, SIGNAL(deviceRemoved()), this, SLOT(onBootloaderRemoved()), Qt::QueuedConnection);

    connect(usbFilterBL, SIGNAL(deviceDiscovered()), this, SLOT(onBootloaderDetected()), Qt::QueuedConnection);

    connect(usbFilterUP, SIGNAL(deviceRemoved()), this, SLOT(onBootloaderRemoved()), Qt::QueuedConnection);

    connect(usbFilterUP, SIGNAL(deviceDiscovered()), this, SLOT(onBootloaderDetected()), Qt::QueuedConnection);

    connect(&dfu, SIGNAL(operationProgress(QString,int)), this, SLOT(onStatusUpdate(QString, int)));

    conMngr = Core::ICore::instance()->connectionManager();

    /* Enter the loader if it's available */
    setUploaderStatus(uploader::ENTERING_LOADER);
    onBootloaderDetected();

    /* Else begin our normal actions waitin' for a board */
    if (getUploaderStatus() != uploader::BL_SITTING) {
        setUploaderStatus(uploader::DISCONNECTED);
        setStatusInfo(tr("GCS ready for flight board connection..."), uploader::STATUSICON_OK);
    }
}

/**
 * @brief Class destructor, nothing needs to be destructed ATM
 */
UploaderGadgetWidget::~UploaderGadgetWidget()
{

}

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
    iapUpdated = false;
    if(getUploaderStatus() == uploader::ENTERING_LOADER)
        return;
    if(getUploaderStatus() == uploader::UPGRADING_CATCHLOADER)
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
            onRescueButtonClick();

            Core::ModeManager::instance()->activateModeByWorkspaceName("Firmware");
            // Set the status, so when we enter the loader we cue the
            // upgrade state machine/actions.
            setUploaderStatus(uploader::UPGRADING);
        }
    }
    emit newBoardSeen(board, device);
}

/**
 * @brief slot called by the iap object when it is updated
 */
void UploaderGadgetWidget::onIAPUpdated()
{
    if ((getUploaderStatus() == uploader::ENTERING_LOADER) ||
            (getUploaderStatus() == uploader::UPGRADING))
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
        setStatusInfo(tr("Open cancelled"), uploader::STATUSICON_FAIL);
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

// Would be nice to do the work here required to constify firmwareImage
bool UploaderGadgetWidget::flashFirmware(QByteArray &firmwareImage)
{
    QEventLoop loop;

    QTimer timeout;

    connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));

    timeout.setSingleShot(true);
    timeout.start(120000);      // Very long out of caution. Current Brain
                                // FW takes 36-37 seconds on some machines to
                                // flash.

    tl_dfu::Status operationSuccess = DFUidle;

    setStatusInfo("",uploader::STATUSICON_RUNNING);
    setUploaderStatus(uploader::BL_BUSY);
    onStatusUpdate(QString("Starting upload..."), 0); // set progress bar to 0 while erasing
    dfu.UploadPartitionThreaded(firmwareImage, DFU_PARTITION_FW, currentBoard.max_code_size.toInt());

    /* disconnects when loop comes out of scope */
    connect(&dfu, &DFUObject::uploadFinished, &loop, [&] (tl_dfu::Status status) {
        operationSuccess = status;
        loop.exit();
    } );

    loop.exec();                /* Wait for timeout or download complete */

    timeout.stop();

    if (operationSuccess != Last_operation_Success) {
        setUploaderStatus(uploader::BL_SITTING);
        setStatusInfo(tr("Firmware upload failed"), uploader::STATUSICON_FAIL);

        return false;
    }

    setStatusInfo(tr("Firmware upload success"), uploader::STATUSICON_OK);

    if ((!firmwareImage.right(100).startsWith("TlFw")) && (!firmwareImage.right(100).startsWith("OpFw"))) {
        setUploaderStatus(uploader::BL_SITTING);

        return true;
    }

    tempArray.clear();
    tempArray.append(firmwareImage.right(100));
    tempArray.chop(20);
    QString user("                    ");
    user = user.replace(0, m_widget->userDefined_LD_lbl->text().length(), m_widget->userDefined_LD_lbl->text());
    tempArray.append(user.toLatin1());
    setStatusInfo(tr("Starting firmware metadata upload"), uploader::STATUSICON_INFO);
    dfu.UploadPartitionThreaded(tempArray, DFU_PARTITION_DESC, 100);

    operationSuccess = DFUidle;

    timeout.start(10000);
    loop.exec();
    timeout.stop();

    if(operationSuccess != Last_operation_Success) {
            setStatusInfo(tr("Firmware metadata upload failed"), uploader::STATUSICON_FAIL);

            setUploaderStatus(uploader::BL_SITTING);

            return false;
    }

    setUploaderStatus(uploader::BL_SITTING);
    setStatusInfo(tr("Firmware and firmware metadata upload success"), uploader::STATUSICON_OK);

    // uploaded succeeded so we can assume the loaded file is on the board
    deviceDescriptorStruct descStructure;
    if (UAVObjectUtilManager::descriptionToStructure(tempArray, descStructure)) {
        quint32 crc = dfu.CRCFromQBArray(firmwareImage, currentBoard.max_code_size.toLong());
        FirmwareOnDeviceUpdate(descStructure, QString::number(crc));
    }
    setUploaderStatus(uploader::BL_SITTING);

    return true;
}

/**
 * @brief slot called when the user presses the flash firmware button
 * It start a non blocking firmware upload
 */
void UploaderGadgetWidget::onFlashButtonClick()
{
    if (flashFirmware(loadedFile)) {
        this->activateWindow();
        m_widget->bootButton->setFocus();
    }
}

void UploaderGadgetWidget::haltOrReset(bool halting)
{
    if(!firmwareIap->getIsPresentOnHardware())
        return;

    if (halting) {
        setUploaderStatus(uploader::ENTERING_LOADER);
    } else {
        setUploaderStatus(uploader::DISCONNECTED);
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
        timeout.stop();
        conMngr->suspendPolling();
        onRescueTimer(true);
    } else {
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
    setUploaderStatus(uploader::ENTERING_LOADER);
    setStatusInfo(tr("Please connect the board with USB with no external power applied"), uploader::STATUSICON_INFO);
    onRescueTimer(true);
}

bool UploaderGadgetWidget::downloadSettings() {
    QEventLoop loop;

    QTimer timeout;
    timeout.setSingleShot(true);

    bool operationSuccess = false;

    connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));

    /* disconnects when loop comes out of scope */
    connect(&dfu, &DFUObject::downloadFinished, &loop, [&] (bool status) {
        qDebug() << "downloadFinished in dSettings: " << status;
        operationSuccess = status;
        loop.exit();
    } );

    timeout.start(100000);       /* 100 secs is a long time; unfortunately
                                  * revo settings part is HUUUUGE and takes
                                  * forever to download. */

    triggerPartitionDownload(DFU_PARTITION_SETTINGS);
    loop.exec();                /* Wait for timeout or download complete */
    timeout.stop();

    return operationSuccess;
}

bool UploaderGadgetWidget::askIfShouldContinue() {
    QMessageBox msgBox;
    msgBox.setText(tr("Proceed without saving a configuration backup?"));
    msgBox.setInformativeText(tr("It is strongly encouraged that you save a backup of the configuration before proceeding with upgrade."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    int val = msgBox.exec();

    if (val == QMessageBox::Yes) {
        return true;
    }

    return false;
}

bool UploaderGadgetWidget::saveSettings(const QByteArray &settingsDump) {
    /* save dialog for XML config */
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Settings Backup"),"cloud_exported.uav","*.uav");
    if(filename.isEmpty())
    {
        setStatusInfo(tr("Save cancelled."), uploader::STATUSICON_FAIL);
        setUploaderStatus(uploader::BL_SITTING);
        return false;
    }

    if(!filename.endsWith(".uav",Qt::CaseInsensitive))
        filename.append(".uav");

    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly))
    {
        setStatusInfo(tr("Error could not open file for save"), uploader::STATUSICON_FAIL);

        setUploaderStatus(uploader::BL_SITTING);
        return false;
    }

    file.write(settingsDump);
    file.close();
    setStatusInfo(tr("Dump of configuration saved to file!"), uploader::STATUSICON_OK);

    return true;
}

// Returns negative (<0) if we can't get to the cloud.
// Returns false (0) if the cloud doesn't have our release
// Returns true (>0) if the cloud has our release and would be glad to
// upgrade us.
int UploaderGadgetWidget::isCloudReleaseAvailable(QString srcRelease) {
    QUrl url(hasRevUrl.arg(srcRelease));
    QNetworkRequest request(url);

    QNetworkReply *reply = netMngr->head(request);

    QEventLoop loop;

    QTimer timeout;
    timeout.setSingleShot(true);

    connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    timeout.start(20000);       /* 20 seconds */
    loop.exec();

    timeout.stop();

    if (!reply->isFinished()) {
        setStatusInfo(tr("Timeout communicating with cloud service"), uploader::STATUSICON_FAIL);
        return -1;
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    int code = statusCode.toInt();

    if (code == 200) return 1;
    
    if (code == 404) return 0;

    return -1;          // Unexpected error code
}

bool UploaderGadgetWidget::tradeSettingsWithCloud(QString srcRelease,
        bool upgrading, QByteArray *settingsOut) {
    /* post to cloud service */
    QUrl url(exportUrl);
    QNetworkRequest request(url);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    if (settingsOut != NULL) {
        settingsOut->clear();
    }

    // ZLib compress the data
    QByteArray compressed = qCompress(tempArray, 8);

    // Remove the first 4 bytes, because Qt prepends expected length
    // (not compatible with zlib)
    compressed.remove(0, 4);

    QHttpPart githash, adaptTo, datafile;

    githash.setHeader(QNetworkRequest::ContentDispositionHeader,QVariant("form-data; name=\"githash\""));
    githash.setBody(srcRelease.toLatin1());

    const QString gcsRev(GCS_REVISION);
    adaptTo.setHeader(QNetworkRequest::ContentDispositionHeader,QVariant("form-data; name=\"adaptTo\""));
    adaptTo.setBody(gcsRev.toLatin1());

    datafile.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    datafile.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; filename=\"datafile\"; name=\"datafile\""));
    datafile.setBody(compressed);

    multiPart->append(githash);
    multiPart->append(datafile);

    QNetworkReply *reply = netMngr->post(request, multiPart);

    QEventLoop loop;

    QTimer timeout;
    timeout.setSingleShot(true);

    connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    timeout.start(20000);       /* 20 seconds */
    loop.exec();

    timeout.stop();

    if (!reply->isFinished()) {
        setStatusInfo(tr("Timeout communicating with cloud service"), uploader::STATUSICON_FAIL);
        return false;
    }

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    int code = statusCode.toInt();

    if (code != 200) {
        setStatusInfo(tr("Received status code %1 from cloud").arg(code), uploader::STATUSICON_FAIL);
        return false;
    }

    setStatusInfo(tr("Retrieved dump of configuration from cloud"), uploader::STATUSICON_OK);

    settingsDump = reply->readAll();

    if (settingsOut != NULL) {
        *settingsOut = settingsDump;
    }

    if (upgrading) {
        return true;
    }

    return saveSettings(settingsDump);
}

void UploaderGadgetWidget::upgradeError(QString why)
{
    Q_UNUSED(why);

    m_dialog.hide();

    qDebug() << "Upgrade assistant failed: " + why;

    setUploaderStatus(uploader::DISCONNECTED);

    /* Pop up a proper error dialog */
    QMessageBox msgBox;

    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(tr("Automatic upgrade failed."));
    msgBox.setInformativeText(why);

    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void UploaderGadgetWidget::stepChangeAndDelay(QEventLoop &loop, int delayMs,
        UpgradeAssistantDialog::UpgradeAssistantStep step)
{
    m_dialog.onStepChanged(step);

    QTimer delay;

    delay.setSingleShot(true);
    connect(&delay, SIGNAL(timeout()), &loop, SLOT(quit()));

    delay.start(delayMs);
    loop.exec();
    delay.stop();
}

void UploaderGadgetWidget::doUpgradeOperation(bool blankFC)
{
    Core::ModeManager::instance()->activateModeByWorkspaceName("Firmware");

    m_dialog.onStepChanged(UpgradeAssistantDialog::STEP_ENTERLOADER);
    m_dialog.setOperatingMode(true, true, blankFC);

    QEventLoop loop;
    QTimer timeout;

    timeout.setSingleShot(true);

    bool aborted = false;
    bool entLoader = false;

    int ret;

    struct deviceInfo board = currentBoard;

    connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(&m_dialog, &UpgradeAssistantDialog::finished, &loop, [&] (int result) {
        (void) result;
        aborted = true;
        loop.exit();
    } );

    connect(this, &UploaderGadgetWidget::enteredLoader, &loop, [&] () {
        entLoader = true;
        loop.exit();
    } );

    m_dialog.open();

    if (uploaderStatus != uploader::BL_SITTING) {
        upgradeError(tr("Not in expected bootloader state!"));
        return;
    }

    /* Save the version to convert from, so we can tell the cloud service */
    QString upgradingFrom = m_widget->gitHashOD_lbl->text();

    qDebug() << "Upgrading from " << upgradingFrom;

    /* Infer what operations we need to do -- first, do they need the upgrade
     * tool? */
    bool isCrippledBoard = board.board->queryCapabilities(Core::IBoardType::BOARD_DISABILITY_REQUIRESUPGRADER);

    bool upgradingLoader = false;

    if (!isCrippledBoard) {
        /* If no settings part known, new loader needed. */
        upgradingLoader = !haveSettingsPart();
    }

    m_dialog.setOperatingMode(upgradingLoader, isCrippledBoard, blankFC);

    if (!blankFC) {
        stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_CHECKCLOUD);

        int available = isCloudReleaseAvailable(upgradingFrom);

        if (available < 0) {
            // Cloud service missing.  Pop up a dialog and ask user what to
            // do.

            int val = QMessageBox::critical(this,
                    tr("Cloud upgrade service not available"),
                    tr("The cloud upgrade service is not reachable. "
                       "You may continue without migrating settings, "
                       "or cancel the upgrade."),
                    QMessageBox::Ok | QMessageBox::Cancel,
                    QMessageBox::Cancel);

            if (val != QMessageBox::Ok) {
                aborted = true;
            } else {
                blankFC = true;
            }
        } else if (available == 0) {
            // Cloud service doesn't know about source release.  Pop up a
            // dialog and ask what to do.

            int val = QMessageBox::critical(this,
                    tr("Cloud upgrade service: release unknown"),
                    tr("The cloud upgrade service does not know about "
                       "the firmware on the flight controller. "
                       "You may discard the existing settings, "
                       "cancel the upgrade, or ignore and attempt to "
                       "proceed with migration anyways."),
                    QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Ignore,
                    QMessageBox::Cancel);

            if (val == QMessageBox::Discard) {
                blankFC = true;
            } else if (val != QMessageBox::Ignore) {
                aborted = true;
            }
        }
    }

    // Set this again, as we could have decided to treat the board as blank.
    m_dialog.setOperatingMode(upgradingLoader, isCrippledBoard, blankFC);

    if (aborted) {
        upgradeError(tr("Aborted!"));

        return;
    }

    /* gather up bootupdater, legacy upgrader, and firmware images as needed,
     * so we spot errors before we do destructive things */

    QByteArray upgraderFile, bootUpdateFile, firmwareFile;

    if (isCrippledBoard) {
        if (!FirmwareLoadFromFile(getImagePath(board.board->shortName(), "up"), &upgraderFile)) {
            upgradeError(tr("Unable to load legacy upgrader image for board!"));

            return;
        }
    }

    if (upgradingLoader) {
        if (!FirmwareLoadFromFile(getImagePath(board.board->shortName(), "bu"), &bootUpdateFile)) {
            upgradeError(tr("Unable to load bootloader update image for board!"));

            return;
        }
    }

    if (!FirmwareLoadFromFile(getImagePath(board.board->shortName(), "fw"), &firmwareFile)) {
        upgradeError(tr("Unable to load firmware image for board!"));

        return;
    }

    if (upgradingLoader) {
        stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_UPGRADEBOOTLOADER);

        if (aborted) {
            upgradeError(tr("Aborted!"));

            return;
        }

        /* program the boot upgrader tool */
        FirmwareLoadedClear(true);
        FirmwareLoadedUpdate(bootUpdateFile);
        setUploaderStatus(getUploaderStatus());

        if (!flashFirmware(bootUpdateFile)) {
            upgradeError(tr("Unable to flash upgrader image to board!"));

            return;
        }

        entLoader = false;
        setUploaderStatus(uploader::UPGRADING_CATCHLOADER);
        dfu.JumpToApp(false);
        dfu.CloseBootloaderComs();

        /* Wait for / detect main loader */
        timeout.start(25000);
        loop.exec();
        timeout.stop();

        if (!entLoader) {
            upgradeError(tr("Unable to enter bootloader after bootloader upgrade!"));
        }
    }

    if (isCrippledBoard) {
        stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_PROGRAMUPGRADER);

        if (aborted) {
            upgradeError(tr("Aborted!"));

            return;
        }

        /* program the legacy upgrade tool */
        FirmwareLoadedClear(true);
        FirmwareLoadedUpdate(upgraderFile);
        setUploaderStatus(getUploaderStatus());

        if (!flashFirmware(upgraderFile)) {
            upgradeError(tr("Unable to flash upgrader image to board!"));

            return;
        }

        stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_ENTERUPGRADER);

        if (aborted) {
            upgradeError(tr("Aborted!"));

            return;
        }

        entLoader = false;
        setUploaderStatus(uploader::UPGRADING_CATCHLOADER);
        dfu.JumpToApp(false);
        dfu.CloseBootloaderComs();

        /* Wait for / detect main loader */
        timeout.start(25000);
        loop.exec();
        timeout.stop();

        if (!entLoader) {
            upgradeError(tr("Unable to enter legacy upgrade tool on board!!"));
        }
    }

    QByteArray xmlDump;

    if (!blankFC) {
        stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_DOWNLOADSETTINGS);

        if (aborted) {
            upgradeError(tr("Aborted!"));

            return;
        }

        /* download the settings partition from the board */
        if (!downloadSettings()) {
            upgradeError(tr("Unable to pull settings partition!"));

            return;
        }

        stepChangeAndDelay(loop, 100, UpgradeAssistantDialog::STEP_TRANSLATESETTINGS);

        if (aborted) {
            upgradeError(tr("Aborted!"));

            return;
        }

        /* translate the settings using the cloud service */
        if (!tradeSettingsWithCloud(upgradingFrom, true, &xmlDump)) {
            upgradeError(tr("Unable to use cloud services to translate settings!"));

            return;
        }

        bool done = false;

        while (!done) {
            if (aborted) {
                upgradeError(tr("Aborted!"));

                return;
            }

            ret = m_dialog.PromptUser(tr("Obtained settings from cloud."),
                    tr("It is recommended that you save a backup of the settings "
                        "before proceeding with upgrade."),
                    QStringList() << tr("Continue without saving")
                    << tr("Save settings backup"));

            switch (ret) {
                case 0: /* continue */
                    done = true;
                    break;
                case 1: /* save settings backup */
                    /* try to save. if successful, set done */
                    done = saveSettings(xmlDump);
                    break;
                default:
                    break;
            }
        }
    }

    stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_ERASESETTINGS);

    if (aborted) {
        upgradeError(tr("Aborted!"));

        return;
    }

    /* wipe the board setting partition in prepation of upgrade */
    if (!dfu.WipePartition(DFU_PARTITION_SETTINGS)) {
        upgradeError(tr("Failed to erase settings partition!"));
    }

    if (isCrippledBoard) {
        stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_REENTERLOADER);

        if (aborted) {
            upgradeError(tr("Aborted!"));

            return;
        }

        m_dialog.onStepChanged(UpgradeAssistantDialog::STEP_REENTERLOADER);
        /* re-enter the loader in preparation for flashing fw */

        entLoader = false;
        setUploaderStatus(uploader::UPGRADING_CATCHLOADER);
        dfu.ResetDevice();
        dfu.CloseBootloaderComs();

        /* Wait for / detect main loader */
        timeout.start(25000);
        loop.exec();
        timeout.stop();

        if (!entLoader) {
            upgradeError(tr("Unable to re-enter main bootloader on board!"));

            return;
        }
    }

    stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_FLASHFIRMWARE);

    if (aborted) {
        upgradeError(tr("Aborted!"));

        return;
    }

    /* flash the appropriate main firmware image */
    FirmwareLoadedClear(true);
    FirmwareLoadedUpdate(firmwareFile);
    setUploaderStatus(getUploaderStatus());

    loop.processEvents();
    if (aborted) {
        upgradeError(tr("Aborted!"));

        return;
    }

    if (!flashFirmware(firmwareFile)) {
        upgradeError(tr("Unable to flash firmware image to board!"));

        return;
    }

    if (aborted) {
        upgradeError(tr("Aborted!"));

        return;
    }

    /* Set ignoredrev to what we just programmed, so if we programmed
     * an outdated rev in a developer environment we don't immediately bop
     * up the message */
    ignoredRev = m_widget->gitHashOD_lbl->text();

    stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_BOOT);

    if (aborted) {
        upgradeError(tr("Aborted!"));

        return;
    }

    bool firmwareConnected = false;

    /* start firmware and wait for telemetry connection */
    connect(telMngr, &TelemetryManager::connected, &loop, [&] () {
        firmwareConnected = true;
        loop.exit();
    } );

    dfu.JumpToApp(false);
    dfu.CloseBootloaderComs();
    setUploaderStatus(uploader::DISCONNECTED);

    for (int tries = 0; tries < 2; tries++) {
        if (aborted) {
            upgradeError(tr("Aborted!"));

            return;
        }

        timeout.start(25000);

        if (!firmwareConnected) {
            loop.exec();
        }

        timeout.stop();

        if (aborted) {
            upgradeError(tr("Aborted!"));

            return;
        }

        if (!firmwareConnected) {
            if (tries == 0) {
                ret = m_dialog.PromptUser(
                        tr("Unable to automatically reset board"),
                        tr("Please remove power and USB from the flight board, "
                            " plug it back in, and select \"Continue.\""),
                        QStringList() << tr("Continue"));
            } else {
                upgradeError(tr("Unable to connect to new firmware!"));

                return;
            }
        } else {
            break;
        }
    }

    if (!blankFC) {
        stepChangeAndDelay(loop, 400, UpgradeAssistantDialog::STEP_IMPORT);

        while (true) {
            if (aborted) {
                upgradeError(tr("Aborted!"));

                return;
            }

            ret = m_dialog.PromptUser(
                    tr("Please review the imported settings and save to board."),
                    tr("No settings are currently saved. Select the "
                        "\"Review and Save\" button to import the settings."),
                    QStringList() << tr("Review and Save"));

            if (ret == 0) {
                if (importMngr->importUAVSettings(xmlDump, true)) {
                    break;
                }
            }
        }
    }

    m_dialog.onStepChanged(UpgradeAssistantDialog::STEP_DONE);

    while (!aborted) {
        loop.exec();
    }
}

/**
 * @brief slot called when the user selects the Export Config button.
 * It retrieves the setting partition and sends it to the cloud, trading it
 * for an XML configuration file.
 */
void UploaderGadgetWidget::onExportButtonClick()
{
    if (telMngr->isConnected()) {
        /* select the UAV-oriented export thing */
        Core::ActionManager *am = Core::ICore::instance()->actionManager();

        if (!am) return;

        Core::Command *cmd = am->command("UAVSettingsImportExportPlugin.UAVSettingsExport");

        if (cmd) {
            cmd->action();
        }

        return;
    }

    /* make sure there's a setting partition */
    if (!haveSettingsPart()) {
        QMessageBox msgBox;

        msgBox.setText(tr("No settings partition accessible; can't export."));
        msgBox.setInformativeText(tr("If you're using a F3 or F4 flight controller, please upgrade your bootloader. F1 does not support export configuration in the main loader."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);

        msgBox.exec();

        return;
    }

    /* XXX TODO:  make sure the cloud service is there and has right git rev */

    /* get confirmation from user that using the cloud service is OK */
    QMessageBox msgBox;
    msgBox.setText(tr("Do you wish to export the settings partition as an XML settings file?"));
    msgBox.setInformativeText(tr("This will send the raw configuration information to a dRonin cloud service for translation."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    int val = msgBox.exec();

    if (val != QMessageBox::Yes) {
        return;
    }

    /* pull down settings partition to ram */
    if (!downloadSettings()) {
        setStatusInfo(tr("Error, unable to pull settings partition"), uploader::STATUSICON_FAIL);

        return;
    }

    setStatusInfo(tr("Retrieved settings; contacting cloud..."), uploader::STATUSICON_FAIL);

    QString upgradingFrom = m_widget->gitHashOD_lbl->text();

    tradeSettingsWithCloud(upgradingFrom);
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
    if (uploaderStatus == uploader::BL_SITTING) {
        return;
    }


    foreach(int vendorID, brdMgr->getKnownVendorIDs()) {
        devices.append(USBMonitor::instance()->availableDevices(vendorID,-1,-1,USBMonitor::Upgrader));
        devices.append(USBMonitor::instance()->availableDevices(vendorID,-1,-1,USBMonitor::Bootloader));
    }

    if(devices.length() > 1) {
        setStatusInfo(tr("More than one device was detected in bootloader state"), uploader::STATUSICON_INFO);
        return;
    } else if(devices.length() == 0) {
        setStatusInfo("No devices in bootloader state detected", uploader::STATUSICON_FAIL);
        return;
    }

    USBPortInfo device = devices.first();

    bool triggerUpgrading = false;
    bool inUpgrader = false;
    bool validDescription = false;
    bool blankFC = false;

    if (device.getRunState() == USBMonitor::Upgrader) {
        inUpgrader = true;
    }

    if (dfu.OpenBootloaderComs(device)) {
        tl_dfu::device dev = dfu.findCapabilities();

        QByteArray description = dfu.DownloadDescriptionAsByteArray(dev.SizeOfDesc);
        deviceDescriptorStruct descStructure;
        if(!UAVObjectUtilManager::descriptionToStructure(description, descStructure))
            setStatusInfo(tr("Could not parse firmware metadata"), uploader::STATUSICON_INFO);
        else
        {
            FirmwareOnDeviceUpdate(descStructure, QString::number((dev.FW_CRC)));
            validDescription = true;
        }

        if (!inUpgrader) {
            switch (uploaderStatus) {
            case uploader::UPGRADING:
                triggerUpgrading = true;
            case uploader::UPGRADING_CATCHLOADER:
            case uploader::ENTERING_LOADER:
                break;
            case uploader::DISCONNECTED:
            {
                // look for completed bootloader update (last 2 chars of TlFw string are nulled)
                if (QString(description.left(4)) == "Tl")
                    break;

                if (validDescription) {
                    if (FirmwareCheckForUpdate(descStructure)) {
                        triggerUpgrading = true;
                        break;
                    } 
                } else {
                    QMessageBox msgBox;
                    msgBox.setText(tr("There appears to be no valid firmware on your device.."));
                    msgBox.setInformativeText(tr("Do you want to install firmware?"));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                    msgBox.setDefaultButton(QMessageBox::Yes);

                    int val = msgBox.exec();

                    if (val == QMessageBox::Yes) {
                        triggerUpgrading = true;
                        blankFC = true;
                    }

                    break;      /* Don't boot in any case */
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
        } else {
            m_widget->partitionBrowserTW->setRowCount(0);
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
        DeviceInformationUpdate(info);

        iapUpdated = false;

        if (!inUpgrader) {
            setStatusInfo(tr("Connection to bootloader successful"), uploader::STATUSICON_OK);

            if (FirmwareLoadFromFile(getImagePath(info.board->shortName()), &loadedFile)) {
                FirmwareLoadedClear(true);
                FirmwareLoadedUpdate(loadedFile);
                setStatusInfo(tr("Ready to flash firmware"), uploader::STATUSICON_OK);
                this->activateWindow();
                m_widget->flashButton->setFocus();
            }

            setUploaderStatus(uploader::BL_SITTING);
        } else {
            setStatusInfo(tr("Connected to upgrader-loader"), uploader::STATUSICON_OK);
            setUploaderStatus(uploader::BL_SITTING);
        }

        if (triggerUpgrading) {
            if (currentBoard.board) {
                bool canBeUpgraded = currentBoard.board->queryCapabilities(Core::IBoardType::BOARD_CAPABILITIES_UPGRADEABLE);

                if (canBeUpgraded) {
                    doUpgradeOperation(blankFC);
                    return;
                }
            }
        }

        emit enteredLoader();
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

    if (uploaderStatus != uploader::UPGRADING_CATCHLOADER) {
        setUploaderStatus(uploader::DISCONNECTED);
    }
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
    }
    else
    {
        progress -= 1;
    }
    m_widget->progressBar->setValue(progress);
    if(progress == 0)
    {
        disconnect(usbFilterBL, SIGNAL(deviceDiscovered()), this, SLOT(onRescueTimer()));
        disconnect(usbFilterUP, SIGNAL(deviceDiscovered()), this, SLOT(onRescueTimer()));
        timer.disconnect();
        setStatusInfo(tr("Failed to detect bootloader"), uploader::STATUSICON_FAIL);
        setUploaderStatus(uploader::DISCONNECTED);
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
}

void UploaderGadgetWidget::triggerPartitionDownload(int index)
{
    if(!CheckInBootloaderState())
        return;
    int size = m_widget->partitionBrowserTW->item(index,1)->text().toInt();

    setStatusInfo("",uploader::STATUSICON_RUNNING);
    setUploaderStatus(uploader::BL_BUSY);
    tempArray.clear();
    dfu.DownloadPartitionThreaded(&tempArray, (dfu_partition_label)index, size);
}

/**
 * @brief slot called when the user clicks save on the partition browser
 */
void UploaderGadgetWidget::onPartitionSave()
{
    int index = m_widget->partitionBrowserTW->selectedItems().first()->row();

    QEventLoop loop;

    bool operationSuccess = false;

    /* disconnects when loop comes out of scope */
    connect(&dfu, &DFUObject::downloadFinished, &loop, [&] (bool status) {
        operationSuccess = status;
        loop.exit();
    } );

    triggerPartitionDownload(index);

    loop.exec();

    if(operationSuccess)
    {
        setStatusInfo(tr("Partition download success"), uploader::STATUSICON_OK);
        QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),QDir::homePath(),"*.bin");
        if(filename.isEmpty())
        {
            setStatusInfo(tr("Save cancelled"), uploader::STATUSICON_FAIL);
            setUploaderStatus(uploader::BL_SITTING);
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
    } else {
        setStatusInfo(tr("Partition download failed"), uploader::STATUSICON_FAIL);
    }

    setUploaderStatus(uploader::BL_SITTING);
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
        setStatusInfo(tr("Flash cancelled"), uploader::STATUSICON_FAIL);
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
    setUploaderStatus(uploader::BL_BUSY);

    tl_dfu::Status operationSuccess = DFUidle;

    QEventLoop loop;

    /* disconnects when loop comes out of scope */
    connect(&dfu, &DFUObject::uploadFinished, &loop, [&] (tl_dfu::Status status) {
        operationSuccess = status;
        loop.exit();
    } );

    dfu.UploadPartitionThreaded(tempArray, (dfu_partition_label)index, size);

    loop.exec();

    if(operationSuccess == Last_operation_Success)
        setStatusInfo(tr("Partition upload success"), uploader::STATUSICON_OK);
    else
        setStatusInfo(tr("Partition upload failed"), uploader::STATUSICON_FAIL);
    setUploaderStatus(uploader::BL_SITTING);
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
    if(telemetryConnected && iapUpdated)
    {
        onAutopilotReady();
    }
}

/**
 * @brief Check if the board is in bootloader state
 * @return true if in bootloader state
 */
bool UploaderGadgetWidget::CheckInBootloaderState()
{
    if(uploaderStatus != uploader::BL_SITTING)
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

    QString fwDirectoryStr = getImagePath(boardName);

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
    qDebug() << str;

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
 * @brief Returns the current uploader status
 */
uploader::UploaderStatus UploaderGadgetWidget::getUploaderStatus() const
{
    return uploaderStatus;
}

bool UploaderGadgetWidget::haveSettingsPart() const
{
    /* Would be nice to determine this in a nicer way */
    if (m_widget->partitionBrowserTW->rowCount() >= DFU_PARTITION_SETTINGS) {
        return true;
    }

    return false;
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
    case uploader::ENTERING_LOADER:
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
    case uploader::BL_SITTING:
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

        m_widget->exportConfigButton->setEnabled(haveSettingsPart());

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
    case uploader::UPGRADING:
    case uploader::UPGRADING_CATCHLOADER:
    case uploader::BL_BUSY:
        m_widget->progressBar->setVisible(true);

        m_widget->rescueButton->setEnabled(false);
        m_widget->openButton->setEnabled(false);
        m_widget->bootButton->setEnabled(false);
        m_widget->safeBootButton->setEnabled(false);
        m_widget->flashButton->setEnabled(false);
        m_widget->exportConfigButton->setEnabled(false);
        m_widget->partitionBrowserTW->setContextMenuPolicy(Qt::NoContextMenu);
        break;
    default:
        break;
    }
}

/**
 * @brief Opens the plugin help page on the default browser
 */
void UploaderGadgetWidget::openHelp()
{
    QDesktopServices::openUrl( QUrl("https://github.com/d-ronin/dRonin/wiki/OnlineHelp:-Uploader-Plugin", QUrl::StrictMode) );
}

QString UploaderGadgetWidget::getImagePath(QString boardName, QString imageType)
{
    QString imageName = QString("%1_%2").arg(imageType).arg(boardName.toLower());
    QString imageNameWithSuffix = QString("%1.tlfw").arg(imageName);

    QStringList paths = QStringList() 
        /* Relative paths first; try in application bundle or build dir */
        << "."                                  // uh, right here?
        << "../firmware"                        // windows installed path
        /* Added ../build to these to make sure our assumptions are right 
         * about it being a build tree */
        << "../../../../build"                  // windows / linux build
        << "../../../../../../../build"         // OSX build
        << "../Resources/firmware"              // OSX app bundle
        << "/usr/local/" GCS_PROJECT_BRANDING_PRETTY "/firmware"; // leenucks deb

    foreach (QString path, paths) {
        QDir pathDir = QDir(QCoreApplication::applicationDirPath());

        if (pathDir.cd(path)) {
            /* Two things to try. */
            QString perTargetPath = QString("%1/%2/%3").arg(pathDir.absolutePath()).arg(imageName).arg(imageNameWithSuffix);

            if (QFile::exists(perTargetPath)) {
                return perTargetPath;
            }

            QString directPath = QString("%1/%2").arg(pathDir.absolutePath()).arg(imageNameWithSuffix);

            if (QFile::exists(directPath)) {
                return directPath;
            }
        }
    }

    /* This is sane for file dialogs, too. */
    return QString("");
}

bool UploaderGadgetWidget::FirmwareLoadFromFile(QString filename,
        QByteArray *contents)
{
    QFileInfo fileinfo = QFileInfo(filename);

    if (!fileinfo.exists())
        return false;

    QFile file(fileinfo.filePath());
    if(!file.open(QIODevice::ReadOnly))
        return false;
    *contents = file.readAll();

    return true;
}

bool UploaderGadgetWidget::FirmwareCheckForUpdate(deviceDescriptorStruct device)
{
    const QString gcsRev(GCS_REVISION);
    if (gcsRev.contains(':')) {
        QString gcsShort = gcsRev.mid(gcsRev.indexOf(':') + 1, 8);
        if ((gcsShort != device.gitHash) && (ignoredRev != device.gitHash)) {
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
                ignoredRev = device.gitHash;
        }
    }
    return false;
}
