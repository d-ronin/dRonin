/**
 ******************************************************************************
 *
 * @file       uavobjectbrowserwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dRonin.org, Copyright (C) 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectBrowserPlugin UAVObject Browser Plugin
 * @{
 * @brief The UAVObject Browser gadget plugin
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

#include "uavobjectbrowserwidget.h"
#include "uavobjecttreemodel.h"
#include "browseritemdelegate.h"
#include "treeitem.h"
#include "ui_uavobjectbrowser.h"
#include "ui_viewoptions.h"
#include "uavobjects/uavobjectmanager.h"
#include <QStringList>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QtCore/QDebug>
#include <QItemEditorFactory>
#include "extensionsystem/pluginmanager.h"
#include <math.h>

#define MAXIMUM_UPDATE_PERIOD 200

UAVObjectBrowserWidget::UAVObjectBrowserWidget(QWidget *parent)
    : QWidget(parent)
    , updatePeriod(MAXIMUM_UPDATE_PERIOD)
{
    // Create browser and configuration GUIs
    m_browser = new Ui_UAVObjectBrowser();
    m_viewoptions = new Ui_viewoptions();
    m_viewoptionsDialog = new QDialog(this);
    m_viewoptions->setupUi(m_viewoptionsDialog);
    m_browser->setupUi(this);

    // Create data model
    m_model = new UAVObjectTreeModel(this);

    // Create tree view and add to layout
    treeView = new UAVOBrowserTreeView(MAXIMUM_UPDATE_PERIOD);
    treeView->setObjectName(QString::fromUtf8("treeView"));
    m_browser->verticalLayout->addWidget(treeView);

    connect(m_browser->saveSDButton, &QAbstractButton::clicked, this,
            &UAVObjectBrowserWidget::saveObject);
    connect(m_browser->readSDButton, &QAbstractButton::clicked, this,
            &UAVObjectBrowserWidget::loadObject);
    connect(m_browser->eraseSDButton, &QAbstractButton::clicked, this,
            &UAVObjectBrowserWidget::eraseObject);
    connect(m_browser->sendButton, &QAbstractButton::clicked, this,
            &UAVObjectBrowserWidget::sendUpdate);
    connect(m_browser->requestButton, &QAbstractButton::clicked, this,
            &UAVObjectBrowserWidget::requestUpdate);
    connect(m_browser->viewSettingsButton, &QAbstractButton::clicked, this,
            &UAVObjectBrowserWidget::viewSlot);

    connect(treeView, &QTreeView::collapsed, this, &UAVObjectBrowserWidget::onTreeItemCollapsed);
    connect(treeView, &QTreeView::expanded, this, &UAVObjectBrowserWidget::onTreeItemExpanded);

    connect(m_browser->le_searchField, &QLineEdit::textChanged, this,
            &UAVObjectBrowserWidget::searchTextChanged);
    connect(m_browser->bn_clearSearchField, &QAbstractButton::clicked, this,
            &UAVObjectBrowserWidget::searchTextCleared);

    // Set browser buttons to disabled
    enableUAVOBrowserButtons(false);
}

void UAVObjectBrowserWidget::onTreeItemExpanded(QModelIndex currentProxyIndex)
{
    QModelIndex currentIndex = proxyModel->mapToSource(currentProxyIndex);
    TreeItem *item = static_cast<TreeItem *>(currentIndex.internalPointer());
    TopTreeItem *top = dynamic_cast<TopTreeItem *>(item->parent());

    // Check if current tree index is the child of the top tree item
    if (top) {
        ObjectTreeItem *objItem = dynamic_cast<ObjectTreeItem *>(item);
        // If the cast succeeds, then this is a UAVO
        if (objItem) {
            UAVObject *obj = objItem->object();
            // Check for multiple instance UAVO
            if (!obj) {
                objItem = dynamic_cast<ObjectTreeItem *>(item->getChild(0));
                obj = objItem->object();
            }
            Q_ASSERT(obj);
            UAVObject::Metadata mdata = obj->getMetadata();

            // Determine fastest update
            quint16 tmpUpdatePeriod = MAXIMUM_UPDATE_PERIOD;
            int accessType = UAVObject::GetGcsTelemetryUpdateMode(mdata);
            if (accessType != UAVObject::UPDATEMODE_MANUAL) {
                switch (accessType) {
                case UAVObject::UPDATEMODE_ONCHANGE:
                    tmpUpdatePeriod = 0;
                    break;
                case UAVObject::UPDATEMODE_PERIODIC:
                case UAVObject::UPDATEMODE_THROTTLED:
                    tmpUpdatePeriod = std::min(mdata.gcsTelemetryUpdatePeriod, tmpUpdatePeriod);
                    break;
                }
            }

            accessType = UAVObject::GetFlightTelemetryUpdateMode(mdata);
            if (accessType != UAVObject::UPDATEMODE_MANUAL) {
                switch (accessType) {
                case UAVObject::UPDATEMODE_ONCHANGE:
                    tmpUpdatePeriod = 0;
                    break;
                case UAVObject::UPDATEMODE_PERIODIC:
                case UAVObject::UPDATEMODE_THROTTLED:
                    tmpUpdatePeriod = std::min(mdata.flightTelemetryUpdatePeriod, tmpUpdatePeriod);
                    break;
                }
            }

            expandedUavoItems.insert(obj->getName(), tmpUpdatePeriod);

            if (tmpUpdatePeriod < updatePeriod) {
                updatePeriod = tmpUpdatePeriod;
                treeView->updateTimerPeriod(updatePeriod);
            }
        }
    }
}

void UAVObjectBrowserWidget::onTreeItemCollapsed(QModelIndex currentProxyIndex)
{
    QModelIndex currentIndex = proxyModel->mapToSource(currentProxyIndex);
    TreeItem *item = static_cast<TreeItem *>(currentIndex.internalPointer());
    TopTreeItem *top = dynamic_cast<TopTreeItem *>(item->parent());

    // Check if current tree index is the child of the top tree item
    if (top) {
        ObjectTreeItem *objItem = dynamic_cast<ObjectTreeItem *>(item);
        // If the cast succeeds, then this is a UAVO
        if (objItem) {
            UAVObject *obj = objItem->object();

            // Check for multiple instance UAVO
            if (!obj) {
                objItem = dynamic_cast<ObjectTreeItem *>(item->getChild(0));
                obj = objItem->object();
            }
            Q_ASSERT(obj);

            // Remove the UAVO, getting its stored value first.
            quint16 tmpUpdatePeriod = expandedUavoItems.value(obj->getName());
            expandedUavoItems.take(obj->getName());

            // Check if this was the fastest UAVO
            if (tmpUpdatePeriod == updatePeriod) {
                // If so, search for the new fastest UAVO
                updatePeriod = MAXIMUM_UPDATE_PERIOD;
                foreach (tmpUpdatePeriod, expandedUavoItems) {
                    if (tmpUpdatePeriod < updatePeriod)
                        updatePeriod = tmpUpdatePeriod;
                }
                treeView->updateTimerPeriod(updatePeriod);
            }
        }
    }
}

void UAVObjectBrowserWidget::updateThrottlePeriod(UAVObject *obj)
{
    // Test if this is a metadata object. A UAVO's metadata's object ID is the UAVO's object ID + 1
    if ((obj->getObjID() & 0x01) == 1) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        Q_ASSERT(pm);
        UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
        Q_ASSERT(objManager);
        QVector<UAVObject *> list = objManager->getObjectInstancesVector(obj->getObjID() - 1);
        obj = list.at(0);
    }

    UAVObject::Metadata mdata = obj->getMetadata();

    // Determine fastest update
    quint16 tmpUpdatePeriod = MAXIMUM_UPDATE_PERIOD;
    int accessType = UAVObject::GetGcsTelemetryUpdateMode(mdata);
    if (accessType != UAVObject::UPDATEMODE_MANUAL) {
        switch (accessType) {
        case UAVObject::UPDATEMODE_ONCHANGE:
            tmpUpdatePeriod = 0;
            break;
        case UAVObject::UPDATEMODE_PERIODIC:
        case UAVObject::UPDATEMODE_THROTTLED:
            tmpUpdatePeriod = std::min(mdata.gcsTelemetryUpdatePeriod, tmpUpdatePeriod);
            break;
        }
    }

    accessType = UAVObject::GetFlightTelemetryUpdateMode(mdata);
    if (accessType != UAVObject::UPDATEMODE_MANUAL) {
        switch (accessType) {
        case UAVObject::UPDATEMODE_ONCHANGE:
            tmpUpdatePeriod = 0;
            break;
        case UAVObject::UPDATEMODE_PERIODIC:
        case UAVObject::UPDATEMODE_THROTTLED:
            tmpUpdatePeriod = std::min(mdata.flightTelemetryUpdatePeriod, tmpUpdatePeriod);
            break;
        }
    }

    expandedUavoItems.insert(obj->getName(), tmpUpdatePeriod);

    updatePeriod = MAXIMUM_UPDATE_PERIOD;
    foreach (tmpUpdatePeriod, expandedUavoItems) {
        if (tmpUpdatePeriod < updatePeriod)
            updatePeriod = tmpUpdatePeriod;
    }
    treeView->updateTimerPeriod(updatePeriod);
}

UAVObjectBrowserWidget::~UAVObjectBrowserWidget()
{
    delete m_browser;
}

/**
 * @brief UAVObjectBrowserWidget::setViewOptions Sets the viewing options
 * @param scientific true turns on scientific notation view
 * @param metadata true turns on metadata view
 */
