#version 120

//
//  oculus.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 11/26/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
// this shader is an adaptation (HLSL -> GLSL, removed conditional) of the one in the Oculus sample
// code (Samples/OculusRoomTiny/RenderTiny_D3D1X_Device.cpp), which is under the Apache license
// (http://www.apache.org/licenses/LICENSE-2.0)
//

uniform sampler2D texture;

uniform vec2 lensCenter;
uniform vec2 screenCenter;
uniform vec2 scale;
uniform vec2 scaleIn;
uniform vec4 hmdWarpParam;

vec2 hmdWarp(vec2 in01) {
    vec2 theta = (in01 - lensCenter) * scaleIn;
    float rSq = theta.x * theta.x + theta.y * theta.y;
    vec2 theta1 = theta * (hmdWarpParam.x + hmdWarpParam.y * rSq +
    hmdWarpParam.z * rSq * rSq + hmdWarpParam.w * rSq * rSq * rSq);
    return lensCenter + scale * theta1;
}

void main(void) {
    vec2 tc = hmdWarp(gl_TexCoord[0].st);
    vec2 below = step(screenCenter.st + vec2(-0.25, -0.5), tc.st);
    vec2 above = vec2(1.0, 1.0) - step(screenCenter.st + vec2(0.25, 0.5), tc.st);
    gl_FragColor = mix(vec4(0.0, 0.0, 0.0, 1.0), texture2D(texture, tc), above.s * above.t * below.s * below.t);
}
