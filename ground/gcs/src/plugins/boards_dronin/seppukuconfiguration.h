/**
 ******************************************************************************
 * @file       seppukuconfiguration.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_dRonin dRonin board support plugin
 * @{
 * @brief Supports dRonin board configuration
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

#ifndef SEPPUKUCONFIGURATION_H
#define SEPPUKUCONFIGURATION_H

#include <QSvgRenderer>
#include <QGraphicsSvgItem>

#include "configtaskwidget.h"
#include "ui_seppuku.h"

class SeppukuConfiguration : public ConfigTaskWidget
{
    Q_OBJECT
public:
    SeppukuConfiguration(QWidget *parent = 0);
    ~SeppukuConfiguration();

private:
    Ui::Seppuku *ui;
    QSvgRenderer *m_renderer;
    QGraphicsSvgItem *m_background;
    QGraphicsSvgItem *m_uart3;
    QGraphicsSvgItem *m_uart4;
    QGraphicsScene *m_scene;

    void setupGraphicsScene();
    QGraphicsSvgItem *addGraphicsElement(const QString &elementId);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void setMessage(const QString &name, const QString &msg = QString(), const QString &severity = QString("warning"));

private slots:
    void magChanged(const QString &newVal);
    void outputsChanged(const QString &newVal);
    void checkExtMag();
    void checkDsm();
    void checkUart3(const QString &newVal);
    void checkRcvr(const QString &newVal);
};

#endif // SEPPUKUCONFIGURATION_H

/**
 * @}
 * @}
 */
