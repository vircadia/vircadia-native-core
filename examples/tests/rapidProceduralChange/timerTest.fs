//
//  timerTest.fs 
//  examples/tests/rapidProceduralChange
//
//  Created by Eric Levin on 3/9/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This fragment shader is designed to test the rapid changing of a uniform on the timer.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


uniform float red;

vec3 getSkyboxColor() {
    float blue = red;
    blue = (cos(iGlobalTime) + 1) / 2;
    return vec3(1.0, 0.0, blue);
}

