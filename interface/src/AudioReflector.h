//
//  AudioReflector.h
//  interface
//
//  Created by Brad Hefta-Gaub on 4/2/2014
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef interface_AudioReflector_h
#define interface_AudioReflector_h

#include <QMutex>

#include <VoxelTree.h>

#include "Audio.h"
#include "avatar/MyAvatar.h"

enum AudioSource {
    LOCAL_AUDIO,
    INBOUND_AUDIO
};

class AudioPath {
public:
    AudioPath(AudioSource source = INBOUND_AUDIO, const glm::vec3& origin = glm::vec3(0), 
            const glm::vec3& direction = glm::vec3(0), float attenuation = 1.0f, 
            float delay = 0.0f, float distance = 0.0f, bool isDiffusion = false, int bounceCount = 0);
            
    AudioSource source;
    bool isDiffusion;
    glm::vec3 startPoint;
    glm::vec3 startDirection;
    float startDelay;
    float startAttenuation;

    glm::vec3 lastPoint;
    glm::vec3 lastDirection;
    float lastDistance;
    float lastDelay;
    float lastAttenuation;
    unsigned int bounceCount;

    bool finalized;
    QVector<glm::vec3> reflections;
};

class AudiblePoint {
public:
    glm::vec3 location; /// location of the audible point
    float delay; /// includes total delay including pre delay to the point of the audible location, not to the listener's ears
    float attenuation; /// only the reflective & diffusive portion of attenuation, doesn't include distance attenuation
    float distance; /// includes total distance to the point of the audible location, not to the listener's ears
};

class SurfaceCharacteristics {
public:
    float reflectiveRatio;
    float absorptionRatio;
    float diffusionRatio;
};

class AudioReflector : public QObject {
    Q_OBJECT
public:
    AudioReflector(QObject* parent = NULL);

    // setup functions to configure the resources used by the AudioReflector
    void setVoxels(VoxelTree* voxels) { _voxels = voxels; }
    void setMyAvatar(MyAvatar* myAvatar) { _myAvatar = myAvatar; }
    void setAudio(Audio* audio) { _audio = audio; }

    void render(); /// must be called in the application render loop
    
    void processInboundAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    void processLocalAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);

public slots:
    // statistics
    int getReflections() const { return _reflections; }
    float getAverageDelayMsecs() const { return _averageDelay; }
    float getAverageAttenuation() const { return _averageAttenuation; }
    float getMaxDelayMsecs() const { return _maxDelay; }
    float getMaxAttenuation() const { return _maxAttenuation; }
    float getMinDelayMsecs() const { return _minDelay; }
    float getMinAttenuation() const { return _minAttenuation; }
    float getDelayFromDistance(float distance);
    int getDiffusionPathCount() const { return _diffusionPathCount; }
    int getEchoesInjected() const { return _inboundAudioDelays.size() + _localAudioDelays.size(); }
    int getEchoesSuppressed() const { return _inboundEchoesSuppressed.size() + _localEchoesSuppressed.size(); }

    /// ms of delay added to all echos
    float getPreDelay() const { return _preDelay; }
    void setPreDelay(float preDelay) { _preDelay = preDelay; }

    /// ms per meter that sound travels, larger means slower, which sounds bigger
    float getSoundMsPerMeter() const { return _soundMsPerMeter; }
    void setSoundMsPerMeter(float soundMsPerMeter) { _soundMsPerMeter = soundMsPerMeter; }
    
    /// scales attenuation to be louder or softer than the default distance attenuation
    float getDistanceAttenuationScalingFactor() const { return _distanceAttenuationScalingFactor; }
    void setDistanceAttenuationScalingFactor(float factor) { _distanceAttenuationScalingFactor = factor; }

    /// scales attenuation of local audio to be louder or softer than the default attenuation
    float getLocalAudioAttenuationFactor() const { return _localAudioAttenuationFactor; }
    void setLocalAudioAttenuationFactor(float factor) { _localAudioAttenuationFactor = factor; }

    /// ms window in which we will suppress echoes to reduce comb filter effects
    float getCombFilterWindow() const { return _combFilterWindow; }
    void setCombFilterWindow(float value) { _combFilterWindow = value; }

    /// number of points of diffusion from each reflection point, as fanout increases there are more chances for secondary
    /// echoes, but each diffusion ray is quieter and therefore more likely to be below the sound floor
    int getDiffusionFanout() const { return _diffusionFanout; }
    void setDiffusionFanout(int fanout) { _diffusionFanout = fanout; }
    
    /// ratio 0.0 - 1.0 of amount of each ray that is absorbed upon hitting a surface    
    float getAbsorptionRatio() const { return _absorptionRatio; }
    void setAbsorptionRatio(float ratio);

    // ratio 0.0 - 1.0 of amount of each ray that is diffused upon hitting a surface    
    float getDiffusionRatio() const { return _diffusionRatio; }
    void setDiffusionRatio(float ratio);
    
    // remaining ratio 0.0 - 1.0 of amount of each ray that is cleanly reflected upon hitting a surface    
    float getReflectiveRatio() const { return (1.0f - (_absorptionRatio + _diffusionRatio)); }
    void setReflectiveRatio(float ratio);

