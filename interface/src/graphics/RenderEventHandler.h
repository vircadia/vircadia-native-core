//
//  RenderEventHandler.h
//
//  Created by Bradley Austin Davis on 29/6/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RenderEventHandler_h
#define hifi_RenderEventHandler_h

#include "gl/OffscreenGLCanvas.h"
#include <QEvent.h>
#include <QElapsedTimer.h>

enum ApplicationEvent {
    // Execute a lambda function
    Lambda = QEvent::User + 1,
    // Trigger the next render
    Render,
    // Trigger the next idle
    Idle,
};

class RenderEventHandler : public QObject {
    using Parent = QObject;
    Q_OBJECT
public:

    using RenderCall = std::function <void()>;
    RenderCall _renderCall;

    RenderEventHandler(QOpenGLContext* context, RenderCall renderCall);

    QElapsedTimer _lastTimeRendered;
    std::atomic<bool> _pendingRenderEvent{ true };

    void resumeThread();

private:
    void initialize();

    void render();

    bool event(QEvent* event) override;

    OffscreenGLCanvas* _renderContext{ nullptr };
};

#endif // #include hifi_RenderEventHandler_h