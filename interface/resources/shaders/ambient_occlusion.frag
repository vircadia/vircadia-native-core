#version 120

//
//  ambient_occlusion.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 7/5/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the depth texture
uniform sampler2D depthTexture;

// the distance to the near clip plane
uniform float near;

// the distance to the far clip plane
uniform float far;

// the left and bottom edges of the view window
uniform vec2 leftBottom;

// the right and top edges of the view window
uniform vec2 rightTop;

// given a texture coordinate, returns the 3D view space coordinate
vec3 texCoordToViewSpace(vec2 texCoord) {
    float z = (far * near) / (texture2D(depthTexture, texCoord).r * (far - near) - far);
    return vec3(((texCoord * 2.0 - vec2(1.0, 1.0)) * (rightTop - leftBottom) + rightTop + leftBottom) * z / (-2.0 * near), z);
}

void main(void) {
    float ds = dFdx(gl_TexCoord[0].s);
    float dt = dFdy(gl_TexCoord[0].t);
    vec3 center = texCoordToViewSpace(gl_TexCoord[0].st);
    vec3 left = texCoordToViewSpace(gl_TexCoord[0].st - vec2(-ds, 0.0)) - center;
    vec3 right = texCoordToViewSpace(gl_TexCoord[0].st - vec2(ds, 0.0)) - center;
    vec3 up = texCoordToViewSpace(gl_TexCoord[0].st - vec2(0.0, dt)) - center;
    vec3 down = texCoordToViewSpace(gl_TexCoord[0].st - vec2(0.0, -dt)) - center;
    float occlusion = 0.5 - (left.z / length(left) + right.z / length(right) + up.z / length(up) + down.z / length(down)) / 8.0;
    gl_FragColor = vec4(occlusion, occlusion, occlusion, 0.0);
}
