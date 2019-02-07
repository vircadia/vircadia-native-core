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
    QPushButton* installAPKPushbutton,
    QPushButton* runInterfacePushbutton,
    QCheckBox* runLatest,
    QLineEdit* url,
    QLabel* statusLabel,

    QObject* parent
) : QObject(parent) 
{
    _workingFolderLabel = workingFolderLabel;
    _connectDeviceButton = connectDeviceButton;
    _pullFolderButton = pullFolderButton;
    _detectedDeviceLabel = detectedDeviceLabel;
    _folderLineEdit = folderLineEdit;
    _downloadAPKPushbutton = downloadAPKPushbutton;
    _installAPKPushbutton = installAPKPushbutton;
    _runInterfacePushbutton = runInterfacePushbutton;
    _runLatest = runLatest;
    _url = url;
    _statusLabel = statusLabel;

    folderLineEdit->setText("/sdcard/DCIM/TEST");

    modelNames["SM_G955U1"] = "Samsung S8+ unlocked";

    // Find ADB (Android Debugging Bridge)
#ifdef Q_OS_WIN
    if (QProcessEnvironment::systemEnvironment().contains("ADB_PATH")) {
        QString adbExePath = QProcessEnvironment::systemEnvironment().value("ADB_PATH") + "/platform-tools";
        if (!QFile::exists(adbExePath + "/" + _adbExe)) {
            QMessageBox::critical(0, _adbExe, QString("ADB executable not found in ") + adbExePath);
            exit(-1);
        }

        _adbCommand = adbExePath + "/" + _adbExe;
    } else {
        QMessageBox::critical(0, "ADB_PATH not defined",
            "Please set ADB_PATH to directory containing the `adb` executable");
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

TestRunnerMobile::~TestRunnerMobile() {
}

void TestRunnerMobile::setWorkingFolderAndEnableControls() {
    setWorkingFolder(_workingFolderLabel);

    _connectDeviceButton->setEnabled(true);
}

void TestRunnerMobile::connectDevice() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    QString devicesFullFilename{ _workingFolder + "/devices.txt" };
    QString command = _adbCommand + " devices -l > " + devicesFullFilename;
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
            // Line looks like this: 988a1b47335239434b     device product:dream2qlteue model:SM_G955U1 device:dream2qlteue transport_id:2
            QStringList tokens = line2.split(QRegExp("[\r\n\t ]+"));
            QString deviceID = tokens[0];
            
            QString modelID = tokens[3].split(':')[1];
            QString modelName = "UKNOWN";
            if (modelNames.count(modelID) == 1) {
                modelName = modelNames[modelID];
            }

            _detectedDeviceLabel->setText(modelName + " [" + deviceID + "]");
            _pullFolderButton->setEnabled(true);
            _folderLineEdit->setEnabled(true);
            _downloadAPKPushbutton->setEnabled(true);
        }
    }
#endif
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


            // Replace the `exe` extension with `apk`
            _installerFilename = _installerFilename.replace(_installerFilename.length() - 3, 3, "apk");
            _buildInformation.url = _buildInformation.url.replace(_buildInformation.url.length() - 3, 3, "apk");

            urls << _buildInformation.url;
            filenames << _installerFilename;
        } else {
            QString urlText = _url->text();
            urls << urlText;
            _installerFilename = getInstallerNameFromURL(urlText);
            filenames << _installerFilename;
        }

       _statusLabel->setText("Downloading installer");

        nitpick->downloadFiles(urls, _workingFolder, filenames, (void*)this);
    } else {
        _statusLabel->setText("Installer download complete");
        _installAPKPushbutton->setEnabled(true);
    }
}

void TestRunnerMobile::installAPK() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    _statusLabel->setText("Installing");
    QString command = _adbCommand + " install -r -d " + _workingFolder + "/" + _installerFilename + " >" + _workingFolder  + "/installOutput.txt";
    system(command.toStdString().c_str());
    _statusLabel->setText("Installation complete");
    _runInterfacePushbutton->setEnabled(true);
#endif
}

void TestRunnerMobile::runInterface() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    _statusLabel->setText("Starting Interface");
    QString command = _adbCommand + " shell monkey -p io.highfidelity.hifiinterface -v 1";
    system(command.toStdString().c_str());
    _statusLabel->setText("Interface started");
#endif
}

void TestRunnerMobile::pullFolder() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    _statusLabel->setText("Pulling folder");
    QString command = _adbCommand + " pull " + _folderLineEdit->text() + " " + _workingFolder + _installerFilename;
    system(command.toStdString().c_str());
    _statusLabel->setText("Pull complete");
#endif
}
