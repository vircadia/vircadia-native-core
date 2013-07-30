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

// the aspect ratio of the image
uniform float aspectRatio;

// the depth texture
uniform sampler2D depthTexture;

void main(void) {
    gl_TexCoord[0] = vec4(texCoordCorner + gl_Vertex.x * texCoordRight + gl_Vertex.y * texCoordUp, 0.0, 1.0);
    float depth = texture2D(depthTexture, gl_TexCoord[0].st).r;
    
    // set alpha to zero for invalid depth values
    const float MIN_VISIBLE_DEPTH = 1.0 / 255.0;
    const float MAX_VISIBLE_DEPTH = 254.0 / 255.0;
    gl_FrontColor = vec4(1.0, 1.0, 1.0, step(MIN_VISIBLE_DEPTH, depth) * (1.0 - step(MAX_VISIBLE_DEPTH, depth)));
    gl_Position = gl_ModelViewProjectionMatrix * vec4(0.5 - gl_Vertex.x,
        (gl_Vertex.y - 0.5) / aspectRatio, depth * 2.0 - 2.0, 1.0);
}
