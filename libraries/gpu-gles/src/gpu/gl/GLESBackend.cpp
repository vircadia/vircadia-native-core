//
//  GLBackend.cpp
//  libraries/gpu-gl-android/src/gpu/gl
//
//  Created by Cristian Duarte & Gabriel Calero on 9/21/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include <gpu/gl/GLBackend.h>

#include <QtCore/QProcessEnvironment>


#include <shared/GlobalAppProperties.h>
#include <GPUIdent.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gpu/gl/GLShader.h>

#include "../gles/GLESBackend.h"

using namespace gpu;
using namespace gpu::gl;

static GLBackend* INSTANCE{ nullptr };

BackendPointer GLBackend::createBackend() {
    // FIXME provide a mechanism to override the backend for testing
    // Where the gpuContext is initialized and where the TRUE Backend is created and assigned
    //auto version = QOpenGLContextWrapper::currentContextVersion();
    std::shared_ptr<GLBackend> result;
    
    qDebug() << "Using OpenGL ES backend";
    result = std::make_shared<gpu::gles::GLESBackend>();

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
