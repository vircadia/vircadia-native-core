//
//  StackTestScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Clement Brisset on 7/25/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StackTestScriptingInterface.h"

#include <QLoggingCategory>
#include <QCoreApplication>

Q_DECLARE_LOGGING_CATEGORY(stackTest)
Q_LOGGING_CATEGORY(stackTest, "hifi.tools.stack-test")

void StackTestScriptingInterface::pass(QString message) {
    qCInfo(stackTest) << "PASS" << qPrintable(message);
}

void StackTestScriptingInterface::fail(QString message) {
    qCInfo(stackTest) << "FAIL" << qPrintable(message);
}

void StackTestScriptingInterface::exit(QString message) {
    qCInfo(stackTest) << "COMPLETE" << qPrintable(message);
    qApp->exit();
}
