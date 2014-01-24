#version 120

//
//  grid.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

void main(void) {
    // use the standard exponential fog calculation
    const float FOG_DENSITY = 0.5;
    gl_FragColor = vec4(gl_Color.rgb, exp(-FOG_DENSITY / gl_FragCoord.w));
}
