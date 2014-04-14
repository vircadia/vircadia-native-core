#version 120

//
//  grid.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

void main(void) {
    // use the standard exponential fog calculation
    const float FOG_DENSITY = 0.5;
    gl_FragColor = vec4(gl_Color.rgb, exp(-FOG_DENSITY / gl_FragCoord.w));
}
