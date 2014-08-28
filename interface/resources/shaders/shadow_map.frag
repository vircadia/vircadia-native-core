#version 120

//
//  shadow_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 11/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

uniform sampler2DShadow shadowMap;

// the inverse of the size of the shadow map
const float shadowScale = 1.0 / 2048.0;

// the color in shadow
varying vec4 shadowColor;

void main(void) {
    gl_FragColor = mix(shadowColor, gl_Color, 0.25 *
        (shadow2D(shadowMap, gl_TexCoord[0].stp + vec3(-shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[0].stp + vec3(-shadowScale, shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[0].stp + vec3(shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[0].stp + vec3(shadowScale, shadowScale, 0.0)).r));
}
