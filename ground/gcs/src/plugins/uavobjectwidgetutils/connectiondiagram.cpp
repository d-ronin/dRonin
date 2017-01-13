/**
 ******************************************************************************
 *
 * @file       connectiondiagram.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015-2016
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

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include "connectiondiagram.h"
#include "ui_connectiondiagram.h"
#include <extensionsystem/pluginmanager.h>
#include <uavobjects/uavobjectmanager.h>
#include <uavobjectutil/uavobjectutilmanager.h>
#include <coreplugin/iboardtype.h>
#include "systemsettings.h"

ConnectionDiagram::ConnectionDiagram(QWidget *parent) :
    QDialog(parent), ui(new Ui::ConnectionDiagram), m_background(0)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    utilMngr = pm->getObject<UAVObjectUtilManager>();
    Q_ASSERT(utilMngr);
    if (!utilMngr)
        return;
    uavoMngr = utilMngr->getObjectManager();
    Q_ASSERT(uavoMngr);
    if (!uavoMngr)
        return;

    ui->setupUi(this);
    setWindowTitle(tr("Connection Diagram"));
    setupGraphicsScene();

    connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveToFile()));
}

ConnectionDiagram::~ConnectionDiagram()
{
    delete ui;
}

void ConnectionDiagram::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if(m_background != NULL)
        ui->connectionDiagram->fitInView(m_background, Qt::KeepAspectRatio);
}

void ConnectionDiagram::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if(m_background != NULL)
        ui->connectionDiagram->fitInView(m_background, Qt::KeepAspectRatio);
}

void ConnectionDiagram::setupGraphicsScene()
{
    Core::IBoardType *board = utilMngr->getBoardType();
    if (!board)
        return;

    QString diagram = board->getConnectionDiagram();
    m_renderer = new QSvgRenderer();
    if (QFile::exists(diagram) && m_renderer->load(diagram) && m_renderer->isValid()) {

        m_scene = new QGraphicsScene(this);
        ui->connectionDiagram->setScene(m_scene);

        m_background = new QGraphicsSvgItem();
        m_background->setSharedRenderer(m_renderer);
        m_background->setElementId("background");
        m_background->setOpacity(0);
        m_background->setZValue(-1);
        m_scene->addItem(m_background);

        QStringList elementsToShow;
        elementsToShow << QString("controller-").append(board->shortName().toLower());

        QStringList uavosToShow;
        uavosToShow << "SystemSettings";
        uavosToShow << "ModuleSettings";
        uavosToShow << "OnScreenDisplaySettings";
        uavosToShow << board->getHwUAVO();
        for (const auto &uavoName : uavosToShow) {
            if (uavoName.length()) {
                UAVObject *obj = uavoMngr->getObject(uavoName);
                if (obj)
                    addUavoFieldElements(elementsToShow, obj);
            }
        }

        setupGraphicsSceneItems(elementsToShow);

        ui->connectionDiagram->setSceneRect(m_background->boundingRect());
        ui->connectionDiagram->fitInView(m_background, Qt::KeepAspectRatio);

        qDebug() << "Scene complete";
    }
}

void ConnectionDiagram::setupGraphicsSceneItems(QStringList elementsToShow)
{
    qreal z = 0;

    for (const QString &elementId : elementsToShow) {
        if (m_renderer->elementExists(elementId)) {
            QGraphicsSvgItem *element = new QGraphicsSvgItem();
            element->setSharedRenderer(m_renderer);
            element->setElementId(elementId);
            element->setZValue(z++);
            element->setOpacity(1.0);

            QMatrix matrix = m_renderer->matrixForElement(elementId);
            QRectF orig    = matrix.mapRect(m_renderer->boundsOnElement(elementId));
            element->setPos(orig.x(), orig.y());

            m_scene->addItem(element);
            qDebug() << "Adding " << elementId << " to scene at " << element->pos();
        } else {
            qDebug() << "Element with id: " << elementId << " not found.";
        }
    }
}

void ConnectionDiagram::saveToFile()
{
    QImage image(m_background->boundingRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(0);

    if (m_scene) {
        QPainter painter(&image);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        m_scene->render(&painter);
    }

    bool success = false;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Connection Diagram"), "", tr("Images (*.png *.xpm *.jpg)"));
    if (!fileName.isEmpty()) {
        QFileInfo fn(fileName);
        if (!fn.fileName().contains('.'))
            fileName += ".png";
        success = image.save(fileName);
    }

    if (!success)
        QMessageBox::warning(this, tr("Save Failed"), tr("Invalid filename provided, diagram was not saved."));
}

void ConnectionDiagram::addUavoFieldElements(QStringList &elements, UAVObject *obj, const QString &prefix)
{
    if (!obj) {
        Q_ASSERT(false);
        qWarning() << "Invalid object!";
        return;
    }

    for (const auto field : obj->getFields()) {
        if (!field) {
            Q_ASSERT(false);
            qWarning() << "Invalid object field!";
            continue;
        }
        if (field->getType() != UAVObjectField::ENUM)
            continue;

        quint32 nelements = field->getNumElements();
        if (nelements > 1) {
            for (quint32 i = 0; i < nelements; i++) {
                QString val = field->getValue(i).toString().replace(QRegExp(ENUM_SPECIAL_CHARS), "").toLower();
                elements << QString("%0%1-%2-%3-%4")
                            .arg(prefix)
                            .arg(obj->getName().toLower())
                            .arg(field->getName().toLower())
                            .arg(field->getElementName(i).replace(QRegExp(ENUM_SPECIAL_CHARS), "").toLower())
                            .arg(val);
            }
        } else {
            QString val = field->getValue().toString().replace(QRegExp(ENUM_SPECIAL_CHARS), "").toLower();
            elements << QString("%0%1-%2-%3").arg(prefix).arg(obj->getName().toLower()).arg(field->getName().toLower()).arg(val);
        }
    }
}

/**
 * @}
 * @}
 */
