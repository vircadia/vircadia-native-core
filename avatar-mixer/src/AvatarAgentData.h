//
//  AvatarAgentData.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//
//

#ifndef __hifi__AvatarAgentData__
#define __hifi__AvatarAgentData__

#include <iostream>
#include <AgentData.h>

const char PACKET_FORMAT[] = "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f";

class AvatarAgentData : public AgentData {
public:
    AvatarAgentData();
    ~AvatarAgentData();
    
    void parseData(void *data, int size);
    AvatarAgentData* clone() const;
    
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

#endif /* defined(__hifi__AvatarAgentData__) */
