/**
 ******************************************************************************
 *
 * @file       qmlviewgadgetoptionspage.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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

#ifndef QMLVIEWGADGETOPTIONSPAGE_H
#define QMLVIEWGADGETOPTIONSPAGE_H

#include "QString"
#include "coreplugin/dialogs/ioptionspage.h"
#include <QDebug>
#include <QStringList>

namespace Core
{
class IUAVGadgetConfiguration;
}

class QmlViewGadgetConfiguration;

namespace Ui
{
class QmlViewGadgetOptionsPage;
}

using namespace Core;

class QmlViewGadgetOptionsPage : public IOptionsPage
{
    Q_OBJECT
public:
    explicit QmlViewGadgetOptionsPage(QmlViewGadgetConfiguration *config,
                                      QObject *parent = 0);

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

private:
    Ui::QmlViewGadgetOptionsPage *options_page;
    QmlViewGadgetConfiguration *m_config;

private slots:
};

#endif // QmlViewGADGETOPTIONSPAGE_H
