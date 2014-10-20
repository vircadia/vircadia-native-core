//
//  AudioEffectOptions.cpp
//  hifi
//

#include "AudioEffectOptions.h"

AudioEffectOptions::AudioEffectOptions() :
    _maxRoomSize(50.0f),
    _roomSize(50.0f),
    _reverbTime(4.0f),
    _damping(0.5f),
    _spread(15.0f),
    _inputBandwidth(0.75f),
    _earlyLevel(-22.0f),
    _tailLevel(-28.0f),
    _dryLevel(0.0f),
    _wetLevel(6.0f) {
}

QScriptValue AudioEffectOptions::constructor(QScriptContext* context, QScriptEngine* engine) {
    return engine->newQObject(new AudioEffectOptions());
}
