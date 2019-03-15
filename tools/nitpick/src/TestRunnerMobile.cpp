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

#include <QNetworkInterface>
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
    _downloadAPKPushbutton->setEnabled(true);
}

void TestRunnerMobile::connectDevice() {
#if defined Q_OS_WIN || defined Q_OS_MAC
    if (!_adbInterface) {
        _adbInterface = new AdbInterface();
    }
    
    // Get list of devices
    QString devicesFullFilename{ _workingFolder + "/devices.txt" };
    QString command = _adbInterface->getAdbCommand() + " devices -l > " + devicesFullFilename;
    appendLog(command);
    system(command.toStdString().c_str());

    if (!QFile::exists(devicesFullFilename)) {
        QMessageBox::critical(0, "Internal error", "devices.txt not found");
        exit (-1);
    }

    // Get device IP address
    QString ifconfigFullFilename{ _workingFolder + "/ifconfig.txt" };
    command = _adbInterface->getAdbCommand() + " shell ifconfig > " + ifconfigFullFilename;
    appendLog(command);
    system(command.toStdString().c_str());

    if (!QFile::exists(ifconfigFullFilename)) {
        QMessageBox::critical(0, "Internal error", "ifconfig.txt not found");
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

    sendServerIPToDevice();

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

void TestRunnerMobile::sendServerIPToDevice() {
    // Get device IP
    QFile ifconfigFile{ _workingFolder + "/ifconfig.txt" };
    if (!ifconfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
            "Could not open 'ifconfig.txt'");
        exit(-1);
    }

    QTextStream stream(&ifconfigFile);
    QString line = ifconfigFile.readLine();
    while (!line.isNull()) {
        // The device IP is in the line following the "wlan0" line
        line = ifconfigFile.readLine();
        if (line.left(6) == "wlan0 ") {
            break;
        }
    }

    // The following line looks like this "inet addr:192.168.0.15  Bcast:192.168.0.255  Mask:255.255.255.0"
    // Extract the address and mask
    line = ifconfigFile.readLine();
    QStringList lineParts = line.split(':');
    if (lineParts.size() < 4) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
            "IP address line not in expected format: " + line);
        exit(-1);
    }

    qint64 deviceIP = convertToBinary(lineParts[1].split(' ')[0]);
    qint64 deviceMask = convertToBinary(lineParts[3].split(' ')[0]);
    qint64 deviceSubnet = deviceMask & deviceIP;

    // The device needs to be on the same subnet as the server
    // To find which of our IPs is the server - choose the 1st that is on the same subnet
    // If more than one found then report an error

    QString serverIP;

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (int i = 0; i < interfaces.count(); i++) {
        QList<QNetworkAddressEntry> entries = interfaces.at(i).addressEntries();
        for (int j = 0; j < entries.count(); j++) {
            if (entries.at(j).ip().protocol() == QAbstractSocket::IPv4Protocol) {
                qint64 hostIP = convertToBinary(entries.at(j).ip().toString());
                qint64 hostMask = convertToBinary(entries.at(j).netmask().toString());
                qint64 hostSubnet = hostMask & hostIP;

                if (hostSubnet == deviceSubnet) {
                    if (!serverIP.isNull()) {
                        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                            "Cannot identify server IP (multiple interfaces on device submask)");
                        return;
                    } else {
                        union {
                            uint32_t ip;
                            uint8_t  bytes[4];
                        } u;
                        u.ip = hostIP;

                        serverIP = QString::number(u.bytes[3]) + '.' + QString::number(u.bytes[2]) + '.' + QString::number(u.bytes[1]) + '.' + QString::number(u.bytes[0]);
                    }
                }
            }
        }
    }

    ifconfigFile.close();
}

qint64 TestRunnerMobile::convertToBinary(const QString& str) {
    QString binary;
    foreach (const QString& s, str.split(".")) {
        binary += QString::number(s.toInt(), 2).rightJustified(8, '0');
    }

    return binary.toLongLong(NULL, 2);
}