signals:
    
private:
    VoxelTree* _voxels; // used to access voxel scene
    MyAvatar* _myAvatar; // access to listener
    Audio* _audio; // access to audio API

    // Helpers for drawing
    void drawVector(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

    // helper for generically calculating attenuation based on distance
    float getDistanceAttenuationCoefficient(float distance);

    // statistics
    int _reflections;
    int _diffusionPathCount;
    int _delayCount;
    float _totalDelay;
    float _averageDelay;
    float _maxDelay;
    float _minDelay;
    int _attenuationCount;
    float _totalAttenuation;
    float _averageAttenuation;
    float _maxAttenuation;
    float _minAttenuation;

    glm::vec3 _listenerPosition;
    glm::vec3 _origin;
    glm::quat _orientation;
    
    QVector<AudioPath*> _inboundAudioPaths; /// audio paths we're processing for inbound audio
    QVector<AudiblePoint> _inboundAudiblePoints; /// the audible points that have been calculated from the inbound audio paths
    QMap<float, float> _inboundAudioDelays; /// delay times for currently injected audio points
    QVector<float> _inboundEchoesSuppressed; /// delay times for currently injected audio points

    QVector<AudioPath*> _localAudioPaths; /// audio paths we're processing for local audio
    QVector<AudiblePoint> _localAudiblePoints; /// the audible points that have been calculated from the local audio paths
    QMap<float, float> _localAudioDelays; /// delay times for currently injected audio points
    QVector<float> _localEchoesSuppressed; /// delay times for currently injected audio points

    // adds a sound source to begin an audio path trace, these can be the initial sound sources with their directional properties,
    // as well as diffusion sound sources
    void addAudioPath(AudioSource source, const glm::vec3& origin, const glm::vec3& initialDirection, float initialAttenuation, 
                            float initialDelay, float initialDistance = 0.0f, bool isDiffusion = false);
    
    // helper that handles audioPath analysis
    int analyzePathsSingleStep();
    void handlePathPoint(AudioPath* path, float distance, OctreeElement* elementHit, BoxFace face);
    void clearPaths();
    void analyzePaths();
    void drawRays();
    void drawPath(AudioPath* path, const glm::vec3& originalColor);
    void calculateAllReflections();
    int countDiffusionPaths();
    glm::vec3 getFaceNormal(BoxFace face);

    void injectAudiblePoint(AudioSource source, const AudiblePoint& audiblePoint, const QByteArray& samples, unsigned int sampleTime, int sampleRate);
    void echoAudio(AudioSource source, unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    
    // return the surface characteristics of the element we hit
    SurfaceCharacteristics getSurfaceCharacteristics(OctreeElement* elementHit = NULL);
    
    
    QMutex _mutex;

    float _preDelay;
    float _soundMsPerMeter;
    float _distanceAttenuationScalingFactor;
    float _localAudioAttenuationFactor;
    float _combFilterWindow;

    int _diffusionFanout; // number of points of diffusion from each reflection point

    // all elements have the same material for now...
    float _absorptionRatio;
    float _diffusionRatio;
    float _reflectiveRatio;

    // remember the last known values at calculation    
    bool haveAttributesChanged();

    bool _withDiffusion;
    float _lastPreDelay;
    float _lastSoundMsPerMeter;
    float _lastDistanceAttenuationScalingFactor;
    float _lastLocalAudioAttenuationFactor;
    int _lastDiffusionFanout;
    float _lastAbsorptionRatio;
    float _lastDiffusionRatio;
};


#endif // interface_AudioReflector_h
