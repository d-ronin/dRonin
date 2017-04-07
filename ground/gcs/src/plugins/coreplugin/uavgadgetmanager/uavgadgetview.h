/**
 ******************************************************************************
 *
 * @file       uavgadgetview.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C)
 *2009.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup CorePlugin Core Plugin
 * @{
 * @brief The Core GCS plugin
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

#ifndef UAVGADGETVIEW_H
#define UAVGADGETVIEW_H

#include <QAction>
#include <QSplitter>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QSettings>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
class QLabel;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils
{
class StyledBar;
}

namespace Core
{

class IUAVGadget;
class UAVGadgetManager;

namespace Internal
{

class UAVGadgetView : public QWidget
{
    Q_OBJECT

public:
    UAVGadgetView(UAVGadgetManager *uavGadgetManager, IUAVGadget *uavGadget = 0,
                  QWidget *parent = 0, bool restoring = false);
    virtual ~UAVGadgetView();
    void selectionActivated(int index, bool forceLoadConfiguration);
    void removeGadget();
    IUAVGadget *gadget() const;
    void setGadget(IUAVGadget *uavGadget);
    bool hasGadget(IUAVGadget *uavGadget) const;
    int indexOfClassId(QString classId);
    void showToolbar(bool show);

public slots:
    void closeView();
    void doReplaceGadget(int index);

private slots:
    void currentGadgetChanged(IUAVGadget *gadget);

private:
    void updateToolBar();
    QPointer<UAVGadgetManager> m_uavGadgetManager;
    QPointer<IUAVGadget> m_uavGadget;
    QWidget *m_toolBar;
    QComboBox *m_defaultToolBar;
    QWidget *m_currentToolBar;
    QWidget *m_activeToolBar;
    QComboBox *m_uavGadgetList;
    QToolButton *m_closeButton;
    Utils::StyledBar *m_top;
    QVBoxLayout *tl; // top layout
    int m_defaultIndex;
    QLabel *m_activeLabel;
};
}
}
#endif // UAVGADGETVIEW_H
