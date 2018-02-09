/**
 ******************************************************************************
 * @file       characterosd.cpp
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

#include "characterosd.h"
#include "ui_characterosd.h"

#include <uavobjects/uavobjectmanager.h>

#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickItem>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QLabel>

const QMap<QString, QPair<int, int>> CharacterOSD::m_sizes = {
    // {PanelType option, {width, height}}
    {QStringLiteral("Disabled"), {0, 0}},
    {QStringLiteral("Airspeed"), {3, 1}},
    {QStringLiteral("Altitude"), {7, 1}},
    {QStringLiteral("ArmedFlag"), {3, 3}},
    {QStringLiteral("BatteryVolt"), {7, 1}},
    {QStringLiteral("BatteryCurrent"), {7, 1}},
    {QStringLiteral("BatteryConsumed"), {7, 1}},
    {QStringLiteral("Callsign"), {10, 1}},
    {QStringLiteral("Climb"), {7, 1}},
    {QStringLiteral("Compass"), {5, 2}},
    {QStringLiteral("FlightMode"), {6, 3}},
    {QStringLiteral("FlightTime"), {9, 1}},
    {QStringLiteral("GroundSpeed"), {6, 1}},
    {QStringLiteral("GPS"), {6, 1}},
    {QStringLiteral("HomeDistance"), {7, 1}},
    {QStringLiteral("HomeDirection"), {2, 1}},
    {QStringLiteral("Horizon"), {14, 5}},
    {QStringLiteral("Latitude"), {10, 1}},
    {QStringLiteral("Longitude"), {10, 1}},
    {QStringLiteral("Pitch"), {6, 1}},
    {QStringLiteral("Roll"), {6, 1}},
    {QStringLiteral("RSSI"), {3, 1}},
    {QStringLiteral("RSSIFlag"), {1, 1}},
    {QStringLiteral("Temperature"), {8, 1}},
    {QStringLiteral("Throttle"), {6, 1}},
    {QStringLiteral("Crosshair"), {1, 1}},
    {QStringLiteral("Alarms"), {30, 1}},
    {QStringLiteral("Heading"), {5, 1}}
};

CharacterOSD::CharacterOSD(QWidget *parent) :
    ConfigTaskWidget(parent),
    ui(new Ui::CharacterOSD)
{
    ui->setupUi(this);

    addUAVObject(QStringLiteral("CharOnScreenDisplaySettings"));

    auto objMan = getObjectManager();
    auto obj = objMan->getObject(QStringLiteral("CharOnScreenDisplaySettings"));
    auto panels = obj->getField(QStringLiteral("PanelType"));
    m_maxPanels = panels->getNumElements();

    for (int i = 0; i < m_maxPanels; i++) {
        m_panels.append(new CharacterPanel(i, this));
        auto box = new QGroupBox;
        box->setVisible(false);
        auto layout = new QHBoxLayout;
        layout->setSizeConstraint(QLayout::SetFixedSize);
        box->setLayout(layout);
        box->setObjectName(QStringLiteral("frPanel%0").arg(i));
        box->setTitle(tr("Panel %0").arg(i));

        QStringList props = {
            QStringLiteral("objname:CharOnScreenDisplaySettings"),
            QStringLiteral("fieldname:PanelType"),
            QStringLiteral("element:%0").arg(i),
            QStringLiteral("buttongroup:0")
        };
        box->layout()->addWidget(new QLabel(tr("Type:")));
        auto combo = new QComboBox;
        combo->setProperty("objrelation", QVariant::fromValue(props));
        combo->setProperty("index", i);
        connect(combo, &QComboBox::currentTextChanged, this, &CharacterOSD::onPanelTypeChanged);
        box->layout()->addWidget(combo);

        QStringList propsX = {
            QStringLiteral("objname:CharOnScreenDisplaySettings"),
            QStringLiteral("fieldname:X"),
            QStringLiteral("element:%0").arg(i),
            QStringLiteral("buttongroup:0")
        };
        box->layout()->addWidget(new QLabel(tr("X:")));
        auto spinX = new QSpinBox;
        spinX->setProperty("objrelation", propsX);
        spinX->setProperty("index", i);
        connect(spinX, QOverload<int>::of(&QSpinBox::valueChanged), this, &CharacterOSD::onXChanged);
        box->layout()->addWidget(spinX);

        QStringList propsY = {
            QStringLiteral("objname:CharOnScreenDisplaySettings"),
            QStringLiteral("fieldname:Y"),
            QStringLiteral("element:%0").arg(i),
            QStringLiteral("buttongroup:0")
        };
        box->layout()->addWidget(new QLabel(tr("Y:")));
        auto spinY = new QSpinBox;
        spinY->setProperty("objrelation", propsY);
        spinY->setProperty("index", i);
        connect(spinY, QOverload<int>::of(&QSpinBox::valueChanged), this, &CharacterOSD::onYChanged);
        box->layout()->addWidget(spinY);

        auto del = new QPushButton;
        del->setText(tr("Delete Panel"));
        del->setIcon(QIcon(":/core/images/minus.png"));
        del->setProperty("index", i);
        connect(del, &QPushButton::clicked, this, &CharacterOSD::onDeletePanel);
        box->layout()->addWidget(del);

        ui->layPanels->addWidget(box);
        m_panelWidgets.append(box);
    }

    connect(obj, &UAVObject::objectUpdated, this, &CharacterOSD::refreshFromUavo);

    autoLoadWidgets();

    ui->widOSD->rootContext()->setContextProperty("panelsModel", QVariant::fromValue(m_panels));
    ui->widOSD->rootContext()->setContextProperty("configWidget", QVariant::fromValue(this));
    ui->widOSD->setSource(QUrl(QStringLiteral("qrc:/configgadget/characterosd.qml")));

    auto root = dynamic_cast<QObject *>(ui->widOSD->rootObject());
    connect(root, SIGNAL(selectionChanged(int)), this, SLOT(onSelectionChanged(int)));
    connect(ui->btnAddPanel, &QPushButton::clicked, this, &CharacterOSD::onAddPanel);
}

CharacterOSD::~CharacterOSD()
{
    delete ui;
}

void CharacterOSD::refreshFromUavo()
{
    auto objMan = getObjectManager();
    auto obj = objMan->getObject(QStringLiteral("CharOnScreenDisplaySettings"));
    auto panels = obj->getField(QStringLiteral("PanelType"));
    auto xs = obj->getField(QStringLiteral("X"));
    auto ys = obj->getField(QStringLiteral("Y"));

    m_disabledPanels = 0;

    for (int i = 0; i < m_maxPanels; i++) {
        const auto type = panels->getValue(i).toString();
        int x = xs->getValue(i).toInt();
        int y = ys->getValue(i).toInt();
        auto panel = qobject_cast<CharacterPanel *>(m_panels.at(i));
        if (x != panel->x() || y != panel->y() || type != panel->type()) {
            panel->setX(x);
            panel->setY(y);
            panel->setType(type);
            if (m_sizes.contains(type)) {
                auto size = m_sizes.value(type);
                panel->setWidth(size.first);
                panel->setHeight(size.second);
            } else {
                qWarning() << "Missing size for" << type;
            }
        }
        if (type == QStringLiteral("Disabled"))
            m_disabledPanels++;
    }

    ui->btnAddPanel->setEnabled(m_disabledPanels > 0);
}

void CharacterOSD::onSelectionChanged(int index)
{
    if (index >= 0 && index < m_maxPanels) {
        for (int i = 0; i < m_maxPanels; i++)
            m_panelWidgets.at(i)->setVisible(i == index);
    }
}

void CharacterOSD::onPanelTypeChanged(const QString &type)
{
    // who changed?
    auto box = sender();
    int index = box->property("index").toInt();

    // tell the qml model
    auto panel = qobject_cast<CharacterPanel *>(m_panels.at(index));
    panel->setType(type);
    auto size = m_sizes.value(type);
    panel->setWidth(size.first);
    panel->setHeight(size.second);
}

void CharacterOSD::onXChanged(int x)
{
    // who changed?
    auto box = sender();
    int index = box->property("index").toInt();

    // tell the qml model
    qobject_cast<CharacterPanel *>(m_panels.at(index))->setX(x);
}

void CharacterOSD::onYChanged(int y)
{
    // who changed?
    auto box = sender();
    int index = box->property("index").toInt();

    // tell the qml model
    qobject_cast<CharacterPanel *>(m_panels.at(index))->setY(y);
}

void CharacterOSD::onAddPanel()
{
    for (int i = 0; i < m_maxPanels; i++) {
        const auto panel = qobject_cast<CharacterPanel *>(m_panels.at(i));
        if (panel->type() == QStringLiteral("Disabled")) {
            panel->setType(QStringLiteral("BatteryVolt"));
            const auto box = qobject_cast<QComboBox *>(m_panelWidgets.at(i)->findChild<QComboBox *>());
            box->setCurrentText(panel->type());
            Q_EMIT panelAdded(i);
            break;
        }
    }

    ui->btnAddPanel->setEnabled(--m_disabledPanels > 0);
}

void CharacterOSD::onDeletePanel()
{
    // who changed?
    auto button = sender();
    int index = button->property("index").toInt();

    // set the panel disabled in the widget, and the qml model
    const auto panel = qobject_cast<CharacterPanel *>(m_panels.at(index));
    panel->setType(QStringLiteral("Disabled"));
    const auto box = qobject_cast<QWidget *>(m_panelWidgets.at(index));
    box->setVisible(false);
    const auto combo = qobject_cast<QComboBox *>(box->findChild<QComboBox *>());
    combo->setCurrentText(panel->type());
    Q_EMIT panelDeleted(index);

    ui->btnAddPanel->setEnabled(++m_disabledPanels > 0);
}

/**
 * @}
 * @}
 */
