/**
 ******************************************************************************
 * @file       usagestatsoptionpage.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2015, 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UsageStatsPlugin UsageStats Gadget Plugin
 * @{
 * @brief Usage stats collection plugin
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

#ifndef USAGESTATSOPTIONPAGE_H
#define USAGESTATSOPTIONPAGE_H

#include <QWidget>
#include "coreplugin/dialogs/ioptionspage.h"
#include <coreplugin/iconfigurableplugin.h>

class UsageStatsPlugin;
namespace Core {
class IUAVGadgetConfiguration;
}

namespace Ui {
class UsageStatsOptionPage;
}

using namespace Core;

class UsageStatsOptionPage : public IOptionsPage
{
    Q_OBJECT
public:
    UsageStatsOptionPage(QObject *parent = 0);
    virtual ~UsageStatsOptionPage();

    QString id() const { return QLatin1String("settings"); }
    QString trName() const { return tr("settings"); }
    QString category() const { return "Usage Statistics"; }
    QString trCategory() const { return "Usage Statistics"; }

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
signals:
    void settingsUpdated();
private slots:
private:
    Ui::UsageStatsOptionPage *m_page;
    UsageStatsPlugin *m_config;
};

#endif // USAGESTATSOPTIONPAGE_H
