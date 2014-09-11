//
//  DeferredLightingEffect.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 9/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DeferredLightingEffect_h
#define hifi_DeferredLightingEffect_h

#include "ProgramObject.h"

/// Handles deferred lighting for the bits that require it (voxels, metavoxels...)
class DeferredLightingEffect {
public:
    
    void init();
    void render();

private:

    class LightLocations {
    public:
        int shadowDistances;
        int shadowScale;
        int nearLocation;
        int depthScale;
        int depthTexCoordOffset;
        int depthTexCoordScale;
    };
    
    static void loadLightProgram(const char* name, ProgramObject& program, LightLocations& locations);
        
    ProgramObject _directionalLight;
    LightLocations _directionalLightLocations;
    ProgramObject _directionalLightShadowMap;
    LightLocations _directionalLightShadowMapLocations;
    ProgramObject _directionalLightCascadedShadowMap;
    LightLocations _directionalLightCascadedShadowMapLocations;
};

#endif // hifi_DeferredLightingEffect_h
