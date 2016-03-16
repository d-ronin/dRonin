/**
 ******************************************************************************
 * @file       upgradeassistantdialog.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup  Uploader Uploader Plugin
 * @{
 * @brief The upgrade assistant dialog box
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

#ifndef UPGRADEASSISTANT_H
#define UPGRADEASSISTANT_H

#include <QDialog>
#include <QMap>
namespace Ui {
class UpgradeAssistant;
}

class UpgradeAssistantDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UpgradeAssistantDialog(QWidget *parent = 0);
    ~UpgradeAssistantDialog();
    
private:
    Ui::UpgradeAssistant *ui;
};

#endif // UPGRADEASSISTANT_H
