//
//  TestRunner.cpp
//
//  Created by Nissim Hadar on 23 Jan 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRunner.h"

#include <QFileDialog>

#include "Nitpick.h"
extern Nitpick* nitpick;

void TestRunner::setWorkingFolder(QLabel* workingFolderLabel) {
    // Everything will be written to this folder
    QString previousSelection = _workingFolder;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _workingFolder = QFileDialog::getExistingDirectory(nullptr, "Please select a working folder for temporary files", parent,
        QFileDialog::ShowDirsOnly);

    // If user canceled then restore previous selection and return
    if (_workingFolder == "") {
        _workingFolder = previousSelection;
        return;
    }

    workingFolderLabel->setText(QDir::toNativeSeparators(_workingFolder));
}

void TestRunner::downloadBuildXml(void* caller) {
    // Download the latest High Fidelity build XML.
    //      Note that this is not needed for PR builds (or whenever `Run Latest` is unchecked)
    //      It is still downloaded, to simplify the flow
    buildXMLDownloaded = false;

    QStringList urls;
    QStringList filenames;

    urls << DEV_BUILD_XML_URL;
    filenames << DEV_BUILD_XML_FILENAME;

    nitpick->downloadFiles(urls, _workingFolder, filenames, caller);
}

void Worker::setCommandLine(const QString& commandLine) {
    _commandLine = commandLine;
}

int Worker::runCommand() {
    int result = system(_commandLine.toStdString().c_str());
    emit commandComplete();
    return result;
}

