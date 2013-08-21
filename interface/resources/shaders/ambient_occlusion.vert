#version 120

//
//  ambient_occlusion.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 8/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

void main(void) {
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = gl_Vertex;
}
