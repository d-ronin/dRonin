/**
 ******************************************************************************
 *
 * @file       qmlviewgadgetwidget.cpp
 * @author     Edouard Lafargue Copyright (C) 2010.
 * @author     Dmytro Poplavskiy Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup OPMapPlugin QML Viewer Plugin
 * @{
 * @brief The QML Viewer Gadget
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

#include "qmlviewgadgetwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "utils/svgimageprovider.h"

#include <QDebug>
#include <QSvgRenderer>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>

#include <QQmlEngine>
#include <QQmlContext>

QmlViewGadgetWidget::QmlViewGadgetWidget(QWindow *parent)
    : QQuickView(parent)
{
    setResizeMode(SizeRootObjectToView);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    QVector<QVector<UAVObject *>> objects = objManager->getObjectsVector();

    foreach (const QVector<UAVObject *> &objInst, objects)
        engine()->rootContext()->setContextProperty(objInst.at(0)->getName(), objInst.at(0));

    engine()->rootContext()->setContextProperty("qmlWidget", this);
}

QmlViewGadgetWidget::~QmlViewGadgetWidget()
{
}

void QmlViewGadgetWidget::setQmlFile(QString fn)
{
    m_fn = fn;

    engine()->removeImageProvider("svg");
    SvgImageProvider *svgProvider = new SvgImageProvider(fn);
    engine()->addImageProvider("svg", svgProvider);

    engine()->clearComponentCache();

    // it's necessary to allow qml side to query svg element position
    engine()->rootContext()->setContextProperty("svgRenderer", svgProvider);
    engine()->setBaseUrl(QUrl::fromLocalFile(fn));

    qDebug() << Q_FUNC_INFO << fn;
    setSource(QUrl::fromLocalFile(fn));

    foreach (const QQmlError &error, errors()) {
        qDebug() << error.description();
    }
}

void QmlViewGadgetWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // Reload the schene on the middle mouse button click.
    if (event->button() == Qt::MiddleButton) {
        setQmlFile(m_fn);
    }

    QQuickView::mouseReleaseEvent(event);
}
