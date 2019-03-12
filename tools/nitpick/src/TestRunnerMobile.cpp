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
    QCheckBox* usePreviousInstallationOnMobileCheckBox,
    QCheckBox* runLatest,
    QLineEdit* url,
    QCheckBox* runFullSuite,
    QLineEdit* scriptURL,
    QLabel* statusLabel,

    QObject* parent
) : QObject(parent), TestRunner(workingFolderLabel, statusLabel, usePreviousInstallationOnMobileCheckBox, runLatest, url, runFullSuite, scriptURL)
{
    _connectDeviceButton = connectDeviceButton;
    _pullFolderButton = pullFolderButton;
    _detectedDeviceLabel = detectedDeviceLabel;
    _folderLineEdit = folderLineEdit;
    _downloadAPKPushbutton = downloadAPKPushbutton;
    _installAPKPushbutton = installAPKPushbutton;
    _runInterfacePushbutton = runInterfacePushbutton;

    folderLineEdit->setText("/sdcard/snapshots");

    modelNames["SM_G955U1"] = "Samsung S8+ unlocked";
    modelNames["SM_N960U1"] = "Samsung Note 9 unlocked";
    modelNames["SM_T380"] = "Samsung Tab A";
    modelNames["Quest"] = "Quest";

    _adbInterface = NULL;
}

TestRunnerMobile::~TestRunnerMobile() {
}

void TestRunnerMobile::setWorkingFolderAndEnableControls() {
    setWorkingFolder(_workingFolderLabel);

    _connectDeviceButton->setEnabled(true);
}

void TestRunnerMobile::connectDevice() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    if (!_adbInterface) {
        _adbInterface = new AdbInterface();
    }
    
    QString devicesFullFilename{ _workingFolder + "/devices.txt" };
    QString command = _adbInterface->getAdbCommand() + " devices -l > " + devicesFullFilename;
    appendLog(command);
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
    if (line2.contains("unauthorized")) {
        QMessageBox::critical(0, "Unauthorized device detected", "Please allow USB debugging on device");
    } else if (line2.contains(DEVICE)) {
            // Make sure only 1 device
        QString line3 = devicesFile.readLine();
        if (line3.contains(DEVICE)) {
            QMessageBox::critical(0, "Too many devices detected", "Tests will run only if a single device is attached");
        } else {
            // Line looks like this: 988a1b47335239434b     device product:dream2qlteue model:SM_G955U1 device:dream2qlteue transport_id:2
            QStringList tokens = line2.split(QRegExp("[\r\n\t ]+"));
            QString deviceID = tokens[0];
            
            QString modelID = tokens[3].split(':')[1];
            _modelName = "UNKNOWN";
            if (modelNames.count(modelID) == 1) {
                _modelName = modelNames[modelID];
            }

            _detectedDeviceLabel->setText(_modelName + " [" + deviceID + "]");
            _pullFolderButton->setEnabled(true);
            _folderLineEdit->setEnabled(true);
            _downloadAPKPushbutton->setEnabled(true);
            _installAPKPushbutton->setEnabled(true);
            _runInterfacePushbutton->setEnabled(true);
        }
    }
#endif
}

void TestRunnerMobile::downloadAPK() {
    downloadBuildXml((void*)this);

    downloadComplete();
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

       _downloader->downloadFiles(urls, _workingFolder, filenames, (void*)this);
    } else {
        _statusLabel->setText("Installer download complete");
    }

    _installAPKPushbutton->setEnabled(true);
}

void TestRunnerMobile::installAPK() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    if (!_adbInterface) {
        _adbInterface = new AdbInterface();
    }

    QString installerPathname = QFileDialog::getOpenFileName(nullptr, "Please select the APK", _workingFolder,
        "Available APKs (*.apk)"
    );

    if (installerPathname.isNull()) {
        return;
    }

    // Remove the path
    QStringList parts = installerPathname.split('/');
    _installerFilename = parts[parts.length() - 1];

    _statusLabel->setText("Installing");
    QString command = _adbInterface->getAdbCommand() + " install -r -d " + installerPathname + " >" + _workingFolder  + "/installOutput.txt";
    appendLog(command);
    system(command.toStdString().c_str());
    _statusLabel->setText("Installation complete");
#endif
}

void TestRunnerMobile::runInterface() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    if (!_adbInterface) {
        _adbInterface = new AdbInterface();
    }

    _statusLabel->setText("Starting Interface");

    QString testScript = (_runFullSuite->isChecked())
        ? QString("https://raw.githubusercontent.com/") + nitpick->getSelectedUser() + "/hifi_tests/" + nitpick->getSelectedBranch() + "/tests/testRecursive.js"
        : _scriptURL->text();
 
    // Quest and Android have different commands to run interface
    QString startCommand;
    if (_modelName == "Quest") {
        startCommand = "io.highfidelity.questInterface/.PermissionsChecker";
    } else {
        startCommand = "io.highfidelity.hifiinterface/.PermissionChecker";
    }

    QString command = _adbInterface->getAdbCommand() +
        " shell am start -n " + startCommand +
        " --es args \\\"" +
        " --url file:///~/serverless/tutorial.json" +
        " --no-updater" +
        " --no-login-suggestion" +
        " --testScript " + testScript + " quitWhenFinished" +
        " --testResultsLocation /sdcard/snapshots" +
        "\\\"";

    appendLog(command);
    system(command.toStdString().c_str());
    _statusLabel->setText("Interface started");
#endif
}

void TestRunnerMobile::pullFolder() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    if (!_adbInterface) {
        _adbInterface = new AdbInterface();
    }

    _statusLabel->setText("Pulling folder");
    QString command = _adbInterface->getAdbCommand() + " pull " + _folderLineEdit->text() + " " + _workingFolder;
    appendLog(command);
    system(command.toStdString().c_str());
    _statusLabel->setText("Pull complete");
#endif
}
