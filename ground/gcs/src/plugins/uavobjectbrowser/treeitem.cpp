/**
 ******************************************************************************
 *
 * @file       treeitem.cpp
 *
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 *
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

#include "treeitem.h"
#include "fieldtreeitem.h"
#include <math.h>

/* Constructor */
HighLightManager::HighLightManager(long checkingInterval)
{
    // Start the timer and connect it to the callback
    m_expirationTimer.start(checkingInterval);
    connect(&m_expirationTimer, &QTimer::timeout, this, &HighLightManager::checkItemsExpired);
}

/*
 * Called to add item to list. Item is only added if absent.
 * Returns true if item was added, otherwise false.
 */
bool HighLightManager::add(TreeItem *itemToAdd)
{
    if (m_itemsToUnhighlight.remove(itemToAdd)) {
        m_itemsWaiting.insert(itemToAdd);
    }

    if (m_itemsWaiting.contains(itemToAdd)) {
        return false;
    }

    if (m_itemsToHighlight.contains(itemToAdd)) {
        return false;
    }

    m_itemsToHighlight.insert(itemToAdd);

    return true;
}

/*
 * Callback called periodically by the timer.
 * This method checks for expired highlights and
 * removes them if they are expired.
 * Expired highlights are restored.
 */
void HighLightManager::checkItemsExpired()
{
    for (QSet<TreeItem *>::const_iterator i = m_itemsToUnhighlight.begin();
            i != m_itemsToUnhighlight.end(); ++i) {
        TreeItem *item = *i;

        item->removeHighlight();
        emit updateHighlight(item);
    }

    m_itemsToUnhighlight.clear();
    m_itemsToUnhighlight.swap(m_itemsWaiting);

    for (QSet<TreeItem *>::const_iterator i = m_itemsToHighlight.begin();
            i != m_itemsToHighlight.end(); ++i) {
        TreeItem *item = *i;

        emit updateHighlight(item);
    }

    m_itemsWaiting.swap(m_itemsToHighlight);
}

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent)
    : QObject(nullptr)
    , isPresentOnHardware(true)
    , m_data(data)
    , m_parent(parent)
    , m_highlight(false)
    , m_changed(false)
    , m_updated(false)
{
}

TreeItem::TreeItem(const QVariant &data, TreeItem *parent)
    : QObject(nullptr)
    , isPresentOnHardware(true)
    , m_parent(parent)
    , m_highlight(false)
    , m_changed(false)
    , m_updated(false)
{
    m_data << data << ""
           << "";
}

TreeItem::~TreeItem()
{
    qDeleteAll(m_children);
}

void TreeItem::appendChild(TreeItem *child)
{
    m_children.append(child);
    child->setParentTree(this);
}

void TreeItem::removeChild(TreeItem *child)
{
    m_children.removeAll(child);
}

void TreeItem::insertChild(TreeItem *child)
{
    int index = nameIndex(child->data(0).toString());
    m_children.insert(index, child);
    child->setParentTree(this);
}

TreeItem *TreeItem::getChild(int index)
{
    return m_children.value(index);
}

int TreeItem::childCount() const
{
    return m_children.count();
}

int TreeItem::row() const
{
    if (m_parent)
        return m_parent->m_children.indexOf(const_cast<TreeItem *>(this));

    return 0;
}

int TreeItem::columnCount() const
{
    return m_data.count();
}

QVariant TreeItem::data(int column) const
{
    return m_data.value(column);
}

void TreeItem::setData(QVariant value, int column)
{
    m_data.replace(column, value);
}

void TreeItem::update()
{
    foreach (TreeItem *child, treeChildren())
        child->update();
}

void TreeItem::apply()
{
    foreach (TreeItem *child, treeChildren())
        child->apply();
}

/*
 * Called after a value has changed to trigger highlightning of tree item.
 */
void TreeItem::setHighlight()
{
    m_highlight = true;
    m_changed = false;
    // Add to highlightmanager
    
    m_highlightManager->add(this);

    // If we have a parent, call recursively to update highlight status of parents.
    // This will ensure that the root of a leaf that is changed also is highlighted.
    // Only updates that really changes values will trigger highlight of parents.
    if (m_parent) {
        m_parent->setHighlight();
    }
}

void TreeItem::setUpdatedOnly(bool updated)
{
    if (this->changed() && updated) {
        m_updated = updated;
        m_parent->setUpdatedOnlyParent();
    } else if (!updated)
        m_updated = false;
    foreach (TreeItem *item, this->treeChildren()) {
        item->setUpdatedOnly(updated);
    }
}

void TreeItem::setUpdatedOnlyParent()
{
    FieldTreeItem *field = dynamic_cast<FieldTreeItem *>(this);
    TopTreeItem *top = dynamic_cast<TopTreeItem *>(this);
    if (!field && !top) {
        m_updated = true;
        m_parent->setUpdatedOnlyParent();
    }
}

void TreeItem::removeHighlight()
{
    m_highlight = false;
}

void TreeItem::setHighlightManager(HighLightManager *mgr)
{
    m_highlightManager = mgr;
}

QList<MetaObjectTreeItem *> TopTreeItem::getMetaObjectItems()
{
    return m_metaObjectTreeItemsPerObjectIds.values();
}

QList<DataObjectTreeItem *> TopTreeItem::getDataObjectItems()
{
    return m_objectTreeItemsPerObjectIds.values();
}
