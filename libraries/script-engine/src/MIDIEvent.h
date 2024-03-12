//
//  MIDIEvent.h
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2014-06-30.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_MIDIEvent_h
#define hifi_MIDIEvent_h

#include "ScriptValue.h"

class ScriptEngine;

/// Represents a MIDI protocol event to the scripting engine.
class MIDIEvent {
public:
    double deltaTime;
    unsigned int type;
    unsigned int data1;
    unsigned int data2;
};

Q_DECLARE_METATYPE(MIDIEvent)

void registerMIDIMetaTypes(ScriptEngine* engine);

ScriptValue midiEventToScriptValue(ScriptEngine* engine, const MIDIEvent& event);
bool midiEventFromScriptValue(const ScriptValue &object, MIDIEvent& event);

#endif // hifi_MIDIEvent_h

/// @}
