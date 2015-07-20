//
//  AmbientOcclusionEffect.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 7/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AmbientOcclusionEffect_h
#define hifi_AmbientOcclusionEffect_h

#include <stdint.h>
#include <DependencyManager.h>
#include <gpu/Shader.h>

class AbstractViewStateInterface;

/// A screen space ambient occlusion effect.  See John Chapman's tutorial at
/// http://john-chapman-graphics.blogspot.co.uk/2013/01/ssao-tutorial.html for reference.
class AmbientOcclusionEffect : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    
    void init(AbstractViewStateInterface* viewState);
    void render();
    
private:
    AmbientOcclusionEffect() {}
    virtual ~AmbientOcclusionEffect() {}

    gpu::ShaderPointer _occlusionProgram;
    int _nearLocation;
    int _farLocation;
    int _leftBottomLocation;
    int _rightTopLocation;
    int _noiseScaleLocation;
    int _texCoordOffsetLocation;
    int _texCoordScaleLocation;
    
    gpu::ShaderPointer _blurProgram;
    int _blurScaleLocation;
    
    uint32_t _rotationTextureID;
    AbstractViewStateInterface* _viewState;
};

#endif // hifi_AmbientOcclusionEffect_h
