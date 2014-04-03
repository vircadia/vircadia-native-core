#version 120

//
//  metavoxel_point.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 12/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

uniform float pointScale;

void main(void) {

    // standard diffuse lighting
    gl_FrontColor = vec4(gl_Color.rgb * (gl_LightModel.ambient.rgb + gl_LightSource[0].ambient.rgb +
        gl_LightSource[0].diffuse.rgb * max(0.0, dot(gl_NormalMatrix * gl_Normal, gl_LightSource[0].position.xyz))),
            gl_Color.a);
    
    // extract the first three components of the vertex for position
    gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
    
    // the final component is the size in world space
    gl_PointSize = pointScale * gl_Vertex.w / gl_Position.w;
}
