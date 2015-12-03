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

#include "autotuneshareform.h"
#include "ui_autotuneshareform.h"

AutotuneShareForm::AutotuneShareForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutotuneShareForm)
{
    ui->setupUi(this);
    connect(ui->btnClipboard, SIGNAL(clicked()), this, SLOT(onClipboardClicked()));
    connect(ui->btnDatabase, SIGNAL(clicked()), this, SLOT(onDatabaseClick()));
}

AutotuneShareForm::~AutotuneShareForm()
{
    delete ui;
}

void AutotuneShareForm::replaceItems(QComboBox* cb, const QStringList &items)
{
    QString selected = cb->currentText();
    cb->clear();
    cb->addItems(items);
    cb->setCurrentText(selected);
}

void AutotuneShareForm::setVehicleTypeOptions(QStringList options)
{
    ui->acType->setEditable(false);
    replaceItems(ui->acType, options);
}

void AutotuneShareForm::setObservations(QString value)
{
    ui->teObservations->setText(value);
}

QString AutotuneShareForm::getObservations()
{
    return ui->teObservations->toPlainText();
}

void AutotuneShareForm::setVehicleType(QString type)
{
    ui->acType->setCurrentText(type);
}

QString AutotuneShareForm::getVehicleType()
{
    return ui->acType->currentText();
}

void AutotuneShareForm::setBoardType(QString type)
{
    ui->acBoard->setText(type);
}

QString AutotuneShareForm::getBoardType()
{
    return ui->acBoard->text();
}

void AutotuneShareForm::setWeight(int weight)
{
    ui->acWeight->setText(QString::number(weight));
}

int AutotuneShareForm::getWeight()
{
    return ui->acWeight->text().toInt();
}

void AutotuneShareForm::setVehicleSize(int spacing)
{
    ui->acVehicleSize->setText(QString::number(spacing));
}

int AutotuneShareForm::getVehicleSize()
{
    return ui->acVehicleSize->text().toInt();
}

void AutotuneShareForm::setBatteryCells(int cells)
{
    ui->acBatteryCells->setCurrentText(QString::number(cells));
}

int AutotuneShareForm::getBatteryCells()
{
    return ui->acBatteryCells->currentText().toInt();
}

void AutotuneShareForm::setMotors(QString motors)
{
    ui->acMotors->setText(motors);
}

QString AutotuneShareForm::getMotors()
{
    return ui->acMotors->text();
}

void AutotuneShareForm::setESCs(QString escs)
{
    ui->acEscs->setText(escs);
}

QString AutotuneShareForm::getESCs()
{
    return ui->acEscs->text();
}

void AutotuneShareForm::disableVehicleType(bool disable)
{
    ui->acType->setDisabled(disable);
}

void AutotuneShareForm::disableBoardType(bool disable)
{
    ui->acBoard->setDisabled(disable);
}

void AutotuneShareForm::setProgress(int value)
{
    ui->progressBar->setValue(value);
}

void AutotuneShareForm::disableProgress(bool disabled)
{
    ui->progressBar->setDisabled(disabled);
}

void AutotuneShareForm::disableClipboard(bool disabled)
{
    ui->btnClipboard->setDisabled(disabled);
}

void AutotuneShareForm::disableDatabase(bool disabled)
{
    ui->btnDatabase->setDisabled(disabled);
}

void AutotuneShareForm::onClipboardClicked()
{
    emit ClipboardRequest();
}

void AutotuneShareForm::onDatabaseClick()
{
    emit DatabaseRequest();
}
