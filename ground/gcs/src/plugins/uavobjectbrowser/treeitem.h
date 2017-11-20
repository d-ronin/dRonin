/**
 ******************************************************************************
 *
 * @file       treeitem.h
 *
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
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

#ifndef TREEITEM_H
#define TREEITEM_H

#include "uavobjects/uavobject.h"
#include "uavobjects/uavdataobject.h"
#include "uavobjects/uavmetaobject.h"
#include "uavobjects/uavobjectfield.h"
#include <QtCore/QList>
#include <QtCore/QLinkedList>
#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QObject>
#include <QtCore/QDebug>

class TreeItem;

/*
* Small utility class that handles the higlighting of
* tree grid items.
* Basicly it maintains all items due to be restored to
* non highlighted state in a linked list.
* A timer traverses this list periodically to find out
* if any of the items should be restored. All items are
* updated withan expiration timestamp when they expires.
* An item that is beeing restored is removed from the
* list and its removeHighlight() method is called. Items
* that are not expired are left in the list til next time.
* Items that are updated during the expiration time are
* left untouched in the list. This reduces unwanted emits
* of signals to the repaint/update function.
*/
class HighLightManager : public QObject
{
    Q_OBJECT
public:
    // Constructor taking the checking interval in ms.
    HighLightManager(long checkingInterval);

    // This is called when an item has been set to
    // highlighted = true.
    bool add(TreeItem *itemToAdd);

signals:
    void updateHighlight(TreeItem *);

private slots:
    // Timer callback method.
    void checkItemsExpired();

private:
    // The timer checking highlight expiration.
    QTimer m_expirationTimer;

    // The sets holding all items due to be updated.
    QSet<TreeItem *> m_itemsToHighlight;
    QSet<TreeItem *> m_itemsWaiting;
    QSet<TreeItem *> m_itemsToUnhighlight;
};

class TreeItem : public QObject
{
    Q_OBJECT
protected:
    bool isPresentOnHardware;

public:
    TreeItem(const QList<QVariant> &data, TreeItem *parent = 0);
    TreeItem(const QVariant &data, TreeItem *parent = 0);
    virtual ~TreeItem();

    virtual void appendChild(TreeItem *child);
    virtual void removeChild(TreeItem *child);
    void insertChild(TreeItem *child);

    TreeItem *getChild(int index);
    inline QList<TreeItem *> treeChildren() const { return m_children; }
    int childCount() const;
    int columnCount() const;
    QVariant data(int column = 1) const;
    QString description() { return m_description; }
    void setDescription(QString d)
    {
        d = d.trimmed().toHtmlEscaped();
        if (!d.isEmpty()) {
            // insert html tags to make this rich text so Qt will take care of wrapping
            d.prepend("<span style='font-style: normal'>");
            d.remove("@Ref", Qt::CaseInsensitive);
            d.append("</span>");
            m_description = d;
        }
    }
    // only column 1 (TreeItem::dataColumn) is changed with setData currently
    // other columns are initialized in constructor
    virtual void setData(QVariant value, int column = 1);
    int row() const;
    TreeItem *parent() { return m_parent; }
    void setParentTree(TreeItem *parent) { m_parent = parent; }
    inline virtual bool isEditable() { return false; }
    virtual void update();
    virtual void apply();

    inline bool highlighted() { return m_highlight; }
    void setHighlight();

    inline bool changed() { return m_changed; }
    inline bool updatedOnly() { return m_updated; }
    inline virtual bool getIsPresentOnHardware() const { return isPresentOnHardware; }
    virtual void setIsPresentOnHardware(bool value)
    {
        foreach (TreeItem *item, treeChildren()) {
            item->setIsPresentOnHardware(value);
        }
        isPresentOnHardware = value;
    }
    inline void setChanged(bool changed) { m_changed = changed; }
    void setUpdatedOnly(bool updated);
    void setUpdatedOnlyParent();
    virtual void setHighlightManager(HighLightManager *mgr);

    virtual void removeHighlight();

