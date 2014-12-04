#version 120

//
//  simple.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 9/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the interpolated normal
varying vec4 normal;

// the glow intensity
uniform float glowIntensity;

void main(void) {
    // set the diffuse, normal, specular data
    gl_FragData[0] = vec4(1.0f, 1.0f, 0.0f, 0.0f); //vec4(gl_Color.rgb, glowIntensity);
    gl_FragData[1] = vec4(1.0f, 1.0f, 0.0f, 0.0f); //normalize(normal) * 0.5 + vec4(0.5, 0.5, 0.5, 1.0);
    gl_FragData[2] = vec4(1.0f, 1.0f, 0.0f, 0.0f); //vec4(gl_FrontMaterial.specular.rgb, gl_FrontMaterial.shininess / 128.0);
}
