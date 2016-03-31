/**
 ******************************************************************************
 * @file       upgradeassistantdialog.cpp
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

#include <QDebug>
#include <QFont>
#include <QPushButton>

#include "upgradeassistantdialog.h"
#include "ui_upgradeassistant.h"

UpgradeAssistantDialog::UpgradeAssistantDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpgradeAssistant)
{
    ui->setupUi(this);

    setModal(true);

    stepLabels[STEP_ENTERLOADER] = ui->lblEnterLoader;
    stepLabels[STEP_CHECKCLOUD] = ui->lblCheckCloud;
    stepLabels[STEP_UPGRADEBOOTLOADER] = ui->lblUpgradeBootloader;
    stepLabels[STEP_PROGRAMUPGRADER] = ui->lblProgramUpgrader;
    stepLabels[STEP_ENTERUPGRADER] = ui->lblEnterUpgrader;
    stepLabels[STEP_DOWNLOADSETTINGS] = ui->lblDownloadSettings;
    stepLabels[STEP_TRANSLATESETTINGS] = ui->lblTranslateSettings;
    stepLabels[STEP_ERASESETTINGS] = ui->lblEraseSettings;
    stepLabels[STEP_REENTERLOADER] = ui->lblReenterLoader;
    stepLabels[STEP_FLASHFIRMWARE] = ui->lblFlashFirmware;
    stepLabels[STEP_BOOT] = ui->lblBoot;
    stepLabels[STEP_IMPORT] = ui->lblImport;

    for (int i=STEP_FIRST; i<STEP_NUM; i++) {
        originalText[i] = new QString(stepLabels[i]->text());
    }

    connect(ui->buttonBox, SIGNAL(rejected()), this,
            SLOT(reject()));
}

UpgradeAssistantDialog::~UpgradeAssistantDialog()
{
    delete ui;
}

void UpgradeAssistantDialog::setOperatingMode(bool upgradingBootloader,
        bool usingUpgrader, bool blankFC)
{
    ui->lblUpgradeBootloader->setEnabled(upgradingBootloader);

    ui->lblProgramUpgrader->setEnabled(usingUpgrader);
    ui->lblEnterUpgrader->setEnabled(usingUpgrader);
    ui->lblReenterLoader->setEnabled(usingUpgrader);

    ui->lblDownloadSettings->setEnabled(!blankFC);
    ui->lblTranslateSettings->setEnabled(!blankFC);
    ui->lblImport->setEnabled(!blankFC);

    onStepChanged(curStep);
}

int UpgradeAssistantDialog::PromptUser(
        QString promptText, QString detailText, QStringList buttonText) {
    int clickIndex=-1;

    QList <QPushButton *> buttons;

    QEventLoop loop;

    int i = 0;

    foreach (const QString &buttLabel, buttonText) {
        QPushButton *newButt = new QPushButton(buttLabel);
        buttons.append(newButt);
        ui->buttonBox->addButton(newButt, QDialogButtonBox::ActionRole);

        connect(newButt, &QPushButton::clicked, &loop, [&, i] (bool checked) {
                Q_UNUSED(checked);
                clickIndex = i;
                loop.exit();
            } );

        i++;
    }

    connect(this, &UpgradeAssistantDialog::finished, &loop, &QEventLoop::quit);

    ui->lblStatus->setText(promptText);
    ui->lblDetailStatus->setText(detailText);

    loop.exec();

    foreach (QPushButton *button, buttons) {
        ui->buttonBox->removeButton(button);

        delete button;
    }

    ui->lblStatus->setText(tr("Auto-upgrade running..."));
    ui->lblDetailStatus->setText(tr("Please wait while the upgrader performs automated actions on the flight board."));

    return clickIndex;
}

void UpgradeAssistantDialog::onStepChanged(UpgradeAssistantStep step)
{
    qDebug() << QString("UpgradeAssistant: in step %1").arg(step);

    curStep = step;

    int stepVal = step;

    for (int i=STEP_FIRST; i<STEP_NUM; i++) {
        if (i < stepVal) {
            // Ensure marked done
            if (stepLabels[i]->isEnabled()) {
                stepLabels[i]->setText(*originalText[i] + tr(" done!"));
            } else {
                stepLabels[i]->setText(*originalText[i] + tr(" (skipped)"));
            }
        } else if (i == stepVal) {
            // Ensure not marked done..
            stepLabels[i]->setText(QString("<b>%1</b>").arg(*originalText[i]));
        } else /* i > stepVal */ {
            // Ensure not marked done..
            stepLabels[i]->setText(*originalText[i]);
        }
    }

    if (step == STEP_DONE) {
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);

        ui->lblStatus->setText(tr("Auto-upgrade complete!"));
        ui->lblDetailStatus->setText(tr("It may be necessary to reset your flight board (by removing and attaching power and USB) to apply settings."));
    } else {
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Abort);

        ui->lblStatus->setText(tr("Auto-upgrade running..."));
        ui->lblDetailStatus->setText(tr("Please wait while the upgrader performs automated actions on the flight board."));
    }

    if (isVisible()) {
        raise();
    }
}

void UpgradeAssistantDialog::closeEvent(QCloseEvent* event) {
    (void) event;

    reject();
}
