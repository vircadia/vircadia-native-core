//
//  AvatarData.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#ifndef __hifi__AvatarData__
#define __hifi__AvatarData__

#include <iostream>
#include <AgentData.h>

class AvatarData : public AgentData {
public:
    AvatarData();
    ~AvatarData();
    
    void parseData(void *data, int size);
    AvatarData* clone() const;
    
    float getPitch();
    void setPitch(float pitch);
    float getYaw();
    void setYaw(float yaw);
    float getRoll();
    void setRoll(float roll);
    float getHeadPositionX();
    float getHeadPositionY();
    float getHeadPositionZ();
    void setHeadPosition(float x, float y, float z);
    float getLoudness();
    void setLoudness(float loudness);
    float getAverageLoudness();
    void setAverageLoudness(float averageLoudness);
    float getHandPositionX();
    float getHandPositionY();
    float getHandPositionZ();
    void setHandPosition(float x, float y, float z);

private:
    float _pitch;
    float _yaw;
    float _roll;
    float _headPositionX;
    float _headPositionY;
    float _headPositionZ;
    float _loudness;
    float _averageLoudness;
    float _handPositionX;
    float _handPositionY;
    float _handPositionZ;
};

#endif /* defined(__hifi__AvatarData__) */
