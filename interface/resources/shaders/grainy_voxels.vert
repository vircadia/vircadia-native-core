//
//  grainy_voxels.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 5/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#version 120

// the position in model space
varying vec3 position;

void main(void) {
    position = gl_Vertex.xyz;
    vec4 normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    gl_FrontColor = gl_Color * (gl_LightModel.ambient + gl_LightSource[0].ambient +
        gl_LightSource[0].diffuse * max(0.0, dot(normal, gl_LightSource[0].position)));
    gl_Position = ftransform();
}
