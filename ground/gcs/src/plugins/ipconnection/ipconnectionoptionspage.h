/**
 ******************************************************************************
 *
 * @file       IPconnectionoptionspage.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup IPConnPlugin IP Telemetry Plugin
 * @{
 * @brief IP Connection Plugin impliment telemetry over TCP/IP and UDP/IP
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

#ifndef IPconnectionOPTIONSPAGE_H
#define IPconnectionOPTIONSPAGE_H

#include "coreplugin/dialogs/ioptionspage.h"
#include "ipconnectionconfiguration.h"
#include <QAbstractTableModel>
#include <QStyledItemDelegate>

class IPConnectionConfiguration;

namespace Core
{
class IUAVGadgetConfiguration;
}

namespace Ui
{
class IPconnectionOptionsPage;
}

using namespace Core;

class IPConnectionOptionsModel;

class IPConnectionOptionsPage : public IOptionsPage
{
    Q_OBJECT
public:
    explicit IPConnectionOptionsPage(IPConnectionConfiguration *config,
                                     QObject *parent = 0);
    virtual ~IPConnectionOptionsPage();

    QString id() const { return QLatin1String("settings"); }
    QString trName() const { return tr("settings"); }
    QString category() const { return "IP Network Telemetry"; };
    QString trCategory() const { return "IP Network Telemetry"; };

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

    enum Columns {
        ColumnProtocol = 0,
        ColumnHostname,
        ColumnPort,
        ColumnCount
    };

signals:
    void availableDevChanged();

public slots:
private:
    IPConnectionConfiguration *m_config;
    Ui::IPconnectionOptionsPage *m_page;
    IPConnectionOptionsModel *m_model;
};

class IPConnectionOptionsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit IPConnectionOptionsDelegate(QObject *parent = nullptr)
            : QStyledItemDelegate(parent)
    {
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor,
                       const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
};

class IPConnectionOptionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit IPConnectionOptionsModel(QObject *parent = nullptr)
            : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::EditRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole);
    bool insertRows(int position, int rows,
                    const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows,
                    const QModelIndex &index = QModelIndex());

    QVector<IPConnectionConfiguration::Host> &hosts() { return m_hosts; }

private:
    QVector<IPConnectionConfiguration::Host> m_hosts;
};

#endif // IPconnectionOPTIONSPAGE_H
