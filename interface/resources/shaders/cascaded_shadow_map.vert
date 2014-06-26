#version 120

//
//  cascaded_shadow_map.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 5/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the color in shadow
varying vec4 shadowColor;

// the interpolated position
varying vec4 position;

void main(void) {
    // the shadow color includes only the ambient terms
    shadowColor = gl_Color * (gl_LightModel.ambient + gl_LightSource[0].ambient);
    
    // the normal color includes diffuse
    vec4 normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    gl_FrontColor = shadowColor + gl_Color * (gl_LightSource[0].diffuse * max(0.0, dot(normal, gl_LightSource[0].position)));
    
    // generate the shadow texture coordinates using the eye position
    position = gl_ModelViewMatrix * gl_Vertex;
    
    // use the fixed function transform
    gl_Position = ftransform();
}
