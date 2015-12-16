//
//  AudioEffectOptions.h
//  libraries/audio/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioEffectOptions_h
#define hifi_AudioEffectOptions_h

#include <QObject>
#include <QtScript/QScriptContext>
#include <QtScript/QScriptEngine>

class AudioEffectOptions : public QObject {
    Q_OBJECT

    Q_PROPERTY(float bandwidth READ getBandwidth WRITE setBandwidth)
    Q_PROPERTY(float preDelay READ getPreDelay WRITE setPreDelay)
    Q_PROPERTY(float lateDelay READ getLateDelay WRITE setLateDelay)
    Q_PROPERTY(float reverbTime READ getReverbTime WRITE setReverbTime)
    Q_PROPERTY(float earlyDiffusion READ getEarlyDiffusion WRITE setEarlyDiffusion)
    Q_PROPERTY(float lateDiffusion READ getLateDiffusion WRITE setLateDiffusion)
    Q_PROPERTY(float roomSize READ getRoomSize WRITE setRoomSize)
    Q_PROPERTY(float density READ getDensity WRITE setDensity)
    Q_PROPERTY(float bassMult READ getBassMult WRITE setBassMult)
    Q_PROPERTY(float bassFreq READ getBassFreq WRITE setBassFreq)
    Q_PROPERTY(float highGain READ getHighGain WRITE setHighGain)
    Q_PROPERTY(float highFreq READ getHighFreq WRITE setHighFreq)
    Q_PROPERTY(float modRate READ getModRate WRITE setModRate)
    Q_PROPERTY(float modDepth READ getModDepth WRITE setModDepth)
    Q_PROPERTY(float earlyGain READ getEarlyGain WRITE setEarlyGain)
    Q_PROPERTY(float lateGain READ getLateGain WRITE setLateGain)
    Q_PROPERTY(float earlyMixLeft READ getEarlyMixLeft WRITE setEarlyMixLeft)
    Q_PROPERTY(float earlyMixRight READ getEarlyMixRight WRITE setEarlyMixRight)
    Q_PROPERTY(float lateMixLeft READ getLateMixLeft WRITE setLateMixLeft)
    Q_PROPERTY(float lateMixRight READ getLateMixRight WRITE setLateMixRight)
    Q_PROPERTY(float wetDryMix READ getWetDryMix WRITE setWetDryMix)

public:
    AudioEffectOptions(QScriptValue arguments = QScriptValue());
    AudioEffectOptions(const AudioEffectOptions &other);
    AudioEffectOptions& operator=(const AudioEffectOptions &other);

    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);

    float getBandwidth() const { return _bandwidth; }
    void setBandwidth(float bandwidth) { _bandwidth = bandwidth; }

    float getPreDelay() const { return _preDelay; }
    void setPreDelay(float preDelay) { _preDelay = preDelay; }

    float getLateDelay() const { return _lateDelay; }
    void setLateDelay(float lateDelay) { _lateDelay = lateDelay; }

    float getReverbTime() const { return _reverbTime; }
    void setReverbTime(float reverbTime) { _reverbTime = reverbTime; }

    float getEarlyDiffusion() const { return _earlyDiffusion; }
    void setEarlyDiffusion(float earlyDiffusion) { _earlyDiffusion = earlyDiffusion; }

    float getLateDiffusion() const { return _lateDiffusion; }
    void setLateDiffusion(float lateDiffusion) { _lateDiffusion = lateDiffusion; }

    float getRoomSize() const { return _roomSize; }
    void setRoomSize(float roomSize) { _roomSize = roomSize; }

    float getDensity() const { return _density; }
    void setDensity(float density) { _density = density; }

    float getBassMult() const { return _bassMult; }
    void setBassMult(float bassMult) { _bassMult = bassMult; }

    float getBassFreq() const { return _bassFreq; }
    void setBassFreq(float bassFreq) { _bassFreq = bassFreq; }

    float getHighGain() const { return _highGain; }
    void setHighGain(float highGain) { _highGain = highGain; }

    float getHighFreq() const { return _highFreq; }
    void setHighFreq(float highFreq) { _highFreq = highFreq; }

    float getModRate() const { return _modRate; }
    void setModRate(float modRate) { _modRate = modRate; }

    float getModDepth() const { return _modDepth; }
    void setModDepth(float modDepth) { _modDepth = modDepth; }

    float getEarlyGain() const { return _earlyGain; }
    void setEarlyGain(float earlyGain) { _earlyGain = earlyGain; }

    float getLateGain() const { return _lateGain; }
    void setLateGain(float lateGain) { _lateGain = lateGain; }

    float getEarlyMixLeft() const { return _earlyMixLeft; }
    void setEarlyMixLeft(float earlyMixLeft) { _earlyMixLeft = earlyMixLeft; }

    float getEarlyMixRight() const { return _earlyMixRight; }
    void setEarlyMixRight(float earlyMixRight) { _earlyMixRight = earlyMixRight; }

    float getLateMixLeft() const { return _lateMixLeft; }
    void setLateMixLeft(float lateMixLeft) { _lateMixLeft = lateMixLeft; }

    float getLateMixRight() const { return _lateMixRight; }
    void setLateMixRight(float lateMixRight) { _lateMixRight = lateMixRight; }

    float getWetDryMix() const { return _wetDryMix; }
    void setWetDryMix(float wetDryMix) { _wetDryMix = wetDryMix; }

private:
    float _bandwidth;        // [20, 24000] Hz

    float _preDelay;         // [0, 333] ms
    float _lateDelay;        // [0, 166] ms

    float _reverbTime;       // [0.1, 100] seconds

    float _earlyDiffusion;   // [0, 100] percent
    float _lateDiffusion;    // [0, 100] percent

    float _roomSize;         // [0, 100] percent
    float _density;          // [0, 100] percent

    float _bassMult;         // [0.1, 10] ratio
    float _bassFreq;         // [10, 500] Hz
    float _highGain;         // [-24, 0] dB
    float _highFreq;         // [1000, 12000] Hz

    float _modRate;          // [0.1, 10] Hz
    float _modDepth;         // [0, 100] percent

    float _earlyGain;        // [-96, +24] dB
    float _lateGain;         // [-96, +24] dB

    float _earlyMixLeft;     // [0, 100] percent
    float _earlyMixRight;    // [0, 100] percent
    float _lateMixLeft;      // [0, 100] percent
    float _lateMixRight;     // [0, 100] percent

    float _wetDryMix;        // [0, 100] percent
};

#endif // hifi_AudioEffectOptions_h
