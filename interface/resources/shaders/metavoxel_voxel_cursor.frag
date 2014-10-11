#version 120

//
//  metavoxel_voxel_cursor.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 10/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the inner radius of the outline, squared
const float SQUARED_OUTLINE_INNER_RADIUS = 0.81;

// the outer radius of the outline, squared
const float SQUARED_OUTLINE_OUTER_RADIUS = 1.0;

// the inner radius of the inset, squared
const float SQUARED_INSET_INNER_RADIUS = 0.855625;

// the outer radius of the inset, squared
const float SQUARED_INSET_OUTER_RADIUS = 0.950625;

void main(void) {
    // use the distance to compute the ring color, then multiply it by the varying color
    float squaredDistance = dot(gl_TexCoord[0].str, gl_TexCoord[0].str);
    float alpha = step(SQUARED_OUTLINE_INNER_RADIUS, squaredDistance) * step(squaredDistance, SQUARED_OUTLINE_OUTER_RADIUS);
    float white = step(SQUARED_INSET_INNER_RADIUS, squaredDistance) * step(squaredDistance, SQUARED_INSET_OUTER_RADIUS);
    gl_FragColor = gl_Color * vec4(white, white, white, alpha);
}