void UAVObjectBrowserWidget::setViewOptions(bool scientific, bool metadata,
                                            bool hideNotPresent)
{
    m_viewoptions->cbMetaData->setChecked(metadata);
    m_viewoptions->cbScientific->setChecked(scientific);
    m_viewoptions->cbHideNotPresent->setChecked(hideNotPresent);
}

/**
 * @brief Initializes the model and makes the necessary signal/slot connections
 *
 */
void UAVObjectBrowserWidget::initialize()
{
    m_model->initializeModel(m_viewoptions->cbScientific->isChecked());

    // Create and configure the proxy model
    proxyModel = new TreeSortFilterProxyModel(this);
    proxyModel->setSourceModel(m_model);
    treeView->setModel(proxyModel);
    treeView->setColumnWidth(0, 300);

    treeView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    treeView->setSelectionBehavior(QAbstractItemView::SelectItems);
    treeView->setUniformRowHeights(true);

    BrowserItemDelegate *m_delegate = new BrowserItemDelegate(proxyModel);
    treeView->setItemDelegate(m_delegate);

    // Connect signals
    connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &UAVObjectBrowserWidget::toggleUAVOButtons);

    showMetaData(m_viewoptions->cbMetaData->isChecked());
    refreshHiddenObjects();
    connect(m_viewoptions->cbScientific, &QAbstractButton::toggled, this,
            &UAVObjectBrowserWidget::viewOptionsChangedSlot);
    connect(m_viewoptions->cbHideNotPresent, &QAbstractButton::toggled, this,
            &UAVObjectBrowserWidget::showNotPresent);
    connect(m_viewoptions->cbMetaData, &QAbstractButton::toggled, this,
            &UAVObjectBrowserWidget::showMetaData);
    connect(m_model, &UAVObjectTreeModel::presentOnHardwareChanged, this,
            &UAVObjectBrowserWidget::doRefreshHiddenObjects,
            (Qt::ConnectionType)(Qt::UniqueConnection | Qt::QueuedConnection));
}

