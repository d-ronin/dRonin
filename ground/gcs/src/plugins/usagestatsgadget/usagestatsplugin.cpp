/**
 ******************************************************************************
 *
 * @file       usagestatsplugin.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UsageStatsGadgetPlugin UsageStats Gadget Plugin
 * @{
 * @brief A place holder gadget plugin
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
 */
#include "usagestatsplugin.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QtPlugin>
#include <QStringList>
#include <extensionsystem/pluginmanager.h>
#include "uploader/uploadergadgetfactory.h"
#include "coreplugin/icore.h"
#include <QMainWindow>
#include <QPushButton>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QTabBar>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include GCS_VERSION_INFO_FILE
#include "coreplugin/coreconstants.h"
#include <QTimer>
#include <QScrollBar>
#include <QSysInfo>
#include "usagestatsoptionpage.h"

UsageStatsPlugin::UsageStatsPlugin(): sendUsageStats(true), sendPrivateData(true), installationUUID("")
{
    loop = new QEventLoop(this);
    connect(&netMngr, SIGNAL(finished(QNetworkReply*)), loop, SLOT(quit()));
}

UsageStatsPlugin::~UsageStatsPlugin()
{
    Core::ICore::instance()->saveSettings(this);
}

void UsageStatsPlugin::readConfig(QSettings *qSettings, Core::UAVConfigInfo *configInfo)
{
    Q_UNUSED(configInfo)
    qSettings->beginGroup(QLatin1String("UsageStatistics"));
    sendUsageStats = (qSettings->value(QLatin1String("SendUsageStats"), sendUsageStats).toBool());
    sendPrivateData = (qSettings->value(QLatin1String("SendPrivateData"), sendPrivateData).toBool());
    //Check the Installation UUID and Generate a new one if required
    installationUUID = QUuid(qSettings->value(QLatin1String("InstallationUUID"), "").toString());
    if(installationUUID.isNull()) { //Create new UUID
        installationUUID = QUuid::createUuid();
    }

    qSettings->endGroup();
}

void UsageStatsPlugin::saveConfig(QSettings *qSettings, Core::UAVConfigInfo *configInfo)
{
    Q_UNUSED(configInfo)
    qSettings->beginGroup(QLatin1String("UsageStatistics"));
    qSettings->setValue(QLatin1String("SendUsageStats"), sendUsageStats);
    qSettings->setValue(QLatin1String("SendPrivateData"), sendPrivateData);
    qSettings->setValue(QLatin1String("InstallationUUID"), installationUUID.toString());

    qSettings->endGroup();
}

void UsageStatsPlugin::shutdown()
{

}

bool UsageStatsPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)
    Core::ICore::instance()->readSettings(this);
    UsageStatsOptionPage *mop = new UsageStatsOptionPage(this);
    addAutoReleasedObject(mop);
    addAutoReleasedObject(new AppCloseHook(this));
    return true;
}

void UsageStatsPlugin::extensionsInitialized()
{
    pluginManager = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pluginManager);
    connect(pluginManager, SIGNAL(pluginsLoadEnded()), this, SLOT(pluginsLoadEnded()));
}

bool UsageStatsPlugin::coreAboutToClose()
{
    if(!sendUsageStats)
        return true;
    QTimer::singleShot(10000, loop, SLOT(quit()));
    QUrl url("http://dronin-autotown.appspot.com/usageStats");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    netMngr.post(request, processJson());
    loop->exec();
    return true;
}

void UsageStatsPlugin::pluginsLoadEnded()
{
    updateSettings();
}

void UsageStatsPlugin::updateSettings()
{
    UploaderGadgetFactory *uploader = pluginManager->getObject<UploaderGadgetFactory>();
    QMainWindow *mw = Core::ICore::instance()->mainWindow();
    if(sendUsageStats) {
        if(uploader) {
            if(sendUsageStats) {
                connect(uploader, SIGNAL(newBoardSeen(deviceInfo,deviceDescriptorStruct)), this, SLOT(addNewBoardSeen(deviceInfo, deviceDescriptorStruct)), Qt::UniqueConnection);
            }
            else {
                disconnect(uploader, SIGNAL(newBoardSeen(deviceInfo,deviceDescriptorStruct)), this, SLOT(addNewBoardSeen(deviceInfo, deviceDescriptorStruct)));
            }
    }
        if(mw) {
        searchForWidgets(mw, sendUsageStats);
        }
    }
    Core::ICore::instance()->saveSettings(this);
}

void UsageStatsPlugin::addNewBoardSeen(deviceInfo board, deviceDescriptorStruct device)
{
    boardLog b;
    b.time = QDateTime::currentDateTime();
    b.board = board;
    b.device = device;
    boardLogList.append(b);
}

void UsageStatsPlugin::searchForWidgets(QObject *mw, bool conn)
{
    foreach (QObject *obj, mw->children()) {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(obj);
        QAbstractSlider *slider = qobject_cast<QAbstractSlider*>(obj);
        QScrollBar *bar = qobject_cast<QScrollBar*>(obj);
        QTabBar *tab = qobject_cast<QTabBar*>(obj);
        if(button) {
            if(conn) {
                connect(button, SIGNAL(clicked(bool)), this, SLOT(onButtonClicked()), Qt::UniqueConnection);
            }
            else {
                disconnect(button, SIGNAL(clicked(bool)), this, SLOT(onButtonClicked()));
            }
        }
        else if(slider && !bar) {
            if(conn) {
                connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)), Qt::UniqueConnection);
            }
            else {
                disconnect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)));
            }
        }
        else if (tab) {
            if(conn) {
                connect(tab, SIGNAL(currentChanged(int)), this, SLOT(onTabCurrentChanged(int)), Qt::UniqueConnection);
            }
            else {
                disconnect(tab, SIGNAL(currentChanged(int)), this, SLOT(onTabCurrentChanged(int)));
            }
        }
        else {
            searchForWidgets(obj, conn);
        }
    }
}

