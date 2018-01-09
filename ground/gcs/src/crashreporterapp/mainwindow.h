/**
 ******************************************************************************
 * @file       mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QtNetwork/QNetworkReply>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setDumpFile(QString filename);

private:
    Ui::MainWindow *ui;
    QString dumpFile;
    static const QString postUrl;
    static const QString gitCommit;
    static const QString gitBranch;
    static const bool gitDirty;
    static const QString gitTag;

private slots:
    void onShowReport();
    void onSaveReport();
    void onOkSend();
    void onSendReport();
    void onCancelSend();
    void onUploadProgress(qint64, qint64);
    void onSendFinished(QNetworkReply* reply);
};

#endif // MAINWINDOW_H
