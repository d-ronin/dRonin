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

#include "pfdqmlgadgetoptionspage.h"
#include "pfdqmlgadgetconfiguration.h"
#include "ui_pfdqmlgadgetoptionspage.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjects/uavobjectmanager.h"
#include "uavobjects/uavdataobject.h"

#include <QtAlgorithms>
#include <QStringList>

PfdQmlGadgetOptionsPage::PfdQmlGadgetOptionsPage(PfdQmlGadgetConfiguration *config, QObject *parent)
    : IOptionsPage(parent)
    , m_config(config)
{
}

// creates options page widget (uses the UI file)
QWidget *PfdQmlGadgetOptionsPage::createPage(QWidget *parent)
{
    options_page = new Ui::PfdQmlGadgetOptionsPage();
    // main widget
    QWidget *optionsPageWidget = new QWidget(parent);
    // main layout
    options_page->setupUi(optionsPageWidget);

    // Restore the contents from the settings:
    options_page->qmlSourceFile->setExpectedKind(Utils::PathChooser::File);
    options_page->qmlSourceFile->setPromptDialogFilter(tr("QML file (*.qml)"));
    options_page->qmlSourceFile->setPromptDialogTitle(tr("Choose QML file"));
    options_page->qmlSourceFile->setPath(m_config->qmlFile());

    return optionsPageWidget;
}

/**
 * Called when the user presses apply or OK.
 *
 * Saves the current values
 *
 */
void PfdQmlGadgetOptionsPage::apply()
{
    m_config->setQmlFile(options_page->qmlSourceFile->path());
}

void PfdQmlGadgetOptionsPage::finish()
{
}
