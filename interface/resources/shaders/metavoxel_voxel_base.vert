#version 120

//
//  metavoxel_voxel_base.vert
//  vertex shader
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
    // transform and store the normal for interpolation
    normal = vec4(normalize(gl_NormalMatrix * gl_Normal), 0.0);
    
    // use the fixed-function position
    gl_Position = ftransform();
    
    // copy the color for interpolation
    gl_FrontColor = vec4(gl_Color.rgb, 0.0);
}