    int nameIndex(QString name)
    {
        for (int i = 0; i < childCount(); ++i) {
            if (name < getChild(i)->data(0).toString())
                return i;
        }
        return childCount();
    }

    TreeItem *findChildByName(QString name)
    {
        foreach (TreeItem *child, m_children) {
            if (name == child->data(0).toString()) {
                return child;
            }
        }
        return 0;
    }

    virtual bool isDefaultValue() const { return true; }

protected:
    bool childrenAreDefaultValue() const {
        bool ret = true;
        for (const auto item : treeChildren())
            ret &= item->isDefaultValue();
        return ret;
    }

private slots:

private:
    QList<TreeItem *> m_children;
    // m_data contains: [0] property name, [1] value, [2] unit
    QList<QVariant> m_data;
    QString m_description;
    TreeItem *m_parent;
    bool m_highlight;
    bool m_changed;
    bool m_updated;
    HighLightManager *m_highlightManager;
    static int m_highlightTimeMs;

public:
    static const int dataColumn = 1;
};

class DataObjectTreeItem;
class MetaObjectTreeItem;

class TopTreeItem : public TreeItem
{
    Q_OBJECT
public:
    TopTreeItem(const QList<QVariant> &data, TreeItem *parent = 0)
        : TreeItem(data, parent)
    {
    }
    TopTreeItem(const QVariant &data, TreeItem *parent = 0)
        : TreeItem(data, parent)
    {
    }

    void addObjectTreeItem(quint32 objectId, DataObjectTreeItem *oti)
    {
        m_objectTreeItemsPerObjectIds[objectId] = oti;
    }

    DataObjectTreeItem *findDataObjectTreeItemByObjectId(quint32 objectId)
    {
        return m_objectTreeItemsPerObjectIds.contains(objectId)
            ? m_objectTreeItemsPerObjectIds[objectId]
            : 0;
    }

    void addMetaObjectTreeItem(quint32 objectId, MetaObjectTreeItem *oti)
    {
        m_metaObjectTreeItemsPerObjectIds[objectId] = oti;
    }

    MetaObjectTreeItem *findMetaObjectTreeItemByObjectId(quint32 objectId)
    {
        return m_metaObjectTreeItemsPerObjectIds.contains(objectId)
            ? m_metaObjectTreeItemsPerObjectIds[objectId]
            : 0;
    }
    QList<MetaObjectTreeItem *> getMetaObjectItems();
    QList<DataObjectTreeItem *> getDataObjectItems();

private:
    QMap<quint32, DataObjectTreeItem *> m_objectTreeItemsPerObjectIds;
    QMap<quint32, MetaObjectTreeItem *> m_metaObjectTreeItemsPerObjectIds;
};

class ObjectTreeItem : public TreeItem
{
    Q_OBJECT
public:
    ObjectTreeItem(const QList<QVariant> &data, TreeItem *parent = 0)
        : TreeItem(data, parent)
        , m_obj(0)
    {
    }
    ObjectTreeItem(const QVariant &data, TreeItem *parent = 0)
        : TreeItem(data, parent)
        , m_obj(0)
    {
    }
    virtual void setObject(UAVObject *obj)
    {
        m_obj = obj;
        setDescription(obj->getDescription());
    }
    inline UAVObject *object() { return m_obj; }

private:
    UAVObject *m_obj;
};

class MetaObjectTreeItem : public ObjectTreeItem
{
    Q_OBJECT
public:
    MetaObjectTreeItem(UAVObject *obj, const QList<QVariant> &data, TreeItem *parent = 0)
        : ObjectTreeItem(data, parent)
    {
        setObject(obj);
    }
    MetaObjectTreeItem(UAVObject *obj, const QVariant &data, TreeItem *parent = 0)
        : ObjectTreeItem(data, parent)
    {
        setObject(obj);
    }
};

