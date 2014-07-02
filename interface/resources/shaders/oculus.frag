#version 120

//
//  oculus.frag
//  fragment shader
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

uniform sampler2D texture;

varying float vFade;
varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;

void main(void) {
    // 3 samples for fixing chromatic aberrations
    float r = texture2D(texture, oTexCoord0.xy).r;
    float g = texture2D(texture, oTexCoord1.xy).g;
    float b = texture2D(texture, oTexCoord2.xy).b;
    
    gl_FragColor = vec4(r * vFade, g * vFade, b * vFade, 1.0);
}