/**
 * @brief Refreshes the hidden object display
 */
void UAVObjectBrowserWidget::refreshHiddenObjects()
{
    QList<QModelIndex> indexList = m_model->getDataObjectIndexes();
    foreach (QModelIndex modelIndex, indexList) {
        QModelIndex proxyModelindex = proxyModel->mapFromSource(modelIndex);

        TreeItem *item = static_cast<TreeItem *>(modelIndex.internalPointer());
        if (item)
            treeView->setRowHidden(proxyModelindex.row(), proxyModelindex.parent(),
                                   m_viewoptions->cbHideNotPresent->isChecked()
                                       && !item->getIsPresentOnHardware());
    }
}

/**
 * @brief UAVObjectBrowserWidget::showMetaData Shows UAVO metadata
 * @param show true shows the metadata, false hides metadata
 */
void UAVObjectBrowserWidget::showMetaData(bool show)
{
    refreshViewOptions();
    QList<QModelIndex> metaIndexes = m_model->getMetaDataIndexes();
    foreach (QModelIndex modelIndex, metaIndexes) {
        QModelIndex proxyModelindex = proxyModel->mapFromSource(modelIndex);

        treeView->setRowHidden(proxyModelindex.row(), proxyModelindex.parent(), !show);
    }
}

/**
 * @brief fires the viewOptionsChanged signal with the current values.
 */
