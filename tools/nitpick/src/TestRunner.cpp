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
