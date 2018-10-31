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
}

QString PythonInterface::getPythonCommand() {
    return _pythonCommand;
}
