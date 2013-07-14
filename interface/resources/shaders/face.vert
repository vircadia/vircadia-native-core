#version 120

//
//  face.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 7/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the lower left texture coordinate
uniform vec2 texCoordCorner;

// the texture coordinate vector from left to right
uniform vec2 texCoordRight;

// the texture coordinate vector from bottom to the top
uniform vec2 texCoordUp;

// the depth texture
uniform sampler2D depthTexture;

void main(void) {
    gl_TexCoord[0] = vec4(texCoordCorner + gl_Vertex.x * texCoordRight + gl_Vertex.y * texCoordUp, 0.0, 1.0);
    gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);
    gl_Position = gl_ModelViewProjectionMatrix * vec4(0.5 - gl_Vertex.x,
        gl_Vertex.y * length(texCoordUp) / length(texCoordRight), -texture2D(depthTexture, gl_TexCoord[0].st).r, 1.0);
}
