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

    // pass along the vertex color
    gl_FrontColor = gl_Color;
    
    // extract the first four components of the vertex for position
    gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
    
    gl_PointSize = pointScale * gl_Vertex.w / gl_Position.w;
}
