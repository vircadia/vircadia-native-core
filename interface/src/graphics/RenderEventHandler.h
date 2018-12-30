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

#include <functional>
#include <QEvent>
#include <QElapsedTimer>
#include "gl/OffscreenGLCanvas.h"

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

    using CheckCall = std::function <bool()>;
    using RenderCall = std::function <void()>;

    CheckCall _checkCall;
    RenderCall _renderCall;

    RenderEventHandler(CheckCall checkCall, RenderCall renderCall);

    QElapsedTimer _lastTimeRendered;
    std::atomic<bool> _pendingRenderEvent{ true };

    void resumeThread();

private:
    void initialize();

    void render();

    bool event(QEvent* event) override;
};

#endif // #include hifi_RenderEventHandler_h