void UsageStatsPlugin::onButtonClicked()
{
    widgetActionInfoType info;
    QAbstractButton *button = qobject_cast<QAbstractButton*>(sender());
    info.objectName = button->objectName();
    info.data1 = button->text();
    info.data2 = QString::number(button->isChecked());
    info.className = button->metaObject()->className();
    info.parentName = button->parent()->objectName();
    info.time = QDateTime::currentDateTime();
    info.type = WIDGET_BUTTON;
    widgetLogList.append(info);
}

void UsageStatsPlugin::onSliderValueChanged(int value)
{
    widgetActionInfo info;
    QAbstractSlider *slider = qobject_cast<QAbstractSlider*>(sender());
    info.objectName = slider->objectName();
    info.data1 = QString::number(value);
    info.className = slider->metaObject()->className();
    info.parentName = slider->parent()->objectName();
    info.time = QDateTime::currentDateTime();
    info.type = WIDGET_SLIDER;
    widgetLogList.append(info);
}

void UsageStatsPlugin::onTabCurrentChanged(int value)
{
    widgetActionInfo info;
    QTabBar *tab = qobject_cast<QTabBar*>(sender());
    info.objectName = tab->objectName();
    info.data2 = QString::number(value);
    info.data1 = tab->tabText(value);
    info.className = tab->metaObject()->className();
    info.parentName = tab->parent()->objectName();
    info.time = QDateTime::currentDateTime();
    info.type = WIDGET_TAB;
    widgetLogList.append(info);
}

QByteArray UsageStatsPlugin::processJson() {
    QJsonObject json;
    if(sendPrivateData)
        json["shareIP"] = "true";
    else
        json["shareIP"] = "false";
    json["gcs_version"] =  QLatin1String(Core::Constants::GCS_VERSION_LONG);
    json["gcs_revision"] =  QLatin1String(Core::Constants::GCS_REVISION_PRETTY_STR);
    json["currentOS"] = QSysInfo::prettyProductName();
    json["currentArch"] = QSysInfo::currentCpuArchitecture();
    json["buildInfo"] = QSysInfo::buildAbi();

    if(!installationUUID.isNull())
        json["installationUUID"] = getInstallationUUID();

    QJsonArray boardArray;
    foreach (boardLog board, boardLogList) {
        QJsonObject b;
        b["time"] = board.time.toUTC().toString("yyyy-MM-ddThh:mm:ss.zzzZ");
        b["name"] = board.board.board.data()->getBoardNameFromID(board.device.boardID());
        b["uavoHash"] = QString(board.device.uavoHash.toHex());
        b["ID"] = board.device.boardID();
        b["fwHash"] = QString(board.device.fwHash.toHex());
        b["gitDate"] = board.device.gitDate;
        b["gitHash"] = board.device.gitHash;
        b["gitTag"] = board.device.gitTag;
        if(sendPrivateData)
            b["UUID"] = QString(QCryptographicHash::hash(QByteArray::fromHex(board.board.cpu_serial.toUtf8()), QCryptographicHash::Sha256).toHex());
        boardArray.append(b);
    }
    json["boardsSeen"] = boardArray;
    QJsonArray widgetArray;
    foreach (widgetActionInfo w, widgetLogList) {
        QJsonObject j;
        j["time"] = w.time.toUTC().toString("yyyy-MM-ddThh:mm:ss.zzzZ");
        j["className"] = w.className;
        j["objectName"] = w.objectName;
        j["parentName"] = w.parentName;
        switch (w.type) {
        case WIDGET_BUTTON:
            j["text"] = w.data1;
            j["checked"] = w.data2;
            break;
        case WIDGET_SLIDER:
            j["value"] = w.data1;
            break;
        case WIDGET_TAB:
            j["text"] = w.data1;
            j["index"] = w.data2;
            break;
        default:
            break;
        }
        widgetArray.append(j);
    }
    json["widgets"] = widgetArray;
    return QJsonDocument(json).toJson();
}
bool UsageStatsPlugin::getSendPrivateData() const
{
    return sendPrivateData;
}

void UsageStatsPlugin::setSendPrivateData(bool value)
{
    sendPrivateData = value;
}

bool UsageStatsPlugin::getSendUsageStats() const
{
    return sendUsageStats;
}

void UsageStatsPlugin::setSendUsageStats(bool value)
{
    sendUsageStats = value;
}


QString UsageStatsPlugin::getInstallationUUID() const
{
    return installationUUID.toString().remove(QRegExp("[{}]*"));
}

AppCloseHook::AppCloseHook(UsageStatsPlugin *parent) : Core::ICoreListener(parent), m_parent(parent)
{
}

bool AppCloseHook::coreAboutToClose()
{
    return m_parent->coreAboutToClose();
}
