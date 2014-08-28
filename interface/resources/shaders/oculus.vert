#version 120

//
//  oculus.vert
//  vertex shader
//
//  Created by Ben Arnold on 6/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  this shader is an adaptation (HLSL -> GLSL) of the one in the
//  Oculus_SDK_Overview.pdf for the 3.2 SDK.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

uniform vec2 EyeToSourceUVScale;
uniform vec2 EyeToSourceUVOffset;
uniform mat4 EyeRotationStart;
uniform mat4 EyeRotationEnd;

attribute vec2 position;
attribute vec4 color;
attribute vec2 texCoord0;
attribute vec2 texCoord1;
attribute vec2 texCoord2;

varying float vFade;
varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;

vec2 TimewarpTexCoord(vec2 texCoord, mat4 rotMat)
{
    // Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic
    // aberration and distortion). These are now "real world" vectors in direction (x,y,1)
    // relative to the eye of the HMD. Apply the 3x3 timewarp rotation to these vectors.
    vec3 transformed = vec3( rotMat * vec4(texCoord.xy, 1, 1) );
    
    // Project them back onto the Z=1 plane of the rendered images.
    vec2 flattened = (transformed.xy / transformed.z);
    
    // Scale them into ([0,0.5],[0,1]) or ([0.5,0],[0,1]) UV lookup space (depending on eye)
    return (EyeToSourceUVScale * flattened + EyeToSourceUVOffset);
}

void main()
{
    float timewarpMixFactor = color.a;
    mat4 mixedEyeRot = EyeRotationStart * (1.0 - timewarpMixFactor) + EyeRotationEnd * (timewarpMixFactor);
    
    oTexCoord0 = TimewarpTexCoord(texCoord0, mixedEyeRot);
    oTexCoord1 = TimewarpTexCoord(texCoord1, mixedEyeRot);
    oTexCoord2 = TimewarpTexCoord(texCoord2, mixedEyeRot);
    
    //Flip y texture coordinates
    oTexCoord0.y = 1.0 - oTexCoord0.y;
    oTexCoord1.y = 1.0 - oTexCoord1.y;
    oTexCoord2.y = 1.0 - oTexCoord2.y;
    
    gl_Position = vec4(position.xy, 0.5, 1.0);
    vFade = color.r; // For vignette fade
}