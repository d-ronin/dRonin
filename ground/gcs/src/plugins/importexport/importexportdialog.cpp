#include "importexportdialog.h"
#include "ui_importexportdialog.h"

ImportExportDialog::ImportExportDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportExportDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Import Export Settings"));

    connect(ui->widget, &ImportExportGadgetWidget::done, this, &QWidget::close);
}

ImportExportDialog::~ImportExportDialog()
{
    delete ui;
}

void ImportExportDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
