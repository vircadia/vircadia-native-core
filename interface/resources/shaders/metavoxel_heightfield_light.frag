#version 120

//
//  metavoxel_heightfield_light.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the interpolated normal
varying vec4 normal;

void main(void) {
    // compute the base color based on OpenGL lighting model
    gl_FragColor = gl_Color * (gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient +
        gl_FrontLightProduct[0].diffuse * max(0.0, dot(normalize(normal), gl_LightSource[0].position)));
}
