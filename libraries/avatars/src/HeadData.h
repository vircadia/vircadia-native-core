//
//  HeadData.h
//  hifi
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__HeadData__
#define __hifi__HeadData__

#include <iostream>

#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// degrees
const float MIN_HEAD_YAW = -110.f;
const float MAX_HEAD_YAW = 110.f;
const float MIN_HEAD_PITCH = -60.f;
const float MAX_HEAD_PITCH = 60.f;
const float MIN_HEAD_ROLL = -50.f;
const float MAX_HEAD_ROLL = 50.f;

class AvatarData;

class HeadData {
public:
    HeadData(AvatarData* owningAvatar);
    virtual ~HeadData() { };
    
    // degrees
    float getLeanSideways() const { return _leanSideways; }
    void setLeanSideways(float leanSideways) { _leanSideways = leanSideways; }
    float getLeanForward() const { return _leanForward; }
    void setLeanForward(float leanForward) { _leanForward = leanForward; }
    float getYaw() const { return _yaw; }
    void setYaw(float yaw) { _yaw = glm::clamp(yaw, MIN_HEAD_YAW, MAX_HEAD_YAW); }
    float getPitch() const { return _pitch; }
    void setPitch(float pitch) { _pitch = glm::clamp(pitch, MIN_HEAD_PITCH, MAX_HEAD_PITCH); }
    float getRoll() const { return _roll; }
    void setRoll(float roll) { _roll = glm::clamp(roll, MIN_HEAD_ROLL, MAX_HEAD_ROLL); }
    virtual float getTweakedYaw() const { return _yaw; }
    virtual float getTweakedPitch() const { return _pitch; }
    virtual float getTweakedRoll() const { return _roll; }

    glm::quat getOrientation() const;
    void setOrientation(const glm::quat& orientation);

    float getAudioLoudness() const { return _audioLoudness; }
	void setAudioLoudness(float audioLoudness) { _audioLoudness = audioLoudness; }

    float getAudioAverageLoudness() const { return _audioAverageLoudness; }
	void setAudioAverageLoudness(float audioAverageLoudness) { _audioAverageLoudness = audioAverageLoudness; }

    const QVector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }
    
    float getPupilDilation() const { return _pupilDilation; }
    void setPupilDilation(float pupilDilation) { _pupilDilation = pupilDilation; }
    
    // degrees
    void addYaw(float yaw);
    void addPitch(float pitch);
    void addRoll(float roll);
    void addLean(float sideways, float forwards);
    
    const glm::vec3& getLookAtPosition() const { return _lookAtPosition; }
    void setLookAtPosition(const glm::vec3& lookAtPosition) { _lookAtPosition = lookAtPosition; }
    
    friend class AvatarData;
    
protected:
    // degrees
    float _yaw;
    float _pitch;
    float _roll;
    float _leanSideways;
    float _leanForward;

    glm::vec3 _lookAtPosition;
    float _audioLoudness;
    bool _isFaceshiftConnected;
    float _leftEyeBlink;
    float _rightEyeBlink;
    float _averageLoudness;
    float _browAudioLift;
    float _audioAverageLoudness;
    QVector<float> _blendshapeCoefficients;
    float _pupilDilation;
    AvatarData* _owningAvatar;
    
private:
    // privatize copy ctor and assignment operator so copies of this object cannot be made
    HeadData(const HeadData&);
    HeadData& operator= (const HeadData&);
};

#endif /* defined(__hifi__HeadData__) */
