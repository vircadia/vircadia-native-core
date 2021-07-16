//
//  KeyEvent.h
//  script-engine/src
//
//  Created by Stephen Birarda on 2014-10-27.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_KeyEvent_h
#define hifi_KeyEvent_h

#include <QKeyEvent>
#include <QScriptValue>

/// Represents a keyboard event to the scripting engine. Exposed as <code><a href="https://apidocs.vircadia.dev/global.html#KeyEvent">KeyEvent</a></code>
class KeyEvent {
public:
    KeyEvent();
    KeyEvent(const QKeyEvent& event);
    bool operator==(const KeyEvent& other) const;
    operator QKeySequence() const;
    
    static QScriptValue toScriptValue(QScriptEngine* engine, const KeyEvent& event);
    static void fromScriptValue(const QScriptValue& object, KeyEvent& event);
    
    int key;
    QString text;
    bool isShifted;
    bool isControl;
    bool isMeta;
    bool isAlt;
    bool isKeypad;
    bool isValid;
    bool isAutoRepeat;
};

Q_DECLARE_METATYPE(KeyEvent)

#endif // hifi_KeyEvent_h

/// @}
