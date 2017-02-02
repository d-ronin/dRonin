/**
 ******************************************************************************
 *
 * @file       usagestatsplugin.h
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

#ifndef USAGESTATSPLUGIN_H_
#define USAGESTATSPLUGIN_H_

#include <coreplugin/iconfigurableplugin.h>
#include <coreplugin/icorelistener.h>
#include <extensionsystem/iplugin.h>
#include "uploader/uploader_global.h"
#include "uavobjectutil/devicedescriptorstruct.h"
#include "debuggadget/debugengine.h"
#include <QDateTime>
#include <QAbstractButton>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QUuid>

using namespace uploader;
struct boardLog
{
    QDateTime time;
    deviceInfo board;
    deviceDescriptorStruct device;
};
enum widgetType {WIDGET_BUTTON, WIDGET_SLIDER, WIDGET_TAB};
typedef struct widgetActionInfoType {
    widgetType type;
    QDateTime time;
    QString objectName;
    QString className;
    QString parentName;
    QString data1;
    QString data2;
} widgetActionInfo;
typedef struct debugMessageStruct {
    QDateTime time;
    int level;
    QString levelString;
    QString message;
    QString file;
    int line;
    QString function;
} DebugMessage;

class UsageStatsPlugin :  public Core::IConfigurablePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.dronin.plugins.UsageStats")

public:
    UsageStatsPlugin();
    ~UsageStatsPlugin();
    void readConfig(QSettings *qSettings, Core::UAVConfigInfo *configInfo);
    void saveConfig(QSettings *qSettings, Core::UAVConfigInfo *configInfo);
    void shutdown();
    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    bool coreAboutToClose();
    bool getSendUsageStats() const;
    void setSendUsageStats(bool value);
    bool getSendPrivateData() const;
    void setSendPrivateData(bool value);
    int getDebugLogLevel() const;
    void setDebugLogLevel(int value);

    QString getInstallationUUID() const;
public slots:
    void updateSettings();
private:
    ExtensionSystem::PluginManager *pluginManager;
    QList<boardLog> boardLogList;
    QList<widgetActionInfo> widgetLogList;
    QList<DebugMessage> debugMessageList;
    QByteArray processJson();
    QString externalIP;
    QNetworkAccessManager netMngr;
    QEventLoop *loop;
    bool sendUsageStats;
    bool sendPrivateData;
    QUuid installationUUID;
    int debugLogLevel;
private slots:
    void pluginsLoadEnded();
    void addNewBoardSeen(deviceInfo, deviceDescriptorStruct);
    void searchForWidgets(QObject *mw, bool connect);
    void onButtonClicked();
    void onSliderValueChanged(int);
    void onTabCurrentChanged(int);
    void onDebugMessage(DebugEngine::Level, const QString &, const QString &, const int, const QString &);
};
class AppCloseHook : public Core::ICoreListener {
    Q_OBJECT
public:
    AppCloseHook(UsageStatsPlugin *parent);
    bool coreAboutToClose();
private:
    UsageStatsPlugin *m_parent;
};
#endif /* USAGESTATSPLUGIN_H_ */
