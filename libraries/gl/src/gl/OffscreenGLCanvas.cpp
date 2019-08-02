//
//  OffscreenGLCanvas.cpp
//  interface/src/renderer
//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "OffscreenGLCanvas.h"

#include "Config.h"

#include <QtCore/QProcessEnvironment>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QThreadStorage>
#include <QtCore/QPointer>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

#include <DependencyManager.h>

#include "Context.h"
#include "GLHelpers.h"
#include "GLLogging.h"

OffscreenGLCanvas::OffscreenGLCanvas() :
    _context(new QOpenGLContext),
    _offscreenSurface(new QOffscreenSurface)
{
    setFormat(getDefaultOpenGLSurfaceFormat());
}

OffscreenGLCanvas::~OffscreenGLCanvas() {
    clearThreadContext();

    // A context with logging enabled needs to be current when it's destroyed
    _context->makeCurrent(_offscreenSurface);
    delete _context;
    _context = nullptr;

    _offscreenSurface->destroy();
    delete _offscreenSurface;
    _offscreenSurface = nullptr;

}

void OffscreenGLCanvas::setFormat(const QSurfaceFormat& format) {
    _context->setFormat(format);
}
    
bool OffscreenGLCanvas::create(QOpenGLContext* sharedContext) {
    if (nullptr != sharedContext) {
        sharedContext->doneCurrent();
        _context->setShareContext(sharedContext);
    }
    if (!_context->create()) {
        qFatal("Failed to create OffscreenGLCanvas context");
    }

    _offscreenSurface->setFormat(_context->format());
    _offscreenSurface->create();
    if (!_offscreenSurface->isValid()) {
        qFatal("Offscreen surface is invalid");
    }
    return true;
}

bool OffscreenGLCanvas::makeCurrent() {
    bool result = _context->makeCurrent(_offscreenSurface);
    if (result) {
        std::call_once(_reportOnce, [] {
            LOG_GL_CONTEXT_INFO(glLogging, gl::ContextInfo().init());
        });
    }

    return result;
}

void OffscreenGLCanvas::doneCurrent() {
    _context->doneCurrent();
}

QObject* OffscreenGLCanvas::getContextObject() {
    return _context;
}

void OffscreenGLCanvas::moveToThreadWithContext(QThread* thread) {
    clearThreadContext();
    moveToThread(thread);
    _context->moveToThread(thread);
}

struct ThreadContextStorage : public Dependency {
    QThreadStorage<QPointer<OffscreenGLCanvas>> threadContext;
};

void OffscreenGLCanvas::setThreadContext() {
    if (!DependencyManager::isSet<ThreadContextStorage>()) {
        DependencyManager::set<ThreadContextStorage>();
    }
    auto threadContextStorage = DependencyManager::get<ThreadContextStorage>();
    QPointer<OffscreenGLCanvas> p(this);
    threadContextStorage->threadContext.setLocalData(p);
}

void OffscreenGLCanvas::clearThreadContext() {
    if (!DependencyManager::isSet<ThreadContextStorage>()) {
        return;
    }
    auto threadContextStorage = DependencyManager::get<ThreadContextStorage>();
    if (!threadContextStorage->threadContext.hasLocalData()) {
        return;
    }
    auto& threadContext = threadContextStorage->threadContext.localData();
    if (this != threadContext.operator OffscreenGLCanvas *()) {
        return;
    }
    threadContextStorage->threadContext.setLocalData(nullptr);
}

bool OffscreenGLCanvas::restoreThreadContext() {
    // Restore the rendering context for this thread
    if (!DependencyManager::isSet<ThreadContextStorage>()) {
        return false;
    }

    auto threadContextStorage = DependencyManager::get<ThreadContextStorage>();
    if (!threadContextStorage->threadContext.hasLocalData()) {
        return false;
    }

    auto threadCanvas = threadContextStorage->threadContext.localData();
    if (!threadCanvas) {
        return false;
    }

    if (!threadCanvas->makeCurrent()) {
        qFatal("Unable to restore Offscreen rendering context");
    }

    return true;
}