void UAVObjectBrowserWidget::refreshViewOptions()
{
    emit viewOptionsChanged(
        m_viewoptions->cbScientific->isChecked(),
        m_viewoptions->cbMetaData->isChecked(), m_viewoptions->cbHideNotPresent->isChecked());
}

/**
 * @brief UAVObjectBrowserWidget::showNotPresent Shows or hides object not present on the hardware
 * @param show true shows the objects not present on the hardware, false hides them
 */
void UAVObjectBrowserWidget::showNotPresent(bool show)
{
    Q_UNUSED(show);
    refreshViewOptions();
    refreshHiddenObjects();
}

void UAVObjectBrowserWidget::doRefreshHiddenObjects()
{
    refreshHiddenObjects();
}

/**
 * @brief UAVObjectBrowserWidget::sendUpdate Sends a UAVO to board RAM. Does not affect board NVRAM.
 */
void UAVObjectBrowserWidget::sendUpdate()
{
    this->setFocus();
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    // This occurs when the selected item is not inside a UAVObject
    if (objItem == NULL) {
        return;
    }

    UAVDataObject *dataObj = qobject_cast<UAVDataObject *>(objItem->object());
    if (dataObj && dataObj->isSettings())
        objItem->setUpdatedOnly(true);
    objItem->apply();
    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
    obj->updated();

    // Search for the new fastest UAVO
    updateThrottlePeriod(obj);
}

/**
 * @brief UAVObjectBrowserWidget::requestUpdate Requests a UAVO from board RAM. Does not affect
 * board NVRAM.
 */
void UAVObjectBrowserWidget::requestUpdate()
{
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    // This occurs when the selected item is not inside a UAVObject
    if (objItem == NULL) {
        return;
    }

    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
    obj->requestUpdate();

    // Search for the new fastest UAVO
    updateThrottlePeriod(obj);
}

/**
 * @brief UAVObjectBrowserWidget::findCurrentObjectTreeItem Finds the UAVO selected in the object
 * tree
 * @return Object tree item corresponding to UAVO name
 */
ObjectTreeItem *UAVObjectBrowserWidget::findCurrentObjectTreeItem()
{
    QModelIndex current = proxyModel->mapToSource(treeView->currentIndex());
    TreeItem *item = static_cast<TreeItem *>(current.internalPointer());
    ObjectTreeItem *objItem = 0;

    // Recursively iterate over child branches until the parent UAVO branch is found
    while (item) {
        // Attempt a dynamic cast
        objItem = dynamic_cast<ObjectTreeItem *>(item);

        // If the cast succeeds, then this is a UAVO or UAVO metada. Stop the while loop.
        if (objItem)
            break;

        // If it fails, then set item equal to the parent branch, and try again.
        item = item->parent();
    }

    return objItem;
}

