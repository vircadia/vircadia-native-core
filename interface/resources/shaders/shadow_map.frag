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

varying vec4 shadowColor;

void main(void) {
    gl_FragColor = mix(shadowColor, gl_Color, shadow2D(shadowMap, gl_TexCoord[0].stp).r *
        shadow2D(shadowMap, gl_TexCoord[1].stp).r *
        shadow2D(shadowMap, gl_TexCoord[2].stp).r *
        shadow2D(shadowMap, gl_TexCoord[3].stp).r);
}
