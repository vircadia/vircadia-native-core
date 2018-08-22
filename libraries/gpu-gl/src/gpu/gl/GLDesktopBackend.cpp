//
//  GLBackend.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include <gpu/gl/GLBackend.h>

#include <functional>

#include <QtCore/QProcessEnvironment>

#include <shared/GlobalAppProperties.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLHelpers.h>
#include <gpu/gl/GLShader.h>

#include "../gl41/GL41Backend.h"
#include "../gl45/GL45Backend.h"

using namespace gpu;
using namespace gpu::gl;

static GLBackend* INSTANCE{ nullptr };

BackendPointer GLBackend::createBackend() {
    // FIXME provide a mechanism to override the backend for testing
    // Where the gpuContext is initialized and where the TRUE Backend is created and assigned
    auto version = QOpenGLContextWrapper::currentContextVersion();
    std::shared_ptr<GLBackend> result;
    if (!::gl::disableGl45() && version >= 0x0405) {
        qCDebug(gpugllogging) << "Using OpenGL 4.5 backend";
        result = std::make_shared<gpu::gl45::GL45Backend>();
    } else {
        qCDebug(gpugllogging) << "Using OpenGL 4.1 backend";
        result = std::make_shared<gpu::gl41::GL41Backend>();
    }
    result->initInput();
    result->initTransform();
    result->initTextureManagementStage();

    INSTANCE = result.get();
    void* voidInstance = &(*result);
    qApp->setProperty(hifi::properties::gl::BACKEND, QVariant::fromValue(voidInstance));
    return result;
}

GLBackend& getBackend() {
    if (!INSTANCE) {
        INSTANCE = static_cast<GLBackend*>(qApp->property(hifi::properties::gl::BACKEND).value<void*>());
    }
    return *INSTANCE;
}
