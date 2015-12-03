/**
 ******************************************************************************
 * @file       autotuneshareform.h
 * @author     dRonin, http://dronin.org, Copyright (C) 2015
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Utility to present a form to the user where he can input his
 * aircraft details
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
 */

#ifndef AUTOTUNESHAREFORM_H
#define AUTOTUNESHAREFORM_H

#include <QDialog>
#include <QComboBox>

namespace Ui {
class AutotuneShareForm;
}

class AutotuneShareForm : public QDialog
{
    Q_OBJECT

public:
    explicit AutotuneShareForm(QWidget *parent = 0);
    ~AutotuneShareForm();
    void setVehicleTypeOptions(QStringList options);
    void setObservations(QString value);
    QString getObservations();
    void setVehicleType(QString type);
    QString getVehicleType();
    void disableVehicleType(bool disable);
    void setBoardType(QString type);
    QString getBoardType();
    void disableBoardType(bool disable);
    void setWeight(int weight);
    int getWeight();
    void setVehicleSize(int spacing);
    int getVehicleSize();
    void setBatteryCells(int cells);
    int getBatteryCells();
    void setMotors(QString motors);
    QString getMotors();
    void setESCs(QString escs);
    QString getESCs();
    void setProgress(int value);
    void disableProgress(bool disabled);
    void disableClipboard(bool disabled);
    void disableDatabase(bool disabled);

signals:
    void ClipboardRequest();
    void DatabaseRequest();

public slots:
    void onClipboardClicked();
    void onDatabaseClick();

private:
    Ui::AutotuneShareForm *ui;
    void replaceItems(QComboBox* cb, const QStringList &items);
};

#endif // AUTOTUNESHAREFORM_H
