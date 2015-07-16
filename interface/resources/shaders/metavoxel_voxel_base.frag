#version 120

//
//  metavoxel_voxel_base.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 9/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the interpolated normal
varying vec4 normal;

void main(void) {
    // store the interpolated color and normal
    gl_FragData[0] = gl_Color;
    gl_FragData[1] = normalize(normal) * 0.5 + vec4(0.5, 0.5, 0.5, 1.0);
}
