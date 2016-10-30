/**
 ******************************************************************************
 *
 * @file       configtaskwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectWidgetUtils Plugin
 * @{
 * @brief Utility plugin for UAVObject to Widget relation management
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
#ifndef CONFIGTASKWIDGET_H
#define CONFIGTASKWIDGET_H


#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "uavobjectutilmanager.h"
#include <QQueue>
#include <QWidget>
#include <QList>
#include <QLabel>
#include "smartsavebutton.h"
#include "mixercurvewidget.h"
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include "uavobjectwidgetutils_global.h"
#include <QDesktopServices>
#include <QUrl>
#include <QEvent>

class UAVOBJECTWIDGETUTILS_EXPORT ConfigTaskWidget: public QWidget
{
    Q_OBJECT

public:
    struct shadow
    {
        QWidget * widget;
        double scale;
        bool isLimited;
        bool useUnits;
    };
    struct objectToWidget
    {
        UAVObject * object;
        UAVObjectField * field;
        QWidget * widget;
        int index;
        double scale;
        bool isLimited;
        bool useUnits;
        QList<shadow *> shadowsList;
    };

    struct temphelper
    {
        quint32 objid;
        quint32 objinstid;
        bool operator==(const temphelper& lhs)
        {
            return (lhs.objid==this->objid && lhs.objinstid==this->objinstid);
        }
    };

    enum buttonTypeEnum {none,save_button,apply_button,reload_button,default_button,help_button,reboot_button,connections_button};
    enum metadataSetEnum {ALL_METADATA, SETTINGS_METADATA_ONLY, NONSETTINGS_METADATA_ONLY};

    struct uiRelationAutomation
    {
        QString objname;
        QString fieldname;
        QString element;
        QString url;
        double scale;
        bool haslimits;
        bool useUnits;
        buttonTypeEnum buttonType;
        QList<int> buttonGroup;
    };

    ConfigTaskWidget(QWidget *parent = 0);
    virtual ~ConfigTaskWidget();

    void disableMouseWheelEvents();
    bool eventFilter( QObject * obj, QEvent * evt );

    void saveObjectToSD(UAVObject *obj);
    UAVObjectManager* getObjectManager();
    UAVObjectUtilManager* getObjectUtilManager();
    static double listMean(QList<double> list);
    static double listVar(QList<double> list);

    void addUAVObject(QString objectName, QList<int> *reloadGroups=NULL);
    void addUAVObject(UAVObject * objectName, QList<int> *reloadGroups=NULL);

    void addWidget(QWidget * widget);

    /**
     * @brief Add an UAVObject field to widget relation to the management system
     * Note: This is the instance called for objrelation dynamic properties
     * @param object name of the object to add
     * @param field name of the field to add
     * @param widget pointer to the widget whitch will display/define the field value
     * @param index index of the element of the field to add to this relation
     * @param scale scale value of this relation
     * @param isLimited bool to signal if this widget contents is limited in value
     * @param useUnits bool to signal if units from the UAVO should be added to the widget, NOTE: if scaling is applied an attempt will be made to fix units, or they will be dropped
     * @param defaultReloadGroups default and reload groups this relation belongs to
     * @param instID instance ID of the object used on this relation
     */
    void addUAVObjectToWidgetRelation(QString object, QString field, QWidget *widget, int index = 0, double scale = 1, bool isLimited = false, bool useUnits = false, QList<int> *defaultReloadGroups = 0, quint32 instID = 0);

    void addUAVObjectToWidgetRelation(UAVObject *obj, UAVObjectField *field, QWidget *widget, int index = 0, double scale = 1, bool isLimited = false, bool useUnits = false, QList<int> *defaultReloadGroups = 0, quint32 instID = 0);

    void addUAVObjectToWidgetRelation(QString object, QString field, QWidget *widget, QString element, double scale, bool isLimited = false, bool useUnits = false, QList<int> *defaultReloadGroups = 0, quint32 instID = 0);

    void addUAVObjectToWidgetRelation(UAVObject *obj, UAVObjectField *field, QWidget *widget, QString element, double scale, bool isLimited = false, bool useUnits = false, QList<int> *defaultReloadGroups = 0, quint32 instID = 0);

    void addUAVObjectToWidgetRelation(QString object, QString field, QWidget *widget, QString index);
    void addUAVObjectToWidgetRelation(UAVObject *obj, UAVObjectField * field, QWidget *widget, QString index);


    //BUTTONS//
    void addApplySaveButtons(QPushButton * update,QPushButton * save);
    void addReloadButton(QPushButton * button,int buttonGroup);
    void addDefaultButton(QPushButton * button,int buttonGroup);
    void addRebootButton(QPushButton * button);
    /**
     * @brief addConnectionsButton Add connection diagram button
     * @param button Widget to connect
     */
    void addConnectionsButton(QPushButton *button);
    //////////

    void addWidgetToDefaultReloadGroups(QWidget * widget, QList<int> *groups);

    //bool addShadowWidget(QWidget *masterWidget, QWidget *shadowWidget, double shadowScale = 1, bool shadowIsLimited = false, bool shadowUseUnits = false);
    bool addShadowWidget(QString object, QString field, QWidget *widget, int index = 0, double scale = 1, bool isLimited = false, bool useUnits = false, QList<int> *defaultReloadGroups = NULL, quint32 instID = 0);

    void autoLoadWidgets();
    void loadAllLimits();

    bool isAutopilotConnected();
    bool isDirty();
    void setDirty(bool value);

    bool allObjectsUpdated();
    void setOutOfLimitsStyle(QString style){outOfLimitsStyle=style;}
    void addHelpButton(QPushButton * button,QString url);
    void forceShadowUpdates();
    void forceConnectedState();

    void setNotMandatory(QString object);

    virtual void tabSwitchingAway() {}

