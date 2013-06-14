#version 120

//
//  iris.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 6/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the interpolated normal
varying vec4 normal;

void main(void) {

    // transform and store the normal for interpolation
    normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    
    // compute standard diffuse lighting per-vertex
    gl_FrontColor = gl_Color * (gl_LightModel.ambient + gl_LightSource[0].ambient +
        gl_LightSource[0].diffuse * max(0.0, dot(normal, gl_LightSource[0].position)));
    
    // copy the tex coordinate for interpolation
    gl_TexCoord[0] = gl_MultiTexCoord0;
    
    // use standard pipeline transform
    gl_Position = ftransform();
}
