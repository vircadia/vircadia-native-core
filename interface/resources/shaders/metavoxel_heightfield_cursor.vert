#version 120

//
//  metavoxel_heighfield_cursor.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 8/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the height texture
uniform sampler2D heightMap;

// the distance between height points in texture space
uniform float heightScale;

void main(void) {
    // compute the view space coordinates
    float height = texture2D(heightMap, gl_MultiTexCoord0.st).r;
    vec4 viewPosition = gl_ModelViewMatrix * (gl_Vertex + vec4(0.0, height, 0.0, 0.0));
    gl_Position = gl_ProjectionMatrix * viewPosition;

    // generate the texture coordinates from the view position
    gl_TexCoord[0] = vec4(dot(viewPosition, gl_EyePlaneS[4]), dot(viewPosition, gl_EyePlaneT[4]), 0.0, 1.0);
    
    // the zero height should be invisible
    gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0 - step(height, 0.0));
}
