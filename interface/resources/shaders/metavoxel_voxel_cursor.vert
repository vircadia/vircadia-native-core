#version 120

//
//  metavoxel_voxel_cursor.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 10/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

void main(void) {
    // compute the view space coordinates
    vec4 viewPosition = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = gl_ProjectionMatrix * viewPosition;
    
    // generate the texture coordinates from the view position
    gl_TexCoord[0] = vec4(dot(viewPosition, gl_EyePlaneS[4]), dot(viewPosition, gl_EyePlaneT[4]),
        dot(viewPosition, gl_EyePlaneR[4]), 1.0);
    
    // copy the color for interpolation
    gl_FrontColor = gl_Color;
}
