//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <GenericThread.h>

#include <gl/Context.h>
#include <gpu/gl/GLBackend.h>
#include <qml/OffscreenSurface.h>

class RenderThread : public GenericThread {
    using Parent = GenericThread;
public:
    QWindow* _window{ nullptr };
    std::mutex _mutex;
    gpu::ContextPointer _gpuContext;  // initialized during window creation
    std::shared_ptr<gpu::Backend> _backend;
    std::atomic<size_t> _presentCount{ 0 };
    std::mutex _frameLock;
    std::queue<gpu::FramePointer> _pendingFrames;
    gpu::FramePointer _activeFrame;
    uint32_t _externalTexture{ 0 };
    glm::mat4 _correction;
    hifi::qml::OffscreenSurface* _offscreen{ nullptr };

    gl::Context _glContext;
    uint32_t _uiTexture{ 0 };
    uint32_t _uiFbo{ 0 };

    void move(const glm::vec3& v);
    void setup() override;
    bool process() override;
    void shutdown() override;

    void initialize(QWindow* window, hifi::qml::OffscreenSurface* offscreen);

    void submitFrame(const gpu::FramePointer& frame);
    void updateFrame();
    void renderFrame();

    bool makeCurrent() {
        return _glContext.makeCurrent();
    }

    void doneCurrent() {
        _glContext.doneCurrent();
    }
};
