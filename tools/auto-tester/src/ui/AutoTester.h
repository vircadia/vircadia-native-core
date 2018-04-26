//
//  AutoTester.h
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_AutoTester_h
#define hifi_AutoTester_h

#include <QtWidgets/QMainWindow>
#include <QSignalMapper>
#include "ui_AutoTester.h"

#include "../Downloader.h"
#include "../Test.h"

class AutoTester : public QMainWindow {
    Q_OBJECT

public:
    AutoTester(QWidget *parent = Q_NULLPTR);
    void downloadImage(const QUrl& url);
    void downloadImages(const QStringList& URLs, const QString& directoryName, const QStringList& filenames);

private slots:
    void on_evaluateTestsButton_clicked();
    void on_createRecursiveScriptButton_clicked();
    void on_createAllRecursiveScriptsButton_clicked();
	void on_createTestButton_clicked();
    void on_createMDFileButton_clicked();
    void on_createAllMDFilesButton_clicked();
    void on_createTestsOutlineButton_clicked();
    void on_closeButton_clicked();

    void saveImage(int index);

private:
    Ui::AutoTesterClass ui;
    Test* test;

    std::vector<Downloader*> downloaders;

    // local storage for parameters - folder to store downloaded files in, and a list of their names
    QString _directoryName;
    QStringList _filenames;

    // Used to enable passing a parameter to slots
    QSignalMapper* signalMapper;

    int _numberOfImagesToDownload;
    int _numberOfImagesDownloaded;
    int _index;
};

#endif // hifi_AutoTester_h