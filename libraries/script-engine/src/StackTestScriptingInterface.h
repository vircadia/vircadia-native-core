//
//  StackTestScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Clement Brisset on 7/25/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#pragma once

#ifndef hifi_StackTestScriptingInterface_h
#define hifi_StackTestScriptingInterface_h

#include <QObject>

class StackTestScriptingInterface : public QObject {
    Q_OBJECT

public:
    StackTestScriptingInterface(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void pass(QString message = QString());
    Q_INVOKABLE void fail(QString message = QString());

    Q_INVOKABLE void exit(QString message = QString());
};

#endif // hifi_StackTestScriptingInterface_h

/// @}
