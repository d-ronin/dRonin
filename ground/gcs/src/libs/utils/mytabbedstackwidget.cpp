/**
 ******************************************************************************
 *
 * @file       mytabbedstackwidget.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
 * @{
 *
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

#include "mytabbedstackwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QtCore/QDebug>

MyTabbedStackWidget::MyTabbedStackWidget(QWidget *parent, bool isVertical, bool iconAbove)
    : QWidget(parent),
      m_vertical(isVertical),
      m_iconAbove(iconAbove)
{
    m_listWidget = new QListWidget(this);
    m_stackWidget = new QStackedWidget();
    m_stackWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QBoxLayout *toplevelLayout;
    if (m_vertical) {
        toplevelLayout = new QHBoxLayout;
        toplevelLayout->addWidget(m_listWidget);
        toplevelLayout->addWidget(m_stackWidget);
        m_listWidget->setFlow(QListView::TopToBottom);
        m_listWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    } else {
        toplevelLayout = new QVBoxLayout;
        toplevelLayout->addWidget(m_stackWidget);
        toplevelLayout->addWidget(m_listWidget);
        m_listWidget->setFlow(QListView::LeftToRight);
        m_listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    if (m_iconAbove && m_vertical) {
        m_listWidget->setFixedWidth(91); // this should be computed instead
        m_listWidget->setWrapping(false);
        // make the scrollbar small and similar to OS X so it doesn't overlay the icons
        m_listWidget->setStyleSheet(QString("QScrollBar:vertical { width: 6px; border-width: 0px; background: none; margin: 2px 0px 2px 0px; }"
                                            "QScrollBar::handle:vertical { background: #5c5c5c; border-radius: 3px; }"
                                            "QScrollBar::add-line:vertical { width: 0; height: 0; }"
                                            "QScrollBar::sub-line:vertical { width: 0; height: 0 }"));
    }

    toplevelLayout->setSpacing(0);
    toplevelLayout->setContentsMargins(0, 0, 0, 0);
    m_listWidget->setContentsMargins(0, 0, 4, 0);
    m_listWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_listWidget->setSpacing(4);
    m_listWidget->setViewMode(QListView::IconMode);
    m_stackWidget->setContentsMargins(0, 0, 0, 0);
    setLayout(toplevelLayout);

    connect(m_listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(showWidget(int)),Qt::QueuedConnection);
}

void MyTabbedStackWidget::insertTab(const int index, QWidget *tab, const QIcon &icon, const QString &label)
{
    tab->setContentsMargins(0, 0, 0, 0);
    m_stackWidget->insertWidget(index, tab);
    QListWidgetItem *item = new QListWidgetItem(icon, label);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    item->setToolTip(label);

    if (m_vertical) {
        item->setSizeHint(QSize(85, 82));
    }

    m_listWidget->insertItem(index, item);
}

void MyTabbedStackWidget::removeTab(int index)
{
    QWidget * wid=m_stackWidget->widget(index);
    m_stackWidget->removeWidget(wid);
    delete wid;
    QListWidgetItem *item = m_listWidget->item(index);
    m_listWidget->removeItemWidget(item);
    delete item;
}

int MyTabbedStackWidget::currentIndex() const
{
    return m_listWidget->currentRow();
}

void MyTabbedStackWidget::setCurrentIndex(int index)
{
    m_listWidget->setCurrentRow(index);
}

void MyTabbedStackWidget::showWidget(int index)
{
    if(m_stackWidget->currentIndex()==index)
        return;
    bool proceed=false;
    emit currentAboutToShow(index,&proceed);
    if(proceed)
    {
        m_stackWidget->setCurrentIndex(index);
        emit currentChanged(index);
    }
    else
    {
        m_listWidget->setCurrentRow(m_stackWidget->currentIndex(),QItemSelectionModel::ClearAndSelect);
    }
}

void MyTabbedStackWidget::insertCornerWidget(int index, QWidget *widget)
{
    Q_UNUSED(index);

    widget->hide();
}

void MyTabbedStackWidget::setHidden(int index, bool hide)
{
    QListWidgetItem *i = m_listWidget->item(index);

    if (!i) {
        return;
    }

    i->setHidden(hide);
}
