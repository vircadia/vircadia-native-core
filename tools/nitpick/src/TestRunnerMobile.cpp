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

TestRunnerMobile::TestRunnerMobile(QLabel* workingFolderLabel, QPushButton *conectDeviceButton, QPushButton *pullFolderButton, QLabel* detectedDeviceLabel, QObject* parent)
    : QObject(parent) 
{
    _workingFolderLabel = workingFolderLabel;
    _connectDeviceButton = conectDeviceButton;
    _pullFolderButton = pullFolderButton;
    _detectedDeviceLabel = detectedDeviceLabel;
}

TestRunnerMobile::~TestRunnerMobile() {
}

void TestRunnerMobile::setWorkingFolderAndEnableControls() {
    setWorkingFolder(_workingFolderLabel);

    _connectDeviceButton->setEnabled(true);

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
    int result = system(command.toStdString().c_str());

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
            _pullFolderButton->setEnabled(true);
            _detectedDeviceLabel->setText(line2.remove(DEVICE));
        }
    }
}

void TestRunnerMobile::pullFolder() {
    QString command = _adbCommand + " devices > " + _workingFolder + "/devices.txt";
}
