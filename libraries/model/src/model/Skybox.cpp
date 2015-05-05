//
//  Skybox.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 5/4/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Skybox.h"

using namespace model;

Skybox::Skybox() {
}

void Skybox::setCubemap(const gpu::TexturePointer& cubemap) {
    _cubemap = cubemap;
}

