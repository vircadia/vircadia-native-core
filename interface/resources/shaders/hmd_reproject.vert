//
//  Created by Bradley Austin Davis on 2016/07/11
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#version 410 core
in vec3 Position;
in vec2 TexCoord;

out vec3 vPosition;
out vec2 vTexCoord;

void main() {
  gl_Position = vec4(Position, 1);
  vTexCoord = TexCoord;
  vPosition = Position;
}
