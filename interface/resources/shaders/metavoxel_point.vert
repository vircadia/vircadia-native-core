#version 120

//
//  metavoxel_point.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 12/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

uniform float pointScale;

void main(void) {

    // standard diffuse lighting
    gl_FrontColor = vec4(gl_Color.rgb * (gl_LightModel.ambient.rgb + gl_LightSource[0].ambient.rgb +
        gl_LightSource[0].diffuse.rgb * max(0.0, dot(gl_NormalMatrix * gl_Normal, gl_LightSource[0].position.xyz))),
            0.0);
    
    // extract the first three components of the vertex for position
    gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
    
    // the final component is the size in world space
    gl_PointSize = pointScale * gl_Vertex.w / gl_Position.w;
}
