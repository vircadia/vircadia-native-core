//
//  AudioReflector.h
//  interface
//
//  Created by Brad Hefta-Gaub on 4/2/2014
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AudioReflector__
#define __interface__AudioReflector__

#include <QMutex>

#include <VoxelTree.h>

#include "Audio.h"
#include "avatar/MyAvatar.h"

class AudioPath {
public:
    AudioPath(const glm::vec3& origin = glm::vec3(0), const glm::vec3& direction = glm::vec3(0), float attenuation = 1.0f, 
            float delay = 0.0f, int bounceCount = 0);
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

class AudioPoint {
public:
    glm::vec3 location;
    float delay;
    float attenuation;
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

    void setVoxels(VoxelTree* voxels) { _voxels = voxels; }
    void setMyAvatar(MyAvatar* myAvatar) { _myAvatar = myAvatar; }
    void setAudio(Audio* audio) { _audio = audio; }

    void render();
    
    int getReflections() const { return _reflections; }
    float getAverageDelayMsecs() const { return _averageDelay; }
    float getAverageAttenuation() const { return _averageAttenuation; }
    float getMaxDelayMsecs() const { return _maxDelay; }
    float getMaxAttenuation() const { return _maxAttenuation; }
    float getMinDelayMsecs() const { return _minDelay; }
    float getMinAttenuation() const { return _minAttenuation; }
    float getDelayFromDistance(float distance);
    int getDiffusionPathCount() const { return _diffusionPathCount; }

    void processInboundAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    void processLocalAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);

public slots:

    float getPreDelay() const { return _preDelay; }
    void setPreDelay(float preDelay) { _preDelay = preDelay; }
    float getSoundMsPerMeter() const { return _soundMsPerMeter; } /// ms per meter, larger means slower
    void setSoundMsPerMeter(float soundMsPerMeter) { _soundMsPerMeter = soundMsPerMeter; }
    float getDistanceAttenuationScalingFactor() const { return _distanceAttenuationScalingFactor; } /// ms per meter, larger means slower
    void setDistanceAttenuationScalingFactor(float factor) { _distanceAttenuationScalingFactor = factor; }

    int getDiffusionFanout() const { return _diffusionFanout; } /// number of points of diffusion from each reflection point
    void setDiffusionFanout(int fanout) { _diffusionFanout = fanout; } /// number of points of diffusion from each reflection point
    
signals:
    
private:
    VoxelTree* _voxels; // used to access voxel scene
    MyAvatar* _myAvatar; // access to listener
    Audio* _audio; // access to audio API

    // Helpers for drawing
    void drawRays();
    void drawVector(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

    // OLD helper for playing audio
    void echoReflections(const glm::vec3& origin, const QVector<glm::vec3>& reflections, const QByteArray& samples, 
                                        unsigned int sampleTime, int sampleRate);

    // OLD helper for calculating reflections
    QVector<glm::vec3> calculateReflections(const glm::vec3& earPosition, const glm::vec3& origin, const glm::vec3& originalDirection);
    void calculateDiffusions(const glm::vec3& earPosition, const glm::vec3& origin, 
                const glm::vec3& thisReflection, float thisDistance, float thisAttenuation, int thisBounceCount, 
                BoxFace thisReflectionFace, QVector<glm::vec3> reflectionPoints);


    // OLD helper for drawing refections
    void drawReflections(const glm::vec3& origin, const glm::vec3& originalColor, const QVector<glm::vec3>& reflections);

    // OLD helper for calculating reflections
    void calculateAllReflections();
    
    // resets statistics
    void reset();
    
    // helper for generically calculating attenuation based on distance
    float getDistanceAttenuationCoefficient(float distance);

    // helper for generically calculating attenuation based on bounce count, used in old/non-diffusion mode
    float getBounceAttenuationCoefficient(int bounceCount);
    
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
    
    // old way of doing this...
    QVector<glm::vec3> _frontRightUpReflections;
    QVector<glm::vec3> _frontLeftUpReflections;
    QVector<glm::vec3> _backRightUpReflections;
    QVector<glm::vec3> _backLeftUpReflections;
    QVector<glm::vec3> _frontRightDownReflections;
    QVector<glm::vec3> _frontLeftDownReflections;
    QVector<glm::vec3> _backRightDownReflections;
    QVector<glm::vec3> _backLeftDownReflections;
    QVector<glm::vec3> _frontReflections;
    QVector<glm::vec3> _backReflections;
    QVector<glm::vec3> _leftReflections;
    QVector<glm::vec3> _rightReflections;
    QVector<glm::vec3> _upReflections;
    QVector<glm::vec3> _downReflections;
    
    
    // NOTE: Here's the new way, we will have an array of AudioPaths, we will loop on all of our currently calculating audio 
    // paths, and calculate one ray per path. If that ray doesn't reflect, or reaches a max distance/attenuation, then it
    // is considered finalized.
    // If the ray hits a surface, then, based on the characteristics of that surface, it will create calculate the new 
    // attenuation, path length, and delay for the primary path. For surfaces that have diffusion, it will also create
    // fanout number of new paths, those new paths will have an origin of the reflection point, and an initial attenuation
    // of their diffusion ratio. Those new paths will be added to the active audio paths, and be analyzed for the next loop.
    QVector<AudioPath*> _audioPaths;
    QVector<AudioPoint> _audiblePoints;
    
    // adds a sound source to begin an audio path trace, these can be the initial sound sources with their directional properties,
    // as well as diffusion sound sources
    void addSoundSource(const glm::vec3& origin, const glm::vec3& initialDirection, float initialAttenuation, float initialDelay);
    
    // helper that handles audioPath analysis
    int analyzePathsSingleStep();
    void analyzePaths();
    void newDrawRays();
    void drawPath(AudioPath* path, const glm::vec3& originalColor);
    void newCalculateAllReflections();
    int countDiffusionPaths();
    glm::vec3 getFaceNormal(BoxFace face);

    void injectAudiblePoint(const AudioPoint& audiblePoint, const QByteArray& samples, unsigned int sampleTime, int sampleRate);
    void oldEchoAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    void newEchoAudio(unsigned int sampleTime, const QByteArray& samples, const QAudioFormat& format);
    
    // return the surface characteristics of the element we hit
    SurfaceCharacteristics getSurfaceCharacteristics(OctreeElement* elementHit = NULL);
    
    
    QMutex _mutex;

    float _preDelay;
    float _soundMsPerMeter;
    float _distanceAttenuationScalingFactor;

    int _diffusionFanout; // number of points of diffusion from each reflection point

    // all elements have the same material for now...
    float _absorptionRatio;
    float _diffusionRatio;
    float _reflectiveRatio;
    
    bool _withDiffusion;
};


#endif /* defined(__interface__AudioReflector__) */
