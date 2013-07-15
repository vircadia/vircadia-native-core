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

class AmbientOcclusionEffect {
public:
    
    AmbientOcclusionEffect();
    
    void render(int screenWidth, int screenHeight);

private:
    
    GLuint _depthTextureID;
    
    static ProgramObject* _program;
};

#endif /* defined(__interface__AmbientOcclusionEffect__) */
