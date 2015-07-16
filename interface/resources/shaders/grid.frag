#version 120

//
//  grid.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Update by Ryan Huffman on 12/30/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

varying vec2 modelCoords;

// Squared distance from center of 1x1 square to an edge along a single axis (0.5 * 0.5)
const float SQUARED_DISTANCE_TO_EDGE = 0.25;

void main(void) {
    // Squared distance from grid center - this assumes the grid lines are from
    // 0.0 ... 1.0 in model space.
    float sqDist = pow(modelCoords.x - 0.5, 2) + pow(modelCoords.y - 0.5, 2);
    float alpha = max(0, SQUARED_DISTANCE_TO_EDGE - sqDist) / SQUARED_DISTANCE_TO_EDGE;
    alpha *= gl_Color.a;
    gl_FragColor = vec4(gl_Color.rgb, alpha);
}
