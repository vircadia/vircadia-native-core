//
//  LocationScriptingInterface.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocationScriptingInterface_h
#define hifi_LocationScriptingInterface_h

#include <qscriptengine.h>

class LocationScriptingInterface : public QObject {
    Q_OBJECT
public:
    static LocationScriptingInterface* getInstance();

    static QScriptValue locationGetter(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue locationSetter(QScriptContext* context, QScriptEngine* engine);
private:
    LocationScriptingInterface() {};
};

#endif // hifi_LocationScriptingInterface_h
