#version 120

//
//  metavoxel_heightfield_light_shadow_map.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the shadow texture
uniform sampler2DShadow shadowMap;

// the inverse of the size of the shadow map
const float shadowScale = 1.0 / 2048.0;

// the interpolated normal
varying vec4 normal;

void main(void) {
    // compute the base color based on OpenGL lighting model
    float diffuse = dot(normalize(normal), gl_LightSource[0].position);
    float facingLight = step(0.0, diffuse) * 0.25 *
        (shadow2D(shadowMap, gl_TexCoord[1].stp + vec3(-shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[1].stp + vec3(-shadowScale, shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[1].stp + vec3(shadowScale, -shadowScale, 0.0)).r +
        shadow2D(shadowMap, gl_TexCoord[1].stp + vec3(shadowScale, shadowScale, 0.0)).r);
    gl_FragColor = gl_Color * (gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient +
        gl_FrontLightProduct[0].diffuse * (diffuse * facingLight));
}
