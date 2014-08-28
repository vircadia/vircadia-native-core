#version 120

//
//  cascaded_shadow_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 5/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the shadow texture
uniform sampler2DShadow shadowMap;

// the distances to the cascade sections
uniform vec3 shadowDistances;

// the inverse of the size of the shadow map
const float shadowScale = 1.0 / 2048.0;

// the color in shadow
varying vec4 shadowColor;

// the interpolated position
varying vec4 position;

void main(void) {
    // compute the index of the cascade to use and the corresponding texture coordinates
    int shadowIndex = int(dot(step(vec3(position.z), shadowDistances), vec3(1.0, 1.0, 1.0)));
    vec3 shadowTexCoord = vec3(dot(gl_EyePlaneS[shadowIndex], position), dot(gl_EyePlaneT[shadowIndex], position),
        dot(gl_EyePlaneR[shadowIndex], position));
    
    gl_FragColor = mix(shadowColor, gl_Color, 0.25 *
        (shadow2D(shadowMap, shadowTexCoord + vec3(-shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, shadowTexCoord + vec3(-shadowScale, shadowScale, 0.0)).r +
        shadow2D(shadowMap, shadowTexCoord + vec3(shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, shadowTexCoord + vec3(shadowScale, shadowScale, 0.0)).r));
}
