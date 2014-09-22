#version 120

//
//  deferred_light_limited.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 9/19/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the radius (hard cutoff) of the light effect
uniform float radius;

void main(void) {
    // find the right "right" direction
    vec3 firstRightCandidate = cross(gl_LightSource[1].spotDirection, vec3(0.0, 1.0, 0.0));
    vec3 secondRightCandidate = cross(gl_LightSource[1].spotDirection, vec3(1.0, 0.0, 0.0));
    vec3 right = mix(firstRightCandidate, secondRightCandidate, step(length(firstRightCandidate), length(secondRightCandidate)));
    right = normalize(right);
    
    // and the "up"
    vec3 up = cross(right, gl_LightSource[1].spotDirection);
    
    // and the "back," which depends on whether this is a spot light
    vec3 back = -gl_LightSource[1].spotDirection * step(gl_LightSource[1].spotCosCutoff, 0.0);
    
    // find the eight corners of the bounds
    vec4 c0 = gl_ProjectionMatrix * vec4(gl_LightSource[1].position.xyz +
        radius * (-up - right + back), 1.0);
    vec4 c1 = gl_ProjectionMatrix * vec4(gl_LightSource[1].position.xyz +
        radius * (-up + right + back), 1.0);
    vec4 c2 = gl_ProjectionMatrix * vec4(gl_LightSource[1].position.xyz +
        radius * (up - right + back), 1.0);
    vec4 c3 = gl_ProjectionMatrix * vec4(gl_LightSource[1].position.xyz +
        radius * (up + right + back), 1.0);
    vec4 c4 = gl_ProjectionMatrix * vec4(gl_LightSource[1].position.xyz +
        radius * (-up - right + gl_LightSource[1].spotDirection), 1.0);
    vec4 c5 = gl_ProjectionMatrix * vec4(gl_LightSource[1].position.xyz +
        radius * (-up + right + gl_LightSource[1].spotDirection), 1.0);
    vec4 c6 = gl_ProjectionMatrix * vec4(gl_LightSource[1].position.xyz +
        radius * (up - right + gl_LightSource[1].spotDirection), 1.0);
    vec4 c7 = gl_ProjectionMatrix * vec4(gl_LightSource[1].position.xyz +
        radius * (up + right + gl_LightSource[1].spotDirection), 1.0);
    
    // find their projected extents
    vec2 extents = max(
        max(max(gl_Vertex.xy * (c0.xy / max(c0.w, 0.001)), gl_Vertex.xy * (c1.xy / max(c1.w, 0.001))),
            max(gl_Vertex.xy * (c2.xy / max(c2.w, 0.001)), gl_Vertex.xy * (c3.xy / max(c3.w, 0.001)))),
        max(max(gl_Vertex.xy * (c4.xy / max(c4.w, 0.001)), gl_Vertex.xy * (c5.xy / max(c5.w, 0.001))),
            max(gl_Vertex.xy * (c6.xy / max(c6.w, 0.001)), gl_Vertex.xy * (c7.xy / max(c7.w, 0.001)))));
    
    // make sure they don't extend beyond the screen
    extents = min(extents, vec2(1.0, 1.0));
    
    gl_Position = vec4(gl_Vertex.xy * extents, 0.0, 1.0);
    gl_TexCoord[0] = vec4(dot(gl_Position, gl_ObjectPlaneS[3]), dot(gl_Position, gl_ObjectPlaneT[3]), 0.0, 1.0);
}
