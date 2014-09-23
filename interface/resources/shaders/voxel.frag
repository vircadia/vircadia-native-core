#version 120

//
//  voxel.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 9/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the interpolated normal
varying vec4 normal;

void main(void) {
    // store the color and normal directly
    gl_FragData[0] = gl_Color;
    gl_FragData[1] = normalize(normal) * 0.5 + vec4(0.5, 0.5, 0.5, 1.0);
}
