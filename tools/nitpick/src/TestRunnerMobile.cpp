//
//  TestRunnerMobile.cpp
//
//  Created by Nissim Hadar on 22 Jan 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestRunnerMobile.h"

#include <QThread>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include "Nitpick.h"
extern Nitpick* nitpick;

TestRunnerMobile::TestRunnerMobile(
    QLabel* workingFolderLabel,
    QPushButton *connectDeviceButton,
    QPushButton *pullFolderButton,
    QLabel* detectedDeviceLabel,
    QLineEdit *folderLineEdit,
    QPushButton* downloadAPKPushbutton,
    QCheckBox* runLatest,
    QLineEdit* url,

    QObject* parent
) : QObject(parent) 
{
    _workingFolderLabel = workingFolderLabel;
    _connectDeviceButton = connectDeviceButton;
    _pullFolderButton = pullFolderButton;
    _detectedDeviceLabel = detectedDeviceLabel;
    _folderLineEdit = folderLineEdit;
    _downloadAPKPushbutton = downloadAPKPushbutton;
    _runLatest = runLatest;
    _url = url;

    folderLineEdit->setText("/sdcard/DCIM/TEST");
}

TestRunnerMobile::~TestRunnerMobile() {
}

void TestRunnerMobile::setWorkingFolderAndEnableControls() {
    setWorkingFolder(_workingFolderLabel);

    _connectDeviceButton->setEnabled(true);
    _downloadAPKPushbutton->setEnabled(true);

    // Find ADB (Android Debugging Bridge) before continuing
#ifdef Q_OS_WIN
    if (QProcessEnvironment::systemEnvironment().contains("ADB_PATH")) {
        QString adbExePath = QProcessEnvironment::systemEnvironment().value("ADB_PATH") + "/platform-tools";
        if (!QFile::exists(adbExePath + "/" + _adbExe)) {
            QMessageBox::critical(0, _adbExe, QString("Python executable not found in ") + adbExePath);
            exit(-1);
        }

        _adbCommand = adbExePath + "/" + _adbExe;
    } else {
        QMessageBox::critical(0, "PYTHON_PATH not defined",
            "Please set PYTHON_PATH to directory containing the Python executable");
        exit(-1);
    }
#elif defined Q_OS_MAC
    _adbCommand = "/usr/local/bin/adb";
    if (!QFile::exists(_adbCommand)) {
        QMessageBox::critical(0, "adb not found",
            "python3 not found at " + _adbCommand);
        exit(-1);
    }
#endif
}

void TestRunnerMobile::connectDevice() {
    QString devicesFullFilename{ _workingFolder + "/devices.txt" };
    QString command = _adbCommand + " devices > " + devicesFullFilename;
    system(command.toStdString().c_str());

    if (!QFile::exists(devicesFullFilename)) {
        QMessageBox::critical(0, "Internal error", "devicesFullFilename not found");
        exit (-1);
    }

    // Device should be in second line
    QFile devicesFile(devicesFullFilename);
    devicesFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QString line1 = devicesFile.readLine();
    QString line2 = devicesFile.readLine();

    const QString DEVICE{ "device" };
    if (line2.contains(DEVICE)) {
        // Make sure only 1 device
        QString line3 = devicesFile.readLine();
        if (line3.contains(DEVICE)) {
            QMessageBox::critical(0, "Too many devices detected", "Tests will run only if a single device is attached");

        } else {
            _detectedDeviceLabel->setText(line2.remove(DEVICE));
            _pullFolderButton->setEnabled(true);
            _folderLineEdit->setEnabled(true);
        }
    }
}

void TestRunnerMobile::downloadAPK() {
    downloadBuildXml((void*)this);
}


void TestRunnerMobile::downloadComplete() {
    if (!buildXMLDownloaded) {
        // Download of Build XML has completed
        buildXMLDownloaded = true;

        // Download the High Fidelity installer
        QStringList urls;
        QStringList filenames;
        if (_runLatest->isChecked()) {
            parseBuildInformation();

            _installerFilename = INSTALLER_FILENAME_LATEST;

            urls << _buildInformation.url;
            filenames << _installerFilename;
        } else {
            QString urlText = _url->text();
            urls << urlText;
            _installerFilename = getInstallerNameFromURL(urlText);
            filenames << _installerFilename;
        }

       _statusLabel->setText("Downloading installer");

       //// nitpick->downloadFiles(urls, _workingFolder, filenames, (void*)this);

        // `downloadComplete` will run again after download has completed

    } else {
        // Download of Installer has completed
////        appendLog(QString("Tests started at ") + QString::number(_testStartDateTime.time().hour()) + ":" +
////            QString("%1").arg(_testStartDateTime.time().minute(), 2, 10, QChar('0')) + ", on " +
////            _testStartDateTime.date().toString("ddd, MMM d, yyyy"));

        _statusLabel->setText("Installing");

        // Kill any existing processes that would interfere with installation
////        killProcesses();

////        runInstaller();
    }
}

void TestRunnerMobile::pullFolder() {
    QString command = _adbCommand + " pull " + _folderLineEdit->text() + " " + _workingFolder + " >" + _workingFolder + "/pullOutput.txt";
    system(command.toStdString().c_str());
}
