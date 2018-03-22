//
//  Created by Sam Gateau on 2016/07/21
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLInputFormat.h"
#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;


GLInputFormat::GLInputFormat() {
}

GLInputFormat:: ~GLInputFormat() {

}

GLInputFormat* GLInputFormat::sync(const Stream::Format& inputFormat) {
    GLInputFormat* object = Backend::getGPUObject<GLInputFormat>(inputFormat);

    if (!object) {
        object = new GLInputFormat();
        object->key = inputFormat.getKey();
        Backend::setGPUObject(inputFormat, object);
    }

    return object;
}
