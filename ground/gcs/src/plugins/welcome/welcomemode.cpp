/**
 ******************************************************************************
 *
 * @file       welcomemode.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup WelcomePlugin Welcome Plugin
 * @{
 * @brief The GCS Welcome plugin
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

#include "welcomemode.h"
#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/modemanager.h>

#include <utils/styledbar.h>
#include <utils/pathutils.h>

#include <QDesktopServices>

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QDebug>

#include <QtQuick>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlContext>

#include <cstdlib>

using namespace ExtensionSystem;
using namespace Utils;

namespace Welcome {

struct WelcomeModePrivate
{
    WelcomeModePrivate();

    QQuickView *quickView;
};

WelcomeModePrivate::WelcomeModePrivate()
{
}

// ---  WelcomeMode
// instUUID is used to hash-- to determine if we're in the fraction of users
// that should get a staggered announcement.  It may be bogus on first run,
// but no big deal.
WelcomeMode::WelcomeMode(QString instUUID)
    : m_d(new WelcomeModePrivate)
    , m_priority(Core::Constants::P_MODE_WELCOME)
{
    m_d->quickView = new QQuickView;
    m_d->quickView->setResizeMode(QQuickView::SizeRootObjectToView);

    m_d->quickView->engine()->rootContext()->setContextProperty("welcomePlugin", this);
    m_d->quickView->engine()->rootContext()->setContextProperty("instHash",
                                                                QVariant(qHash(instUUID)));
    m_d->quickView->engine()->rootContext()->setContextProperty(
        "gitHash", QVariant(Core::Constants::GCS_REVISION_SHORT_STR));

    QString fn = Utils::PathUtils().InsertDataPath(QString("%%DATAPATH%%/welcome/main.qml"));
    m_d->quickView->setSource(QUrl::fromLocalFile(fn));
    m_container = NULL;

    connect(Core::ModeManager::instance(), &Core::ModeManager::modesChanged, this,
            &WelcomeMode::modesChanged);
    modesChanged();
}

WelcomeMode::~WelcomeMode()
{
    delete m_d->quickView;
    delete m_d;
}

QString WelcomeMode::name() const
{
    return tr("Welcome");
}

QIcon WelcomeMode::icon() const
{
    return QIcon(QLatin1String(":/core/gcs_logo_64"));
}

int WelcomeMode::priority() const
{
    return m_priority;
}

QWidget *WelcomeMode::widget()
{
    if (!m_container) {
        m_container = QWidget::createWindowContainer(m_d->quickView);
        m_container->setMinimumSize(64, 64);
        m_container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    }
    return m_container;
}

const char *WelcomeMode::uniqueModeName() const
{
    return Core::Constants::MODE_WELCOME;
}

QList<int> WelcomeMode::context() const
{
    static QList<int> contexts = QList<int>()
        << Core::UniqueIDManager::instance()->uniqueIdentifier(Core::Constants::C_WELCOME_MODE);
    return contexts;
}

void WelcomeMode::openPage(const QString &page)
{
    Core::ModeManager::instance()->activateModeByWorkspaceName(page);
}

void WelcomeMode::triggerAction(const QString &actionId)
{
    Core::ModeManager::instance()->triggerAction(actionId);
}

void WelcomeMode::modesChanged()
{
    QVector<IMode *> modes = Core::ModeManager::instance()->modes();
    QStringList modeNames;

    foreach (IMode *mode, modes)
        modeNames.append(mode->name());

    auto buttons = m_d->quickView->rootObject()->findChild<QObject *>("modeButtons");
    if (buttons)
        buttons->setProperty("modeNames", modeNames);
    else
        qWarning() << "[WelcomeMode::modesChanged] Can't find mode buttons";
}

} // namespace Welcome
