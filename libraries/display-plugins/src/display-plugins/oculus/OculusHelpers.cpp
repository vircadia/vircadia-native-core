//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OculusHelpers.h"


namespace Oculus {
    ovrHmd _hmd;
    unsigned int _frameIndex{ 0 };
    ovrEyeRenderDesc _eyeRenderDescs[2];
    ovrPosef _eyePoses[2];
    ovrVector3f _eyeOffsets[2];
    ovrFovPort _eyeFovs[2];
    mat4 _eyeProjections[2];
    mat4 _compositeEyeProjections[2];
    uvec2 _desiredFramebufferSize;
}


