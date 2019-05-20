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
        // Use the python launcher as we need python 3, and python 2 may be installed
        // See https://www.python.org/dev/peps/pep-0397/
        const QString pythonLauncherExecutable{ "py.exe" };
        QString pythonPath = PathUtils::getPathToExecutable(pythonLauncherExecutable);
        if (!pythonPath.isNull()) {
            _pythonCommand = pythonPath + _pythonExe;
        }
        else {
            QMessageBox::critical(0, pythonLauncherExecutable + " not found",
                "Please verify that py.exe is in the PATH");
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