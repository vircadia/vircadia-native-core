#version 120

//
//  ambient_occlusion.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 7/5/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

// the depth texture
uniform sampler2D depth;

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
    float z = (far * near) / (texture2D(depth, texCoord).r * (far - near) - far);
    return vec3(((texCoord * 2.0 - vec2(1.0, 1.0)) * (rightTop - leftBottom) + rightTop + leftBottom) * z / (-2.0 * near), z);
}

void main(void) {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
