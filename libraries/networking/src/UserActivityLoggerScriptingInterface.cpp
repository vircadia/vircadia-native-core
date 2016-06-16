//
//  UserActivityLoggerScriptingInterface.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 6/06/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UserActivityLoggerScriptingInterface.h"
#include "UserActivityLogger.h"

void UserActivityLoggerScriptingInterface::logAction(QString action, QVariantMap details) const {
    QMetaObject::invokeMethod(&UserActivityLogger::getInstance(), "logAction",
                              Q_ARG(QString, action),
                              Q_ARG(QJsonObject, QJsonObject::fromVariantMap(details)));
}
