#version 120

//
//  grid.vert
//  vertex shader
//
//  Created by Ryan Huffman on 12/30/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

varying vec2 modelCoords;

void main(void) {
    modelCoords = gl_Vertex.xy;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xy, 0, 1);
    gl_FrontColor = gl_Color;
}
