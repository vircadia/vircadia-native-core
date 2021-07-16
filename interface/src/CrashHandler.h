//
//  CrashHandler.h
//  interface/src
//
//  Created by Clement Brisset on 01/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CrashHandler_h
#define hifi_CrashHandler_h

#include <string>
#include <QCoreApplication>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(crash_handler)

bool startCrashHandler(std::string appPath);
void setCrashAnnotation(std::string name, std::string value);
void startCrashHookMonitor(QCoreApplication* app);

#endif // hifi_CrashHandler_h
