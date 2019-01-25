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

#include <QFile>
#include <QMessageBox>
#include <QProcess>

PythonInterface::PythonInterface() {
#ifdef Q_OS_WIN
    if (QProcessEnvironment::systemEnvironment().contains("PYTHON_PATH")) {
        QString _pythonPath = QProcessEnvironment::systemEnvironment().value("PYTHON_PATH");
        if (!QFile::exists(_pythonPath + "/" + _pythonExe)) {
            QMessageBox::critical(0, _pythonExe, QString("Python executable not found in ") + _pythonPath);
            exit(-1);
        }
        
        _pythonCommand = _pythonPath + "/" + _pythonExe;
    } else {
        QMessageBox::critical(0, "PYTHON_PATH not defined",
                              "Please set PYTHON_PATH to directory containing the Python executable");
        exit(-1);
    }
#elif defined Q_OS_MAC
    _pythonCommand = "/usr/local/bin/python3";
    if (!QFile::exists(_pythonCommand)) {
        QMessageBox::critical(0, "PYTHON_PATH not defined",
                              "python3 not found at " + _pythonCommand);
        exit(-1);
    }
#endif
}

QString PythonInterface::getPythonCommand() {
    return _pythonCommand;
}
