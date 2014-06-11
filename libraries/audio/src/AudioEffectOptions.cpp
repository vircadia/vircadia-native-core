//
//  AudioEffectOptions.cpp
//  hifi
//

#include "AudioEffectOptions.h"

AudioEffectOptions::AudioEffectOptions() { }

QScriptValue AudioEffectOptions::constructor(QScriptContext* context, QScriptEngine* engine) {
    return engine->newQObject(new AudioEffectOptions());
}
