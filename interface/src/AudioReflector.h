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

    void drawRays();
    void drawVector(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

    void echoReflections(const glm::vec3& origin, const QVector<glm::vec3>& reflections, const QByteArray& samples, 
                                        unsigned int sampleTime, int sampleRate);

    QVector<glm::vec3> calculateReflections(const glm::vec3& earPosition, const glm::vec3& origin, const glm::vec3& originalDirection);
    void calculateDiffusions(const glm::vec3& earPosition, const glm::vec3& origin, 
                const glm::vec3& thisReflection, float thisDistance, float thisAttenuation, int thisBounceCount, 
                BoxFace thisReflectionFace, QVector<glm::vec3> reflectionPoints);


    void drawReflections(const glm::vec3& origin, const glm::vec3& originalColor, const QVector<glm::vec3>& reflections);

    void calculateAllReflections();
    void reset();
    float getDistanceAttenuationCoefficient(float distance);
    
    int _reflections;

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

    glm::vec3 _origin;
    glm::quat _orientation;
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
    
    QMutex _mutex;

    float _preDelay;
    float _soundMsPerMeter;
    float _distanceAttenuationScalingFactor;

    int _diffusionFanout; // number of points of diffusion from each reflection point
    
};


#endif /* defined(__interface__AudioReflector__) */
