/**
 ******************************************************************************
 * @file       characterosd.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup [Group]
 * @{
 * @addtogroup CharacterOSD
 * @{
 * @brief [Brief]
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

#ifndef CHARACTEROSD_H
#define CHARACTEROSD_H

#include <uavobjectwidgetutils/configtaskwidget.h>

// TODO gone
#include <QDebug>

namespace Ui {
class CharacterOSD;
}

class CharacterPanel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(int xPos READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int yPos READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(int width READ width NOTIFY widthChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(int index READ index NOTIFY indexChanged)

public:
    explicit CharacterPanel(int index, QObject *parent = nullptr) : QObject(parent)
      , m_type(QStringLiteral("Disabled"))
      , m_index(index)
    {
    }

    QString type() const { return m_type; }
    void setType(const QString &type) {
        if (m_type != type) {
            m_type = type;
            Q_EMIT typeChanged(type);
            qDebug() << "type changed" << type;
        }
    }
    int x() const { return m_x; }
    void setX(int x) { m_x = x; Q_EMIT xChanged(x); }
    int y() const { return m_y; }
    void setY(int y) { m_y = y; Q_EMIT yChanged(y); }
    int width() const { return m_width; }
    void setWidth(int width) { m_width = width; Q_EMIT widthChanged(width); }
    int height() const { return m_height; }
    void setHeight(int height) { m_height = height; Q_EMIT heightChanged(height); }
    int index() const { return m_index; }

Q_SIGNALS:
    void typeChanged(QString type);
    void xChanged(int x);
    void yChanged(int y);
    void widthChanged(int width);
    void heightChanged(int height);
    void indexChanged(int index);

private:
    QString m_type;
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    int m_index;
};

class CharacterOSD : public ConfigTaskWidget
{
    Q_OBJECT

public:
    explicit CharacterOSD(QWidget *parent = nullptr);
    ~CharacterOSD();

Q_SIGNALS:
    void panelAdded(int index);
    void panelDeleted(int index);

private Q_SLOTS:
    void refreshFromUavo();
    void onSelectionChanged(int index);
    void onPanelTypeChanged(const QString &type);
    void onXChanged(int x);
    void onYChanged(int y);
    void onAddPanel();
    void onDeletePanel();

private:
    Ui::CharacterOSD *ui;
    QList<QObject *> m_panels; // needs to be QList<QObject *> for qml list model
    QVector<QWidget *> m_panelWidgets;
    static const QMap<QString, QPair<int, int>> m_sizes;
    int m_maxPanels;
    int m_disabledPanels;
};

#endif // CHARACTEROSD_H

/**
 * @}
 * @}
 */
