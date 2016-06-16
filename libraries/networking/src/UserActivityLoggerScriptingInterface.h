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

#ifndef hifi_UserActivityLoggerScriptingInterface_h
#define hifi_UserActivityLoggerScriptingInterface_h

#include <QObject>
#include <QVariantMap>

class QScriptValue;

class UserActivityLoggerScriptingInterface : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void logAction(QString action, QVariantMap details) const;
};

#endif // hifi_UserActivityLoggerScriptingInterface_h