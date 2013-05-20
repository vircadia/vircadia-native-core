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

class HeadData {
public:
    HeadData();
    
    float getYaw() const { return _yaw; }
    void setYaw(float yaw);
    
    float getPitch() const { return _pitch; }
    void setPitch(float pitch);
    
    float getRoll() const { return _roll; }
    void setRoll(float roll);
    
    void addYaw(float yaw);
    void addPitch(float pitch);
    void addRoll(float roll);
protected:
    float _yaw;
    float _pitch;
    float _roll;
};

#endif /* defined(__hifi__HeadData__) */
