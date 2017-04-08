/**
 ******************************************************************************
 *
 * @file       IPconnectionoptionspage.cpp
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

#include "ipconnectionoptionspage.h"
#include "ipconnectionconfiguration.h"
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>

#include "ui_ipconnectionoptionspage.h"

IPConnectionOptionsPage::IPConnectionOptionsPage(IPConnectionConfiguration *config, QObject *parent)
    : IOptionsPage(parent)
    , m_config(config)
{
}

IPConnectionOptionsPage::~IPConnectionOptionsPage()
{
}

QWidget *IPConnectionOptionsPage::createPage(QWidget *parent)
{

    m_page = new Ui::IPconnectionOptionsPage();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    m_model = new IPConnectionOptionsModel(this);

    for (const auto &host : m_config->hosts()) {
        m_model->insertRow(m_model->rowCount());
        int row = m_model->rowCount() - 1;
        m_model->setData(m_model->index(row, ColumnProtocol), QVariant::fromValue(host.protocol));
        m_model->setData(m_model->index(row, ColumnHostname), host.hostname);
        m_model->setData(m_model->index(row, ColumnPort), host.port);
    }

    // make the columns take up full width
    m_page->tblHosts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_page->tblHosts->setItemDelegate(new IPConnectionOptionsDelegate(m_page->tblHosts));
    m_page->tblHosts->setModel(m_model);

    connect(m_page->btnAdd, &QPushButton::clicked, this,
            [this]() { m_model->insertRow(m_model->rowCount()); });

    connect(m_page->btnRemove, &QPushButton::clicked, this, [this]() {
        const auto selection = m_page->tblHosts->selectionModel()->selectedIndexes();
        if (selection.count() && selection.at(0).isValid())
            m_page->tblHosts->model()->removeRow(selection.at(0).row());
    });

    return w;
}

void IPConnectionOptionsPage::apply()
{
    // discard invalid rows
    for (int row = 0; row < m_model->rowCount();) {
        auto hostname = m_model->data(m_model->index(row, ColumnHostname)).toString();
        auto port = m_model->data(m_model->index(row, ColumnPort)).toInt();
        if (!hostname.length() || port < 1 || port > 65535)
            m_model->removeRow(row);
        else
            row++;
    }
    m_config->setHosts(m_model->hosts());
    m_config->saveConfig();
    emit availableDevChanged();
}

void IPConnectionOptionsPage::finish()
{
    delete m_page;
}

QWidget *IPConnectionOptionsDelegate::createEditor(QWidget *parent,
                                                   const QStyleOptionViewItem &option,
                                                   const QModelIndex &index) const
{
    Q_UNUSED(option)

    switch (index.column()) {
    case IPConnectionOptionsPage::ColumnProtocol: {
        auto editor = new QComboBox(parent);
        editor->addItem("TCP", IPConnectionConfiguration::ProtocolTcp);
        editor->addItem("UDP", IPConnectionConfiguration::ProtocolUdp);
        editor->setFrame(false);
        return editor;
    }
    case IPConnectionOptionsPage::ColumnHostname: {
        auto editor = new QLineEdit(parent);
        editor->setFrame(false);
        return editor;
    }
    case IPConnectionOptionsPage::ColumnPort: {
        auto editor = new QSpinBox(parent);
        editor->setMinimum(1);
        editor->setMaximum(65535);
        editor->setFrame(false);
        return editor;
    }
    default:
        Q_ASSERT(false);
    }
    return nullptr;
}

void IPConnectionOptionsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant val = index.model()->data(index, Qt::EditRole);

    switch (index.column()) {
    case IPConnectionOptionsPage::ColumnProtocol: {
        auto comboBox = static_cast<QComboBox *>(editor);
        for (int i = 0; i < comboBox->count(); i++) {
            if (comboBox->itemData(i) == val.value<IPConnectionConfiguration::Protocol>())
                comboBox->setCurrentIndex(i);
        }
        break;
    }
    case IPConnectionOptionsPage::ColumnHostname: {
        auto lineEdit = static_cast<QLineEdit *>(editor);
        lineEdit->setText(val.toString());
        break;
    }
    case IPConnectionOptionsPage::ColumnPort: {
        auto spinBox = static_cast<QSpinBox *>(editor);
        spinBox->setValue(val.toInt());
        break;
    }
    default:
        Q_ASSERT(false);
    }
}

void IPConnectionOptionsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                               const QModelIndex &index) const
{
    QVariant val;

    switch (index.column()) {
    case IPConnectionOptionsPage::ColumnProtocol: {
        auto comboBox = static_cast<QComboBox *>(editor);
        val.setValue(QVariant::fromValue(comboBox->currentData()));
        break;
    }
    case IPConnectionOptionsPage::ColumnHostname: {
        auto lineEdit = static_cast<QLineEdit *>(editor);
        val.setValue(lineEdit->text());
        break;
    }
    case IPConnectionOptionsPage::ColumnPort: {
        auto spinBox = static_cast<QSpinBox *>(editor);
        spinBox->interpretText();
        val.setValue(spinBox->value());
        break;
    }
    default:
        Q_ASSERT(false);
        return;
    }

    model->setData(index, val, Qt::EditRole);
}

void IPConnectionOptionsDelegate::updateEditorGeometry(QWidget *editor,
                                                       const QStyleOptionViewItem &option,
                                                       const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

int IPConnectionOptionsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_hosts.count();
}

int IPConnectionOptionsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return IPConnectionOptionsPage::ColumnCount;
}

QVariant IPConnectionOptionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_hosts.count())
        return QVariant();

    const IPConnectionConfiguration::Host host = m_hosts.at(index.row());

    if (index.column() == IPConnectionOptionsPage::ColumnProtocol && role == Qt::DisplayRole)
        return host.protocol == IPConnectionConfiguration::ProtocolTcp ? "TCP" : "UDP";

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case IPConnectionOptionsPage::ColumnProtocol:
            return QVariant::fromValue(host.protocol);
        case IPConnectionOptionsPage::ColumnHostname:
            return host.hostname;
        case IPConnectionOptionsPage::ColumnPort:
            return host.port;
        default:
            Q_ASSERT(false);
        }
    }

    return QVariant();
}

QVariant IPConnectionOptionsModel::headerData(int section, Qt::Orientation orientation,
                                              int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case IPConnectionOptionsPage::ColumnProtocol:
            return tr("Protocol");
        case IPConnectionOptionsPage::ColumnHostname:
            return tr("IP/Hostname");
        case IPConnectionOptionsPage::ColumnPort:
            return tr("Port");
        default:
            return QVariant();
        }
    }
    return QString::number(section + 1);
}

Qt::ItemFlags IPConnectionOptionsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool IPConnectionOptionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        auto &host = m_hosts[index.row()];
        switch (index.column()) {
        case IPConnectionOptionsPage::ColumnProtocol:
            host.protocol = value.value<IPConnectionConfiguration::Protocol>();
            break;
        case IPConnectionOptionsPage::ColumnHostname:
            host.hostname = value.toString();
            break;
        case IPConnectionOptionsPage::ColumnPort:
            host.port = value.toInt();
            break;
        default:
            return false;
        }
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

bool IPConnectionOptionsModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index)

    beginInsertRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; row++)
        m_hosts.insert(position, IPConnectionConfiguration::Host());

    endInsertRows();
    return true;
}

bool IPConnectionOptionsModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index)

    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; row++)
        m_hosts.removeAt(position);

    endRemoveRows();
    return true;
}
