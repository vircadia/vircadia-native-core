//
//  HandControl.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__HandControl__
#define __interface__HandControl__

#include <glm/glm.hpp>

class HandControl {
public:
    HandControl();
    void setScreenDimensions(int width, int height);
    void update( int x, int y );
    glm::vec3 getValues();
    void stop();
    
private:
    bool  _enabled;
    int   _width;
    int   _height;
    int   _startX;
    int   _startY;
    int   _x; 
    int   _y;
    int   _lastX; 
    int   _lastY;
    int   _velocityX;
    int   _velocityY;
    float _rampUpRate;
    float _rampDownRate;
    float _envelope;
    float _leftRight;
    float _downUp;
    float _backFront;			
};

#endif
