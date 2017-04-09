/**
 ******************************************************************************
 *
 * @file       dialgadgetoptionspage.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup DialPlugin Dial Plugin
 * @{
 * @brief Plots flight information rotary style dials
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

#ifndef DIALGADGETOPTIONSPAGE_H
#define DIALGADGETOPTIONSPAGE_H

#include "coreplugin/dialogs/ioptionspage.h"
#include "QString"
#include <QStringList>
#include <QDebug>
#include <QFont>
#include <QFontDialog>
#include <QComboBox>

namespace Core {
class IUAVGadgetConfiguration;
}

class DialGadgetConfiguration;

namespace Ui {
class DialGadgetOptionsPage;
}

using namespace Core;

class DialGadgetOptionsPage : public IOptionsPage
{
    Q_OBJECT
public:
    explicit DialGadgetOptionsPage(DialGadgetConfiguration *config, QObject *parent = 0);

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

private:
    Ui::DialGadgetOptionsPage *options_page;
    DialGadgetConfiguration *m_config;
    QFont font;

    void uavoChanged(QComboBox *widget, const QString &uavo);

private slots:
    void on_fontPicker_clicked();
};

#endif // DIALGADGETOPTIONSPAGE_H
