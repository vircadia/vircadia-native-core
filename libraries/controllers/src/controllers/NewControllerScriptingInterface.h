//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_NewControllerScriptingInterface_h
#define hifi_Controllers_NewControllerScriptingInterface_h

#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "Mapping.h"

class QScriptValue;

namespace Controllers {
    class NewControllerScriptingInterface : public QObject {
        Q_OBJECT
    
    public:
        Q_INVOKABLE void update();
        Q_INVOKABLE QObject* newMapping();
        Q_INVOKABLE float getValue(const int& source);
    };
}


#endif
