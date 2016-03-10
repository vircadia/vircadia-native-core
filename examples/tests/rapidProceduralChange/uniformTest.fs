//
//  uniformTest.fs
//  examples/tests/rapidProceduralChange
//
//  Created by Eric Levin on 3/9/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This fragment shader is designed to test the rapid changing of a uniform.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


uniform float red;

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    fragColor = vec4(red, 0.0, 1.0, 1.0);
}

vec4 getProceduralColor() {
    vec4 result;
    vec2 position = _position.xz;
    position += 0.5;
    mainImage(result, position * iWorldScale.xz);
    return result;
}

