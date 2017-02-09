/**
 ******************************************************************************
 *
 * @file       authorsdialog.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup CorePlugin Core Plugin
 * @{
 * @brief The Core GCS plugin
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

#include "authorsdialog.h"

#include "coreconstants.h"
#include "icore.h"

#include <utils/qtcassert.h>
#include <utils/pathutils.h>

#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QSysInfo>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>

using namespace Core;
using namespace Core::Internal;
using namespace Core::Constants;

AuthorsDialog::AuthorsDialog(QWidget *parent)
    : QDialog(parent)
{
    // We need to set the window icon explicitly here since for some reason the
    // application icon isn't used when the size of the dialog is fixed (at least not on X11/GNOME)
    setWindowIcon(QIcon(":/core/gcs_nontrans_128"));

    setWindowTitle(tr("About GCS Authors"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    auto layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    QString version = QLatin1String(GCS_VERSION_LONG);
    version += QDate(2007, 25, 10).toString(Qt::SystemLocaleDate);

    QLabel *mainLabel = new QLabel(tr("%0 proudly brought to you by this fine team:").arg(GCS_PROJECT_BRANDING_PRETTY));

    QLabel *relCreditsLabel = new QLabel(tr("Current release:"));
    relCreditsLabel->setWordWrap(true);
    QTextBrowser *relCreditsArea = new QTextBrowser(this);
    relCreditsArea->setSource(QUrl::fromLocalFile(Utils::PathUtils::InsertDataPath("%%DATAPATH%%gcsrelauthors.html")));

    QLabel *creditsLabel = new QLabel(tr("All time (including previous contributions under OpenPilot and Tau Labs):"));
    creditsLabel->setWordWrap(true);
    QTextBrowser *creditsArea = new QTextBrowser(this);
    creditsArea->setSource(QUrl::fromLocalFile(Utils::PathUtils::InsertDataPath("%%DATAPATH%%gcsauthors.html")));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    QTC_ASSERT(closeButton, /**/);
    buttonBox->addButton(closeButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
    connect(buttonBox , SIGNAL(rejected()), this, SLOT(reject()));

    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(QPixmap(QLatin1String(":/core/gcs_logo_128")));

    layout->addWidget(logoLabel, 0, 0, 1, 2, Qt::AlignHCenter);
    layout->addWidget(mainLabel, 1, 0, 1, 2, Qt::AlignHCenter);
    layout->addWidget(relCreditsLabel, 2, 0, 1, 1, Qt::AlignBottom);
    layout->addWidget(relCreditsArea, 3, 0, 1, 1, Qt::AlignTop);
    layout->addWidget(creditsLabel, 2, 1, 1, 1, Qt::AlignBottom);
    layout->addWidget(creditsArea, 3, 1, 1, 1, Qt::AlignTop);
    layout->addWidget(buttonBox, 4, 0, 1, 2);
}
