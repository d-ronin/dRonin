/**
 ******************************************************************************
 *
 * @file       usagestatsplugin.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015, 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UsageStatsPlugin UsageStats Gadget Plugin
 * @{
 * @brief Usage stats collection plugin
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
#include "coreplugin/coreconstants.h"
#include <QTimer>
#include <QScrollBar>
#include <QSysInfo>
#include "usagestatsoptionpage.h"

UsageStatsPlugin::UsageStatsPlugin(): sendUsageStats(true), sendPrivateData(true), installationUUID(""), debugLogLevel(DebugEngine::WARNING)
{
    loop = new QEventLoop(this);
    connect(&netMngr, &QNetworkAccessManager::finished, loop, &QEventLoop::quit);
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
    if (installationUUID.isNull()) { //Create new UUID
        installationUUID = QUuid::createUuid();
    }
    debugLogLevel = (qSettings->value(QLatin1String("DebugLogLevel"), debugLogLevel).toInt());

    qSettings->endGroup();
}

void UsageStatsPlugin::saveConfig(QSettings *qSettings, Core::UAVConfigInfo *configInfo)
{
    Q_UNUSED(configInfo)
    qSettings->beginGroup(QLatin1String("UsageStatistics"));
    qSettings->setValue(QLatin1String("SendUsageStats"), sendUsageStats);
    qSettings->setValue(QLatin1String("SendPrivateData"), sendPrivateData);
    qSettings->setValue(QLatin1String("InstallationUUID"), installationUUID.toString());
    qSettings->setValue(QLatin1String("DebugLogLevel"), debugLogLevel);

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
    connect(pluginManager, &ExtensionSystem::PluginManager::pluginsLoadEnded, this, &UsageStatsPlugin::pluginsLoadEnded);
}

bool UsageStatsPlugin::coreAboutToClose()
{
    if(!sendUsageStats)
        return true;
    QTimer::singleShot(10000, loop, &QEventLoop::quit);
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
    if (sendUsageStats) {
        connect(DebugEngine::getInstance(), &DebugEngine::message,
                this, &UsageStatsPlugin::onDebugMessage,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection) /* sigh */);
        if (uploader)
            connect(uploader, &uploader::UploaderGadgetFactory::newBoardSeen, this, &UsageStatsPlugin::addNewBoardSeen, Qt::UniqueConnection);
        if (mw)
            searchForWidgets(mw, true);
    } else {
        if (uploader)
            disconnect(uploader, &uploader::UploaderGadgetFactory::newBoardSeen, this, &UsageStatsPlugin::addNewBoardSeen);
        if (mw)
            searchForWidgets(mw, false);
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
                connect(button, &QAbstractButton::clicked, this, &UsageStatsPlugin::onButtonClicked, Qt::UniqueConnection);
            }
            else {
                disconnect(button, &QAbstractButton::clicked, this, &UsageStatsPlugin::onButtonClicked);
            }
        }
        else if(slider && !bar) {
            if(conn) {
                connect(slider, &QAbstractSlider::valueChanged, this, &UsageStatsPlugin::onSliderValueChanged, Qt::UniqueConnection);
            }
            else {
                disconnect(slider, &QAbstractSlider::valueChanged, this, &UsageStatsPlugin::onSliderValueChanged);
            }
        }
        else if (tab) {
            if(conn) {
                connect(tab, &QTabBar::currentChanged, this, &UsageStatsPlugin::onTabCurrentChanged, Qt::UniqueConnection);
            }
            else {
                disconnect(tab, &QTabBar::currentChanged, this, &UsageStatsPlugin::onTabCurrentChanged);
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

    if (!installationUUID.isNull())
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
        b["nextAncestor"] = board.device.nextAncestor;
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

    QJsonArray debugLogArray;
    foreach (const DebugMessage msg, debugMessageList) {
        QJsonObject d;
        d["time"] = msg.time.toUTC().toString("yyyy-MM-ddThh:mm:ss.zzzZ");
        d["level"] = msg.level;
        d["levelString"] = msg.levelString;
        d["message"] = msg.message;
        d["file"] = msg.file;
        d["line"] = msg.line;
        d["function"] = msg.function;
        debugLogArray.append(d);
    }
    json["debugLog"] = debugLogArray;
    json["debugLevel"] = debugLogLevel;

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

int UsageStatsPlugin::getDebugLogLevel() const
{
    return debugLogLevel;
}

void UsageStatsPlugin::setDebugLogLevel(int value)
{
    debugLogLevel = value;
}

void UsageStatsPlugin::onDebugMessage(DebugEngine::Level level, const QString &msg, const QString &file, const int line, const QString &function)
{
    if (level < debugLogLevel)
        return;

    DebugMessage info;
    switch (level) {
    case DebugEngine::DEBUG:
        info.levelString = "debug";
        break;
    case DebugEngine::INFO:
        info.levelString = "info";
        break;
    case DebugEngine::WARNING:
        info.levelString = "warning";
        break;
    case DebugEngine::CRITICAL:
        info.levelString = "critical";
        break;
    case DebugEngine::FATAL:
        info.levelString = "fatal";
        break;
    }

    info.time = QDateTime::currentDateTime();
    info.level = level;
    info.message = msg;
    info.file = file;
    info.line = line;
    info.function = function;

    debugMessageList.append(info);
}

AppCloseHook::AppCloseHook(UsageStatsPlugin *parent) : Core::ICoreListener(parent), m_parent(parent)
{
}

bool AppCloseHook::coreAboutToClose()
{
    return m_parent->coreAboutToClose();
}