class DataObjectTreeItem : public ObjectTreeItem
{
    Q_OBJECT
public:
    DataObjectTreeItem(const QList<QVariant> &data, TreeItem *parent = 0)
        : ObjectTreeItem(data, parent)
    {
    }
    DataObjectTreeItem(const QVariant &data, TreeItem *parent = 0)
        : ObjectTreeItem(data, parent)
    {
    }
    virtual void apply() override
    {
        foreach (TreeItem *child, treeChildren()) {
            MetaObjectTreeItem *metaChild = dynamic_cast<MetaObjectTreeItem *>(child);
            if (!metaChild)
                child->apply();
        }
    }
    virtual void update() override
    {
        foreach (TreeItem *child, treeChildren()) {
            MetaObjectTreeItem *metaChild = dynamic_cast<MetaObjectTreeItem *>(child);
            if (!metaChild)
                child->update();
        }
    }
    virtual void appendChild(TreeItem *child)  override
    {
        ObjectTreeItem::appendChild(child);
        child->setIsPresentOnHardware(isPresentOnHardware);
    }
    virtual void setObject(UAVObject *obj) override
    {
        UAVDataObject *dobj = dynamic_cast<UAVDataObject *>(obj);
        if (dobj) {
            ObjectTreeItem::setObject(obj);
            connect(dobj, QOverload<UAVDataObject *>::of(&UAVDataObject::presentOnHardwareChanged),
                    this, &DataObjectTreeItem::doRefreshHiddenObjects);
            doRefreshHiddenObjects(dobj);
        } else {
            foreach (TreeItem *item, treeChildren()) {
                DataObjectTreeItem *inst = dynamic_cast<DataObjectTreeItem *>(item);
                if (!inst)
                    item->setIsPresentOnHardware(false);
            }
            isPresentOnHardware = false;
        }
    }
    virtual void setIsPresentOnHardware(bool value) override
    {
        foreach (TreeItem *item, treeChildren()) {
            if (!dynamic_cast<DataObjectTreeItem *>(item))
                item->setIsPresentOnHardware(value);
        }
        isPresentOnHardware = value;
    }

    virtual bool isDefaultValue() const override { return childrenAreDefaultValue(); }


protected slots:
    virtual void doRefreshHiddenObjects(UAVDataObject *dobj)
    {
        foreach (TreeItem *item, treeChildren()) {
            item->setIsPresentOnHardware(dobj->getIsPresentOnHardware());
        }
        isPresentOnHardware = dobj->getIsPresentOnHardware();
    }
};

class InstanceTreeItem : public DataObjectTreeItem
{
    Q_OBJECT
public:
    InstanceTreeItem(UAVObject *obj, const QList<QVariant> &data, TreeItem *parent = 0)
        : DataObjectTreeItem(data, parent)
    {
        setObject(obj);
    }
    InstanceTreeItem(UAVObject *obj, const QVariant &data, TreeItem *parent = 0)
        : DataObjectTreeItem(data, parent)
    {
        setObject(obj);
    }
    virtual void apply() { TreeItem::apply(); }
    virtual void update() { TreeItem::update(); }
protected slots:
    virtual void doRefreshHiddenObjects(UAVDataObject *dobj)
    {
        foreach (TreeItem *item, treeChildren()) {
            item->setIsPresentOnHardware(dobj->getIsPresentOnHardware());
        }
        isPresentOnHardware = dobj->getIsPresentOnHardware();
        bool disable = true;
        if (parent()) {
            if (!isPresentOnHardware)
                parent()->setIsPresentOnHardware(false);
            else {
                foreach (TreeItem *item, parent()->treeChildren()) {
                    InstanceTreeItem *inst = dynamic_cast<InstanceTreeItem *>(item);
                    if (inst && inst->getIsPresentOnHardware())
                        disable = false;
                }
                parent()->setIsPresentOnHardware(!disable);
            }
        }
    }
};

class ArrayFieldTreeItem : public TreeItem
{
    Q_OBJECT
public:
    ArrayFieldTreeItem(const QList<QVariant> &data, TreeItem *parent = 0)
        : TreeItem(data, parent)
    {
    }
    ArrayFieldTreeItem(const QVariant &data, TreeItem *parent = 0)
        : TreeItem(data, parent)
    {
    }

    virtual bool isDefaultValue() const override { return childrenAreDefaultValue(); }
};

#endif // TREEITEM_H
