#version 120

//
//  passthrough.vert
//  vertex shader
//
//  Copyright 2013 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
 
attribute float voxelSizeIn;
varying float voxelSize;

void main(void) {
    gl_FrontColor = gl_Color;
    gl_Position = gl_Vertex; // don't call ftransform(), because we do projection in geometry shader
    voxelSize = voxelSizeIn;
}