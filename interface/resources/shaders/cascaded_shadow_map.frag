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

uniform sampler2DShadow shadowMap;

uniform vec3 shadowDistances;

// the color in shadow
varying vec4 shadowColor;

// the interpolated position
varying vec4 position;

void main(void) {
    int shadowIndex = int(dot(step(vec3(position.z), shadowDistances), vec3(1.0, 1.0, 1.0)));
    vec3 texCoord = vec3(dot(gl_EyePlaneS[shadowIndex], position), dot(gl_EyePlaneT[shadowIndex], position),
        dot(gl_EyePlaneR[shadowIndex], position));
    gl_FragColor = mix(shadowColor, gl_Color, shadow2D(shadowMap, texCoord).r);
}
