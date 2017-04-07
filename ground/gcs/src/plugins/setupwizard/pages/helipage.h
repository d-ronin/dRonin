/**
 ******************************************************************************
 *
 * @file       helipage.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef HELIPAGE_H
#define HELIPAGE_H

#include "abstractwizardpage.h"

namespace Ui
{
class HeliPage;
}

class HeliPage : public AbstractWizardPage
{
    Q_OBJECT

public:
    explicit HeliPage(SetupWizard *wizard, QWidget *parent = 0);
    ~HeliPage();

private:
    Ui::HeliPage *ui;
};

#endif // HELIPAGE_H
