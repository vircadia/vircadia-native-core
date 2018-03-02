//
//  AutoTester.cpp
//  zone/ambientLightInheritence
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AutoTester.h"

AutoTester::AutoTester(QWidget *parent) : QMainWindow(parent) {
    ui.setupUi(this);
    ui.checkBoxInteractiveMode->setChecked(true);
    ui.progressBar->setVisible(false);

    test = new Test();
}

void AutoTester::on_evaluateTestsButton_clicked() {
    ////QUrl imageUrl("http://ribafreixo.com/wp-content/uploads/2017/03/Order-Now-Button-300x113.png");
    ////downloader = new Downloader(imageUrl, this);
    ////connect(downloader, SIGNAL (downloaded()), this, SLOT (saveImage()));
    test->evaluateTests(ui.checkBoxInteractiveMode->isChecked(), ui.progressBar);
}

void AutoTester::on_evaluateTestsRecursivelyButton_clicked() {
    test->evaluateTestsRecursively(ui.checkBoxInteractiveMode->isChecked(), ui.progressBar);
}

void AutoTester::on_createRecursiveScriptButton_clicked() {
    test->createRecursiveScript();
}

void AutoTester::on_createRecursiveScriptsRecursivelyButton_clicked() {
    test->createRecursiveScriptsRecursively();
}

void AutoTester::on_createTestButton_clicked() {
    test->createTest();
}

void AutoTester::on_deleteOldSnapshotsButton_clicked() {
    test->deleteOldSnapshots();
}

void AutoTester::on_closeButton_clicked() {
    exit(0);
}

void AutoTester::downloadImage(QUrl url) {
    downloader = new Downloader(url, this);

    connect(downloader, SIGNAL (downloaded()), this, SLOT (saveImage()));
}

void AutoTester::saveImage() {
    QPixmap image;
    image.loadFromData(downloader->downloadedData());
    int er = image.width();
    int df = image.height();
    image.save("D:/Dicom/lll.png");
}
