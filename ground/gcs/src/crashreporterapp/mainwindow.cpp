/**
 ******************************************************************************
 * @file       mainwindow.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @addtogroup crashreporterapp
 * @{
 * @addtogroup MainWindow
 * @{
 * @brief Sends a crash report to the forum
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QtNetwork/QtNetwork>
#include <QSysInfo>

const QString MainWindow::postUrl = QString("http://dronin-autotown.appspot.com/storeCrash");
const QString MainWindow::gitCommit = QString(GIT_COMMIT);
const QString MainWindow::gitBranch = QString(GIT_BRANCH);
const bool MainWindow::gitDirty = GIT_DIRTY;
const QString MainWindow::gitTag = QString(GIT_TAG);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    connect(ui->textBrowser, SIGNAL(anchorClicked(QUrl)), this, SLOT(onShowReport()));
    connect(ui->pb_Save, SIGNAL(clicked(bool)), this, SLOT(onSaveReport()));
    connect(ui->pb_Cancel, SIGNAL(clicked(bool)), this, SLOT(close()));
    connect(ui->pb_CancelSend, SIGNAL(clicked(bool)), this, SLOT(onCancelSend()));
    connect(ui->pb_OK, SIGNAL(clicked(bool)), this, SLOT(onOkSend()));
    connect(ui->pb_SendReport, SIGNAL(clicked(bool)), this, SLOT(onSendReport()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setDumpFile(QString filename)
{
    dumpFile = filename;
}

void MainWindow::onShowReport()
{
    QString txtFile(QDir::temp().absolutePath() + QDir::separator() + "dump.txt");
    QFile::copy(dumpFile, txtFile);
    QDesktopServices::openUrl(QUrl("file:///" + txtFile, QUrl::TolerantMode));
}

void MainWindow::onSaveReport()
{
    QString file = QFileDialog::getSaveFileName(this, "Choose the filename", QDir::homePath(), "Dump File (*.dmp)");
    if(file.isEmpty())
        return;
    QString trash;
    QFile::copy(dumpFile, file);
}

void MainWindow::onOkSend()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::onSendReport()
{
    ui->stackedWidget->setCurrentIndex(2);

    QFile file(dumpFile);
    file.open(QIODevice::ReadOnly);
    QByteArray dumpData = file.readAll();

    QJsonObject json;
    json["dataVersion"] = 1;
    json["dump"] = QString(dumpData.toBase64());
    json["comment"] = ui->te_Description->toPlainText();
    json["directory"] = QFileInfo(file).absolutePath().split("/").last();
    // version info
    json["gitCommit"] = gitCommit;
    json["gitBranch"] = gitBranch;
    json["gitDirty"] = gitDirty;
    json["gitTag"] = gitTag;

    json["currentOS"] = QSysInfo::prettyProductName();
    json["currentArch"] = QSysInfo::currentCpuArchitecture();
    json["buildInfo"] = QSysInfo::buildAbi();

    QUrl url(postUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onSendFinished(QNetworkReply*)));

    QNetworkReply *reply = manager->post(request, QJsonDocument(json).toJson());

    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
}

void MainWindow::onSendFinished(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        qWarning() << "[MainWindow::onSendFinished]HTTP Error: " << reply->errorString();
        QMessageBox::warning(this, tr("Could not send the crash report"), tr("Ooops, something went wrong"));
        ui->stackedWidget->setCurrentIndex(0);
    }
    else {
        QMessageBox::information(this, tr("Crash report sent"), tr("Thank you!"));
        ui->stackedWidget->setCurrentIndex(0);
        this->close();
    }
    reply->deleteLater();
}

void MainWindow::onCancelSend()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::onUploadProgress(qint64 progress, qint64 total)
{
    if(total == 0)
        return;
    ui->uploadLabel->setText("Uploading");
    ui->progressLabel->setText(QString("Uploaded %0 of %1 bytes").arg(progress).arg(total));
    int p = (progress * 100) / total;
    ui->progressBar->setValue(p);
}
