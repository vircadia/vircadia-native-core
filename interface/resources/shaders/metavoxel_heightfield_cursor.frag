#version 120

//
//  metavoxel_heightfield_cursor.frag
//  fragment shader
//
//  Created by Andrzej Kapolka on 8/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// the cursor texture
uniform sampler2D cursorMap;

void main(void) {
    // just multiply the color by the cursor texture
    gl_FragColor = gl_Color * texture2D(cursorMap, gl_TexCoord[0].st);
}
