/**
 ******************************************************************************
 * @file       scaledpixmaplabel.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup libs GCS Libraries
 * @{
 * @addtogroup utils Utilities
 * @{
 * @brief Provides a QLabel containing a QPixmap that scales with the label
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


#ifndef SCALEDPIXMAPLABEL_H
#define SCALEDPIXMAPLABEL_H

#include <QPixmap>
#include <QLabel>

#include "utils_global.h"

class QTCREATOR_UTILS_EXPORT ScaledPixmapLabel : public QLabel
{
    Q_OBJECT
public:
    /**
     * @brief ScaledPixmapLabel
     * @param parent Parent widget
     * @param f Has the same meaning as for QLabel
     */
    explicit ScaledPixmapLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /**
     * @brief heightForWidth Given a desired width, return the desired height
     * @param width Desired width
     * @return Desired height, or -1 if height doesn't depend on width
     */
    virtual int heightForWidth(int width) const;
    /**
     * @brief scaledPixmap Scale the pixmap to fit desired size
     * @return The scaled pixmap
     */
    QPixmap scaledPixmap() const;
    /**
     * @brief sizeHint Calculate desired size
     * @return Desired size
     */
    virtual QSize sizeHint() const;

protected:
    virtual void resizeEvent(QResizeEvent *event);

public Q_SLOTS:
    /**
     * @brief setPixmap Sets the current pixmap
     * @param pixmap New pixmap
     */
    void setPixmap(const QPixmap &pixmap);

private:
    QPixmap fullPixmap;
};

#endif

/**
 * @}
 * @}
 */
