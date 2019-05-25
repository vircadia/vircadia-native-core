//
//  AdbInterface.cpp
//
//  Created by Nissim Hadar on Feb 11, 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AdbInterface.h"
#include <PathUtils.h>

#include <QFile>
#include <QMessageBox>

QString AdbInterface::getAdbCommand() {
#ifdef Q_OS_WIN
    if (_adbCommand.isNull()) {
        QString adbExe{ "adb.exe" };
        QString adbPath = PathUtils::getPathToExecutable(adbExe);
        if (!adbPath.isNull()) {
            _adbCommand = adbExe;
        } else {
            QMessageBox::critical(0, "adb.exe not found",
                "Please verify that adb.exe is in the PATH");
            exit(-1);
        }
    }
#elif defined Q_OS_MAC
    _adbCommand = "/usr/local/bin/adb";
    if (!QFile::exists(_adbCommand)) {
        QMessageBox::critical(0, "adb not found",
            "adb not found at " + _adbCommand);
        exit(-1);
    }
#endif

    return _adbCommand;
}