/**
 * @brief UAVObjectBrowserWidget::saveObject Save UAVO to board NVRAM. THis loads the UAVO into
 * board RAM.
 */
void UAVObjectBrowserWidget::saveObject()
{
    this->setFocus();
    // Send update so that the latest value is saved
    sendUpdate();
    // Save object
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    // This occurs when the selected item is not inside a UAVObject
    if (objItem == NULL) {
        return;
    }

    UAVDataObject *dataObj = qobject_cast<UAVDataObject *>(objItem->object());
    if (dataObj && dataObj->isSettings())
        objItem->setUpdatedOnly(false);
    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
    updateObjectPersistance(ObjectPersistence::OPERATION_SAVE, obj);

    // Search for the new fastest UAVO
    updateThrottlePeriod(obj);
}

/**
 * @brief UAVObjectBrowserWidget::loadObject  Retrieve UAVO from board NVRAM. This loads the UAVO
 * into board RAM.
 */
void UAVObjectBrowserWidget::loadObject()
{
    // Load object
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    // This occurs when the selected item is not inside a UAVObject
    if (objItem == NULL) {
        return;
    }

    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
    updateObjectPersistance(ObjectPersistence::OPERATION_LOAD, obj);
    // Retrieve object so that latest value is displayed
    requestUpdate();

    // Search for the new fastest UAVO
    updateThrottlePeriod(obj);
}

/**
 * @brief UAVObjectBrowserWidget::eraseObject Erases the selected UAVO from board NVRAM.
 */
void UAVObjectBrowserWidget::eraseObject()
{
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();

    // This occurs when the selected item is not inside a UAVObject
    if (objItem == NULL) {
        return;
    }

    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
    updateObjectPersistance(ObjectPersistence::OPERATION_DELETE, obj);
}

/**
 * @brief UAVObjectBrowserWidget::updateObjectPersistance Sends an object persistance command to the
 * flight controller
 * @param op  ObjectPersistence::OperationOptions enum
 * @param obj UAVObject that will be operated on
 */
void UAVObjectBrowserWidget::updateObjectPersistance(ObjectPersistence::OperationOptions op,
                                                     UAVObject *obj)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    ObjectPersistence *objper =
        dynamic_cast<ObjectPersistence *>(objManager->getObject(ObjectPersistence::NAME));
    if (obj != NULL) {
        ObjectPersistence::DataFields data;
        data.Operation = op;
        data.ObjectID = obj->getObjID();
        data.InstanceID = obj->getInstID();
        objper->setData(data);
        objper->updated();
    }
}

/**
 * @brief UAVObjectBrowserWidget::toggleUAVOButtons Toggles the UAVO buttons depending on
 * 1) which branch of the UAVO tree is selected or 2) if there is no data in the current tree(?)
 * @param current Current model index
 * @param previous unused
 */
void UAVObjectBrowserWidget::toggleUAVOButtons(const QModelIndex &currentProxyIndex,
                                               const QModelIndex &previousIndex)
{
    Q_UNUSED(previousIndex);

    QModelIndex currentIndex = proxyModel->mapToSource(currentProxyIndex);
    TreeItem *item = static_cast<TreeItem *>(currentIndex.internalPointer());
    TopTreeItem *top = dynamic_cast<TopTreeItem *>(item);
    ObjectTreeItem *data = dynamic_cast<ObjectTreeItem *>(item);

    bool enableState = true;

    // Check if current index refers to an empty index
    if (currentIndex == QModelIndex())
        enableState = false;

    // Check if current tree index is the top tree item
    if (top || (data && !data->object()))
        enableState = false;

    enableUAVOBrowserButtons(enableState);
}

/**
 * @brief UAVObjectBrowserWidget::viewSlot Trigger view options dialog
 */
void UAVObjectBrowserWidget::viewSlot()
{
    if (m_viewoptionsDialog->isVisible())
        m_viewoptionsDialog->setVisible(false);
    else {
        QPoint pos = QCursor::pos();
        pos.setX(pos.x() - m_viewoptionsDialog->width());
        m_viewoptionsDialog->move(pos);
        m_viewoptionsDialog->show();
    }
}

