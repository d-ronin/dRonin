/**
 ******************************************************************************
 *
 * @file       connectiondiagram.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @see        The GNU Public License (GPL) Version 3
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup SetupWizard Setup Wizard
 * @{
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#ifndef CONNECTIONDIAGRAM_H
#define CONNECTIONDIAGRAM_H

#include "uavobjectwidgetutils_global.h"

#include <QDialog>
#include <QHash>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <uavobjects/uavobjectmanager.h>
#include <uavobjectutil/uavobjectutilmanager.h>

namespace Ui {
class ConnectionDiagram;
}

class UAVOBJECTWIDGETUTILS_EXPORT ConnectionDiagram : public QDialog {
    Q_OBJECT

public:
    explicit ConnectionDiagram(QWidget *parent = Q_NULLPTR);
    ~ConnectionDiagram();

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

private:
    Ui::ConnectionDiagram *ui;

    QSvgRenderer *m_renderer;
    QGraphicsSvgItem *m_background;
    QGraphicsScene *m_scene;

    UAVObjectManager *uavoMngr;
    UAVObjectUtilManager *utilMngr;

    const QString ENUM_SPECIAL_CHARS = "[\\.\\-\\s\\+/\\(\\)]";

    void setupGraphicsScene();
    void setupGraphicsSceneItems(QStringList elementsToShow);
    void addUavoFieldElements(QStringList &elements, const QString &objName, const QString &fieldName, const QString &prefix = "");

private slots:
    void saveToFile();
};

#endif // CONNECTIONDIAGRAM_H

/**
 * @}
 * @}
 */
