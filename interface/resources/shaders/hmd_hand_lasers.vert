//
//  Created by Bradley Austin Davis on 2016/07/11
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#version 410 core
uniform mat4 mvp = mat4(1);

in vec3 Position;

void main() {
  gl_Position = mvp * vec4(Position, 1);
}