/**
 * @brief UAVObjectBrowserWidget::viewOptionsChangedSlot Triggers when the "view options" checkboxes
 * are toggled
 */
void UAVObjectBrowserWidget::viewOptionsChangedSlot()
{
    emit viewOptionsChanged(
        m_viewoptions->cbScientific->isChecked(),
        m_viewoptions->cbMetaData->isChecked(), m_viewoptions->cbHideNotPresent);
    m_model->initializeModel(m_viewoptions->cbScientific->isChecked());

    // Reset proxy model
    disconnect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
               &UAVObjectBrowserWidget::toggleUAVOButtons);
    delete proxyModel;
    proxyModel = new TreeSortFilterProxyModel(this);
    proxyModel->setSourceModel(m_model);
    treeView->setModel(proxyModel);
    searchTextChanged(m_browser->le_searchField->text());
    connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &UAVObjectBrowserWidget::toggleUAVOButtons);

    showMetaData(m_viewoptions->cbMetaData->isChecked());
    refreshHiddenObjects();
}

/**
 * @brief UAVObjectBrowserWidget::enableUAVOBrowserButtons Enables or disables UAVO browser buttons
 * @param enableState true enables buttons, false disables them.
 */
void UAVObjectBrowserWidget::enableUAVOBrowserButtons(bool enableState)
{
    // temporary hack until session management is fixed
    enableState = true;
    m_browser->sendButton->setEnabled(enableState);
    m_browser->requestButton->setEnabled(enableState);
    m_browser->saveSDButton->setEnabled(enableState);
    m_browser->readSDButton->setEnabled(enableState);
    m_browser->eraseSDButton->setEnabled(enableState);
}

/**
 * @brief UAVObjectBrowserWidget::searchTextChanged Looks for matching text in the UAVO fields
 */
void UAVObjectBrowserWidget::searchTextChanged(QString searchText)
{
    proxyModel->setFilterRegExp(QRegExp(searchText, Qt::CaseInsensitive, QRegExp::FixedString));
}

void UAVObjectBrowserWidget::searchTextCleared()
{
    m_browser->le_searchField->clear();
}

void UAVObjectBrowserWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        searchTextCleared();
        Q_FALLTHROUGH();
    default:
        QWidget::keyPressEvent(e);
    }
}

void UAVObjectBrowserWidget::keyReleaseEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        searchTextCleared();
        Q_FALLTHROUGH();
    default:
        QWidget::keyReleaseEvent(e);
    }
}

//============================

/**
 * @brief UAVOBrowserTreeView::UAVOBrowserTreeView Constructor for reimplementation of QTreeView
 */
UAVOBrowserTreeView::UAVOBrowserTreeView(unsigned int updateTimerPeriod)
    : QTreeView()
    , m_updateTreeViewFlag(false)
{
    // Start timer at 100ms
    m_updateViewTimer.start(updateTimerPeriod);

    // Connect the timer
    connect(&m_updateViewTimer, &QTimer::timeout, this, &UAVOBrowserTreeView::onTimeout_updateView);
}

void UAVOBrowserTreeView::updateTimerPeriod(unsigned int val)
{
    if (val == 0) {
        // If val == 0, disable throttling by stopping the timer.
        m_updateViewTimer.stop();
    } else {
        // If the UAVO has a very fast data rate, then don't go the full speed.
        if (val < 125) {
            val = 125; // Don't try to draw the browser faster than 8Hz.
        }
        m_updateViewTimer.start(val);
    }
}

/**
 * @brief UAVOBrowserTreeView::onTimeout_updateView On timeout, emits dataChanged() signal.
 * Origingally,
 * this was intended to only function on the the data tree indices that had changed since last
 * timeout,
 * but QTreeView does not respect the limits in dataChanged, instead redrawing the entire tree at
 * each
 * update.
 */
