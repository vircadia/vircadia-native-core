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

// the ratio of the indices of refraction
const float refractionEta = 0.75;

void main(void) {

    // transform and store the normal for interpolation
    normal = normalize(gl_ModelViewMatrix * vec4(gl_Normal, 0.0));
    
    // compute standard diffuse lighting per-vertex
    gl_FrontColor = vec4(gl_Color.rgb * (gl_LightModel.ambient.rgb + gl_LightSource[0].ambient.rgb +
        gl_LightSource[0].diffuse.rgb * max(0.0, dot(normal, gl_LightSource[0].position))), gl_Color.a);
    
    // compute the texture coordinate based on where refracted vector hits z=0 in model space
    vec4 incidence = normalize(gl_Vertex - gl_ModelViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0));
    vec4 refracted = refract(incidence, normalize(vec4(gl_Normal, 0.0)), refractionEta);
    gl_TexCoord[0] = (gl_Vertex - (gl_Vertex.z / refracted.z) * refracted) + vec4(0.5, 0.5, 0.0, 0.0);
    
    // use standard pipeline transform
    gl_Position = ftransform();
}
