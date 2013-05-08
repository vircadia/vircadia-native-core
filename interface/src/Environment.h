//
//  Environment.h
//  interface
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Environment__
#define __interface__Environment__

#include "EnvironmentData.h"
#include "InterfaceConfig.h"

class Camera;
class ProgramObject;

class Environment : public EnvironmentData {
public:

    void init();
    void renderAtmosphere(Camera& camera);
    
private:

    ProgramObject* _skyFromAtmosphereProgram;
    int _cameraPosLocation;
    int _lightPosLocation;
    int _invWavelengthLocation;
    int _innerRadiusLocation;
    int _krESunLocation;
    int _kmESunLocation;
    int _kr4PiLocation;
    int _km4PiLocation;
    int _scaleLocation;
    int _scaleDepthLocation;
    int _scaleOverScaleDepthLocation;
    int _gLocation;
    int _g2Location;
};

#endif /* defined(__interface__Environment__) */
