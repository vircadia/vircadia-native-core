//
//  RenderEventHandler.cpp
//
//  Created by Bradley Austin Davis on 29/6/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RenderEventHandler.h"

#include "Application.h"
#include <shared/GlobalAppProperties.h>
#include <shared/QtHelpers.h>

#include "CrashHandler.h"

RenderEventHandler::RenderEventHandler(QOpenGLContext* context) {
    _renderContext = new OffscreenGLCanvas();
    _renderContext->setObjectName("RenderContext");
    _renderContext->create(context);
    if (!_renderContext->makeCurrent()) {
        qFatal("Unable to make rendering context current");
    }
    _renderContext->doneCurrent();

    // Deleting the object with automatically shutdown the thread
    connect(qApp, &QCoreApplication::aboutToQuit, this, &QObject::deleteLater);

    // Transfer to a new thread
    moveToNewNamedThread(this, "RenderThread", [this](QThread* renderThread) {
        hifi::qt::addBlockingForbiddenThread("Render", renderThread);
        _renderContext->moveToThreadWithContext(renderThread);
        _lastTimeRendered.start();
    }, std::bind(&RenderEventHandler::initialize, this), QThread::HighestPriority);
}

void RenderEventHandler::initialize() {
    setObjectName("Render");
    PROFILE_SET_THREAD_NAME("Render");
    setCrashAnnotation("render_thread_id", std::to_string((size_t)QThread::currentThreadId()));
    if (!_renderContext->makeCurrent()) {
        qFatal("Unable to make rendering context current on render thread");
    }
 
}

void RenderEventHandler::resumeThread() {
    _pendingRenderEvent = false;
}

void RenderEventHandler::render() {
    if (qApp->shouldPaint()) {
        _lastTimeRendered.start();
        qApp->paintGL();
    }
}

bool RenderEventHandler::event(QEvent* event) {
    switch ((int)event->type()) {
    case ApplicationEvent::Render:
        render();
        _pendingRenderEvent.store(false);
        return true;

    default:
        break;
    }
    return Parent::event(event);
}

