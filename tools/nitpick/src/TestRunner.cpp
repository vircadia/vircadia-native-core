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

void TestRunner::parseBuildInformation() {
    try {
        QDomDocument domDocument;
        QString filename{ _workingFolder + "/" + DEV_BUILD_XML_FILENAME };
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly) || !domDocument.setContent(&file)) {
            throw QString("Could not open " + filename);
        }

        QString platformOfInterest;
#ifdef Q_OS_WIN
        platformOfInterest = "windows";
#elif defined(Q_OS_MAC)
        platformOfInterest = "mac";
#endif

        QDomElement element = domDocument.documentElement();

        // Verify first element is "projects"
        if (element.tagName() != "projects") {
            throw("File seems to be in wrong format");
        }

        element = element.firstChild().toElement();
        if (element.tagName() != "project") {
            throw("File seems to be in wrong format");
        }

        if (element.attribute("name") != "interface") {
            throw("File is not from 'interface' build");
        }

        // Now loop over the platforms, looking for ours
        bool platformFound{ false };
        element = element.firstChild().toElement();
        while (!element.isNull()) {
            if (element.attribute("name") == platformOfInterest) {
                platformFound = true;
                break;
            }
            element = element.nextSibling().toElement();
        }

        if (!platformFound) {
            throw("File seems to be in wrong format - platform " + platformOfInterest + " not found");
        }

        element = element.firstChild().toElement();
        if (element.tagName() != "build") {
            throw("File seems to be in wrong format");
        }

        // Next element should be the version
        element = element.firstChild().toElement();
        if (element.tagName() != "version") {
            throw("File seems to be in wrong format");
        }

        // Add the build number to the end of the filename
        _buildInformation.build = element.text();

        // First sibling should be stable_version
        element = element.nextSibling().toElement();
        if (element.tagName() != "stable_version") {
            throw("File seems to be in wrong format");
        }

        // Next sibling should be url
        element = element.nextSibling().toElement();
        if (element.tagName() != "url") {
            throw("File seems to be in wrong format");
        }
        _buildInformation.url = element.text();

    }
    catch (QString errorMessage) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), errorMessage);
        exit(-1);
    }
    catch (...) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "unknown error");
        exit(-1);
    }
}

QString TestRunner::getInstallerNameFromURL(const QString& url) {
    // An example URL: https://deployment.highfidelity.com/jobs/pr-build/label%3Dwindows/13023/HighFidelity-Beta-Interface-PR14006-be76c43.exe
    // On Mac, replace `exe` with `dmg`
    try {
        QStringList urlParts = url.split("/");
        return urlParts[urlParts.size() - 1];
    }
    catch (QString errorMessage) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), errorMessage);
        exit(-1);
    }
    catch (...) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "unknown error");
        exit(-1);
    }
}

void Worker::setCommandLine(const QString& commandLine) {
    _commandLine = commandLine;
}

int Worker::runCommand() {
    int result = system(_commandLine.toStdString().c_str());
    emit commandComplete();
    return result;
}
