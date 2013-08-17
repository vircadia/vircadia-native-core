//
//  AmbientOcclusionEffect.h
//  interface
//
//  Created by Andrzej Kapolka on 7/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AmbientOcclusionEffect__
#define __interface__AmbientOcclusionEffect__

#include "InterfaceConfig.h"

class ProgramObject;

/// A screen space ambient occlusion effect.
class AmbientOcclusionEffect {
public:
    
    void init();
    
    void render();
    
private:

    ProgramObject* _occlusionProgram;
    int _nearLocation;
    int _farLocation;
    int _leftBottomLocation;
    int _rightTopLocation;
    int _noiseScaleLocation;
    
    ProgramObject* _blurProgram;
    
    GLuint _rotationTextureID;
};

#endif /* defined(__interface__AmbientOcclusionEffect__) */
