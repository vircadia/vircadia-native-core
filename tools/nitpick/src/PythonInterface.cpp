//
//  PythonInterface.cpp
//
//  Created by Nissim Hadar on Oct 6, 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PythonInterface.h"
#include <PathUtils.h>

#include <QFile>
#include <QMessageBox>

QString PythonInterface::getPythonCommand() {
#ifdef Q_OS_WIN
    if (_pythonCommand.isNull()) {
        QString pythonPath = PathUtils::getPathToExecutable("python.exe");
        if (!pythonPath.isNull()) {
            _pythonCommand = pythonPath + _pythonExe;
        } else {
            QMessageBox::critical(0, "python.exe not found",
                "Please verify that pyton.exe is in the PATH");
            exit(-1);
        }
    }
#elif defined Q_OS_MAC
    _pythonCommand = "/usr/local/bin/python3";
    if (!QFile::exists(_pythonCommand)) {
        QMessageBox::critical(0, "python not found",
            "python3 not found at " + _pythonCommand);
        exit(-1);
    }
#endif

    return _pythonCommand;
}