public slots:
    void onAutopilotDisconnect();
    void onAutopilotConnect();
    void invalidateObjects();
    void apply();
    void save();
signals:
    //fired when a widgets contents changes
    void widgetContentsChanged(QWidget * widget);
    //fired when the framework requests that the widgets values be populated, use for custom behaviour
    void populateWidgetsRequested();
    //fired when the framework requests that the widgets values be refreshed, use for custom behaviour
    void refreshWidgetsValuesRequested();
    //fired when the framework requests that the UAVObject values be updated from the widgets value, use for custom behaviour
    void updateObjectsFromWidgetsRequested();
    //fired when the autopilot connects
    void autoPilotConnected();
    //fired when the autopilot disconnects
    void autoPilotDisconnected();
    void defaultRequested(int group);

private slots:
    void objectUpdated(UAVObject*);
    void defaultButtonClicked();
    void reloadButtonClicked();
    void rebootButtonClicked();
    void connectionsButtonClicked();
    void doRefreshHiddenObjects(UAVDataObject*);
private:
    int currentBoard;
    bool isConnected;
    bool allowWidgetUpdates;
    QStringList objectsList;
    QList <objectToWidget*> objOfInterest;
    ExtensionSystem::PluginManager *pm;
    UAVObjectManager *objManager;
    smartSaveButton *smartsave;
    QMap<UAVObject *,bool> objectUpdates;
    QMap<int,QList<objectToWidget*> *> defaultReloadGroups;
    QMap<QWidget *,objectToWidget*> shadowsList;
    QMap<QPushButton *,QString> helpButtonList;
    QList<QPushButton *> reloadButtonList;
    QList<QPushButton *> rebootButtonList;
    QList<QPushButton *> connectionsButtonList;
    bool dirty;
    bool setFieldFromWidget(QWidget *widget, UAVObjectField *field, int index, double scale, bool usesUnits = false);
    /**
     * Sets a widget from a UAVObject field
     * @param widget pointer to the widget to set
     * @param field pointer to the field from where to get the value from
     * @param index index of the element to use
     * @param scale scale to be used on the assignement
     * @param hasLimits set to true if you want to limit the values (check wiki)
     * @param useUnits set to true to add UAVO field units to widget
     * @return returns true if the assignement was successfull
     */
    bool setWidgetFromField(QWidget *widget, UAVObjectField *field, int index, double scale, bool hasLimits, bool useUnits = false);
    void connectWidgetUpdatesToSlot(QWidget *widget, const char *function);
    void disconnectWidgetUpdatesToSlot(QWidget *widget, const char *function);
    void loadWidgetLimits(QWidget *widget, UAVObjectField *field, int index, bool hasLimits, bool useUnits, double scale);
    /**
     * @brief applyScaleToUnits Attempts to replace SI prefixes to suit scaling
     * @param units Units to apply scaling (e.g. ms for milliseconds)
     * @param scale Scaling to apply / divide by
     * @return New units string, or empty string on failure to scale units
     */
    QString applyScaleToUnits(QString units, double scale);
    QString outOfLimitsStyle;
    QTimer * timeOut;
protected slots:
    virtual void disableObjUpdates();
    virtual void enableObjUpdates();
    virtual void clearDirty();
    virtual void widgetsContentsChanged();
    virtual void populateWidgets();
    virtual void refreshWidgetsValues(UAVObject * obj=NULL);
    virtual void updateObjectsFromWidgets();
    virtual void helpButtonPressed();
protected:
    virtual void enableControls(bool enable);
    void checkWidgetsLimits(QWidget *widget, UAVObjectField *field, int index, bool hasLimits, bool useUnits, QVariant value, double scale);
    virtual QVariant getVariantFromWidget(QWidget *widget, double scale, bool usesUnits = false);
    virtual bool setWidgetFromVariant(QWidget *widget, QVariant value, double scale, QString units = "");
    virtual QString getOptionFromChecked(QWidget* widget, bool checked);
    virtual bool getCheckedFromOption(QWidget* widget, QString option);
    bool resetWidgetToDefault(QWidget *widget);

    UAVObjectUtilManager* utilMngr;
};

#endif // CONFIGTASKWIDGET_H
