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
    void downloadImage(QUrl url);
    void downloadImages(QStringList listOfURLs);

private slots:
    void on_evaluateTestsButton_clicked();
    void on_evaluateTestsRecursivelyButton_clicked();
    void on_createRecursiveScriptButton_clicked();
    void on_createRecursiveScriptsRecursivelyButton_clicked();
    void on_createTestButton_clicked();
    void on_deleteOldSnapshotsButton_clicked();
    void on_closeButton_clicked();

    void saveImage(int index);

private:
    Ui::AutoTesterClass ui;

    Test* test;
    std::vector<Downloader*> downloaders;

    QSignalMapper* signalMapper;

    int _numberOfImagesToDownload;
    int _numberOfImagesDownloaded;
    int _index;
};

#endif // hifi_AutoTester_h