/**
 ******************************************************************************
 * @file       waypointdialog.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup PathPlanner Map Plugin
 * @{
 * @brief Waypoint editor dialog
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

#include "waypointdialog.h"
#include <waypointdelegate.h>
#include <waypoint.h>
#include "ui_waypoint_dialog.h"

WaypointDialog::WaypointDialog(QWidget *parent, QAbstractItemModel *model,
                               QItemSelectionModel *selection)
    : QDialog(parent, Qt::Window)
    , ui(new Ui_waypoint_dialog)
    , model(model)
    , itemSelection(selection)
{
    ui->setupUi(this);
    connect(ui->cbMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &WaypointDialog::setupModeWidgets);

    // Connect up the buttons
    connect(ui->pushButtonOK, &QAbstractButton::clicked, this, &WaypointDialog::onOkButton_clicked);
    connect(ui->pushButtonCancel, &QAbstractButton::clicked, this,
            &WaypointDialog::onCancelButton_clicked);
    connect(ui->pushButtonPrevious, &QAbstractButton::clicked, this,
            &WaypointDialog::onPreviousButton_clicked);
    connect(ui->pushButtonNext, &QAbstractButton::clicked, this,
            &WaypointDialog::onNextButton_clicked);

    mapper = new QDataWidgetMapper(this);

    WaypointDelegate *delegate = new WaypointDelegate(this);
    delegate->loadComboBox(ui->cbMode);

    mapper->setItemDelegate(delegate);
    connect(mapper, &QDataWidgetMapper::currentIndexChanged, this,
            &WaypointDialog::currentIndexChanged);
    mapper->setModel(model);
    mapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
    mapper->addMapping(ui->doubleSpinBoxLatitude, FlightDataModel::LATPOSITION);
    mapper->addMapping(ui->doubleSpinBoxLongitude, FlightDataModel::LNGPOSITION);
    mapper->addMapping(ui->doubleSpinBoxAltitude, FlightDataModel::ALTITUDE);
    mapper->addMapping(ui->doubleSpinBoxNorth, FlightDataModel::NED_NORTH);
    mapper->addMapping(ui->doubleSpinBoxEast, FlightDataModel::NED_EAST);
    mapper->addMapping(ui->doubleSpinBoxDown, FlightDataModel::NED_DOWN);
    mapper->addMapping(ui->lineEditDescription, FlightDataModel::WPDESCRIPTION);
    mapper->addMapping(ui->doubleSpinBoxVelocity, FlightDataModel::VELOCITY);
    mapper->addMapping(ui->cbMode, FlightDataModel::MODE);
    mapper->addMapping(ui->dsb_modeParams, FlightDataModel::MODE_PARAMS);
    mapper->addMapping(ui->checkBoxLocked, FlightDataModel::LOCKED);

    // Make sure the model catches updates from the check box
    connect(ui->checkBoxLocked, &QCheckBox::stateChanged, mapper, &QDataWidgetMapper::submit);

    mapper->setCurrentIndex(selection->currentIndex().row());

    // Support locking the controls when locked
    enableEditWidgets();
    connect(model, &QAbstractItemModel::dataChanged, this, &WaypointDialog::enableEditWidgets);

    // This means whenever the model changes we show those changes.  Since the update is on
    // auto submit changes are still permitted.
    connect(model, &QAbstractItemModel::dataChanged, mapper, &QDataWidgetMapper::revert);

    connect(itemSelection, &QItemSelectionModel::currentRowChanged, this,
            &WaypointDialog::currentRowChanged);

    setModal(true);
}

/**
 * @brief WaypointDialog::currentIndexChanged Called when the data widget selector index
 * changes
 * @param index The newly selected index
 */
void WaypointDialog::currentIndexChanged(int index)
{
    ui->lbNumber->setText(QString::number(index + 1));
    QModelIndex idx = mapper->model()->index(index, 0);
    if (index == itemSelection->currentIndex().row())
        return;
    itemSelection->clear();
    itemSelection->setCurrentIndex(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

WaypointDialog::~WaypointDialog()
{
    delete ui;
}

/**
 * @brief WaypointDialog::setupModeWidgets Whenever the waypoint mode type changes
 * this updates the UI to display the available options (e.g. radius)
 */
void WaypointDialog::setupModeWidgets()
{
    int mode = ui->cbMode->itemData(ui->cbMode->currentIndex()).toInt();
    switch (mode) {
    case Waypoint::MODE_CIRCLERIGHT:
    case Waypoint::MODE_CIRCLELEFT:
        ui->modeParams->setVisible(true);
        ui->modeParams->setText(tr("Radius"));
        ui->dsb_modeParams->setVisible(true);
        break;
    default:
        ui->modeParams->setVisible(false);
        ui->dsb_modeParams->setVisible(false);
        break;
    }
}

/**
 * @brief WaypointDialog::editWaypoint Edit the requested waypoint, show dialog if it is not showing
 * @param[in] number The waypoint to edit
 */
void WaypointDialog::editWaypoint(int number)
{
    if (!isVisible())
        show();
    if (isMinimized())
        showNormal();
    if (!isActiveWindow())
        activateWindow();
    raise();
    setFocus(Qt::OtherFocusReason);
    mapper->setCurrentIndex(number);
}

//! Close the dialog button, accept the changes
void WaypointDialog::onOkButton_clicked()
{
    mapper->submit();
    close();
}

//! Close the dialog button, revert any changes
void WaypointDialog::onCancelButton_clicked()
{
    mapper->revert();
    close();
}

//! User requests the previous waypoint
void WaypointDialog::onPreviousButton_clicked()
{
    mapper->toPrevious();
}

//! User requests the next waypoint
void WaypointDialog::onNextButton_clicked()
{
    mapper->toNext();
}

/**
 * @brief WaypointDialog::currentRowChanged When the selector changes pass the
 * update to the data mapper
 * @param current The newly selected index
 */
void WaypointDialog::currentRowChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);

    mapper->setCurrentIndex(current.row());
}

/**
 * @brief WaypointDialog::enableEditWidgets Enable or disable the controls based
 * on the lock control
 * @param[in] value True if they should be enabled, false to disable
 */
void WaypointDialog::enableEditWidgets()
{
    int row = itemSelection->currentIndex().row();
    bool value = model->data(model->index(row, FlightDataModel::LOCKED)).toBool();
    QWidget *w;
    foreach (QWidget *obj, this->findChildren<QWidget *>()) {
        w = qobject_cast<QComboBox *>(obj);
        if (w)
            w->setEnabled(!value);
        w = qobject_cast<QLineEdit *>(obj);
        if (w)
            w->setEnabled(!value);
        w = qobject_cast<QDoubleSpinBox *>(obj);
        if (w)
            w->setEnabled(!value);
        w = qobject_cast<QSpinBox *>(obj);
        if (w)
            w->setEnabled(!value);
    }
}