void UAVOBrowserTreeView::onTimeout_updateView()
{
    if (m_updateTreeViewFlag == true) {
        QModelIndex topLeftIndex = model()->index(0, 0);
        QModelIndex bottomRightIndex = model()->index(1, 1);

        QTreeView::dataChanged(topLeftIndex, bottomRightIndex);
    }

    m_updateTreeViewFlag = false;
}

/**
 * @brief UAVOBrowserTreeView::updateView Determines if a view updates lies outside the
 * range of updates queued for update
 * @param topLeft Top left index from data model update
 * @param bottomRight Bottom right index from data model update
 */
void UAVOBrowserTreeView::updateView(const QModelIndex &topLeftProxy,
                                     const QModelIndex &bottomRightProxy)
{
    Q_UNUSED(bottomRightProxy);

    // First static_cast from *void to a tree item pointer. This is safe because we know all the
    // indices are tree items
    QModelIndex topLeftModel = proxyModel->mapToSource(topLeftProxy);
    TreeItem *treeItemPtr = static_cast<TreeItem *>(topLeftModel.internalPointer());

    // Second, do a dynamic_cast in order to detect if this tree item is a data object
    DataObjectTreeItem *dataObjectTreeItemPtr = dynamic_cast<DataObjectTreeItem *>(treeItemPtr);

    if (dataObjectTreeItemPtr == NULL) {
        // Do nothing. These QModelIndices are generated by the highlight manager for individual
        // UAVO fields, which are both updated when updating that UAVO's branch of the settings or
        // dynamic data tree.
        return;
    }

    m_updateTreeViewFlag = true;
}

void UAVOBrowserTreeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                      const QVector<int> &roles)
{
    Q_UNUSED(roles);

    // If the timer is active, then throttle updates...
    if (m_updateViewTimer.isActive()) {
        updateView(topLeft, bottomRight);
    } else { // ... otherwise pass them directly on to the treeview.
        QTreeView::dataChanged(topLeft, bottomRight);
    }
}

//============================

TreeSortFilterProxyModel::TreeSortFilterProxyModel(QObject *p)
    : QSortFilterProxyModel(p)
{
    Q_ASSERT(p);
}

/**
 * @brief TreeSortFilterProxyModel::filterAcceptsRow  Taken from
 * http://qt-project.org/forums/viewthread/7782. This proxy model
 * will accept rows:
 *   - That match themselves, or
 *   - That have a parent that matches (on its own), or
 *   - That have a child that matches.
 * @param sourceRow
 * @param sourceParent
 * @return
 */
bool TreeSortFilterProxyModel::filterAcceptsRow(int source_row,
                                                const QModelIndex &source_parent) const
{
    if (filterAcceptsRowItself(source_row, source_parent))
        return true;

    // accept if any of the parents is accepted on it's own merits
    QModelIndex parent = source_parent;
    while (parent.isValid()) {
        if (filterAcceptsRowItself(parent.row(), parent.parent()))
            return true;
        parent = parent.parent();
    }

    // accept if any of the children is accepted on it's own merits
    if (hasAcceptedChildren(source_row, source_parent)) {
        return true;
    }

    return false;
}

bool TreeSortFilterProxyModel::filterAcceptsRowItself(int source_row,
                                                      const QModelIndex &source_parent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool TreeSortFilterProxyModel::hasAcceptedChildren(int source_row,
                                                   const QModelIndex &source_parent) const
{
    QModelIndex item = sourceModel()->index(source_row, 0, source_parent);
    if (!item.isValid()) {
        // qDebug() << "item invalid" << source_parent << source_row;
        return false;
    }

    // check if there are children
    int childCount = item.model()->rowCount(item);
    if (childCount == 0)
        return false;

    for (int i = 0; i < childCount; ++i) {
        if (filterAcceptsRowItself(i, item))
            return true;
        // recursive call -> NOTICE that this is depth-first searching, you're probably better off
        // with breadth first search...
        if (hasAcceptedChildren(i, item))
            return true;
    }

    return false;
}
