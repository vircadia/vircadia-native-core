//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QtCore/QElapsedTimer>

#include <GenericThread.h>
#include <shared/RateCounter.h>
#include <gl/Config.h>
#include <gl/Context.h>
#include <gpu/gl/GLBackend.h>
#include <ovr/VrHandler.h>

class RenderThread : public GenericThread, ovr::VrHandler {
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

    void move(const glm::vec3& v);
    void setup() override;
    bool process() override;
    void shutdown() override;

    void handleInput();

    void submitFrame(const gpu::FramePointer& frame);
    void initialize(QWindow* window);
    void renderFrame();
};
