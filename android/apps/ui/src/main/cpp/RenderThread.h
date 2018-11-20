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
#include <qml/OffscreenSurface.h>

class RenderThread : public GenericThread {
    using Parent = GenericThread;
    using OffscreenSurface = hifi::qml::OffscreenSurface;
    using OffscreenPtr = std::shared_ptr<OffscreenSurface>;
    using DiscardLambda = std::function<void(uint32_t, void*)>;
public:
    gl::Context _glContext;
    std::mutex _mutex;
    std::atomic<size_t> _presentCount{ 0 };
    std::mutex _frameLock;
    OffscreenPtr _offscreen;
    QSize _size;
    DiscardLambda _discardLambda { OffscreenSurface::getDiscardLambda() };
    GLuint _readFbo { 0 };
    GLuint _uiTexture { 0 };

    void setup() override;
    bool process() override;
    void shutdown() override;
    void initialize(QWindow* window);

private:
    void releaseTexture();
};
