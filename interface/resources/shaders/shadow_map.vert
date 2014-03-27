#version 120

//
//  shadow_map.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 3/27/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

varying vec4 shadowColor;

void main(void) {
    // the shadow color depends on the light product
    vec4 normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    float lightProduct = dot(normal, gl_LightSource[0].position);
    shadowColor = mix(vec4(1.0, 1.0, 1.0, 1.0), vec4(0.8, 0.8, 0.8, 1.0), step(0.0, lightProduct));
    
    // standard diffuse lighting
    gl_FrontColor = gl_Color * (gl_LightModel.ambient + gl_LightSource[0].ambient +
        gl_LightSource[0].diffuse * max(0.0, lightProduct));
    
    // generate the shadow texture coordinate using the eye position
    vec4 eyePosition = gl_ModelViewMatrix * gl_Vertex;
    gl_TexCoord[0] = vec4(dot(gl_EyePlaneS[0], eyePosition), dot(gl_EyePlaneT[0], eyePosition),
        dot(gl_EyePlaneR[0], eyePosition), 1.0); 
    
    // use the fixed function transform
    gl_Position = ftransform();
}
