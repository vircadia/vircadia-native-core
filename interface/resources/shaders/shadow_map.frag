#version 120

//
//  shadow_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 11/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

uniform sampler2DShadow shadowMap;

varying vec4 shadowColor;

void main(void) {
    gl_FragColor = mix(shadowColor, gl_Color, shadow2D(shadowMap, gl_TexCoord[0].stp));
}
