/**
 ******************************************************************************
 * @file       scaledpixmaplabel.cpp
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

#include "scaledpixmaplabel.h"


ScaledPixmapLabel::ScaledPixmapLabel(QWidget *parent, Qt::WindowFlags f) :
    QLabel(parent, f)
{
    setScaledContents(false);
}

int ScaledPixmapLabel::heightForWidth(int width) const
{
    if (fullPixmap.isNull())
        return -1;
    return static_cast<int>(static_cast<qreal>(width) * fullPixmap.height()  / fullPixmap.width());
}

QPixmap ScaledPixmapLabel::scaledPixmap() const
{
    return fullPixmap.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QSize ScaledPixmapLabel::sizeHint() const
{
    if (fullPixmap.isNull())
        return QSize();
    int w = std::min(width(), static_cast<int>(fullPixmap.width() / fullPixmap.devicePixelRatio()));
    return QSize(w, heightForWidth(w));
}

void ScaledPixmapLabel::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    if (!fullPixmap.isNull())
        QLabel::setPixmap(scaledPixmap());
}

void ScaledPixmapLabel::setPixmap(const QPixmap &pixmap)
{
    fullPixmap = pixmap;
    if (fullPixmap.isNull())
        QLabel::setPixmap(pixmap);
    else
        QLabel::setPixmap(scaledPixmap());
}

/**
 * @}
 * @}
 */
