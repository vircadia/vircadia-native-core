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
    if (QProcessEnvironment::systemEnvironment().contains("PYTHON_PATH")) {
        QString _pythonPath = QProcessEnvironment::systemEnvironment().value("PYTHON_PATH");
        if (!QFile::exists(_pythonPath + "/" + _pythonExe)) {
            QMessageBox::critical(0, _pythonExe, QString("Python executable not found in ") + _pythonPath);
        }
        _pythonCommand = _pythonPath + "/" + _pythonExe;
    } else {
        QMessageBox::critical(0, "PYTHON_PATH not defined",
                              "Please set PYTHON_PATH to directory containing the Python executable");
        exit(-1);
    }
}

QString PythonInterface::getPythonCommand() {
    return _pythonCommand;
}
