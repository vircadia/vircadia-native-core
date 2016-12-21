//
//  OffscreenGlCanvas.cpp
//  interface/src/renderer
//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "FboCache.h"

#include <QOpenGLFramebufferObject>
#include <QDebug>
#include "ThreadHelpers.h"
#include "RenderUtilsLogging.h"

FboCache::FboCache() {
    // Why do we even HAVE that lever?
}

void FboCache::lockTexture(int texture) {
    withLock(_lock, [&] {
        Q_ASSERT(_fboMap.count(texture));
        if (!_fboLocks.count(texture)) {
            Q_ASSERT(_readyFboQueue.front()->texture() == (GLuint)texture);
            _readyFboQueue.pop_front();
            _fboLocks[texture] = 1;
        } else {
            _fboLocks[texture]++;
        }
    });
}

void FboCache::releaseTexture(int texture) {
    withLock(_lock, [&] {
        Q_ASSERT(_fboMap.count(texture));
        Q_ASSERT(_fboLocks.count(texture));
        int newLockCount = --_fboLocks[texture];
        if (!newLockCount) {
            auto fbo = _fboMap[texture].data();
            if (fbo->size() != _size) {
                // Move the old FBO to the destruction queue.
                // We can't destroy the FBO here because we might 
                // not be on the right thread or have the context active
                _destroyFboQueue.push_back(_fboMap[texture]);
                _fboMap.remove(texture);
            } else {
                _readyFboQueue.push_back(fbo);
            }
            _fboLocks.remove(texture);
        }
    });
}

QOpenGLFramebufferObject* FboCache::getReadyFbo() {
    QOpenGLFramebufferObject* result = nullptr;
    withLock(_lock, [&] {
        // Delete any FBOs queued for deletion
        _destroyFboQueue.clear();

        if (_readyFboQueue.empty()) {
            qCDebug(renderutils) << "Building new offscreen FBO number " << _fboMap.size() + 1;
            result = new QOpenGLFramebufferObject(_size, QOpenGLFramebufferObject::CombinedDepthStencil);
            _fboMap[result->texture()] = QSharedPointer<QOpenGLFramebufferObject>(result);
            _readyFboQueue.push_back(result);
        } else {
            result = _readyFboQueue.front();
        }
    });
    return result;
}

void FboCache::setSize(const QSize& newSize) {
    if (_size == newSize) {
        return;
    }
    _size = newSize;
    withLock(_lock, [&] {
        // Clear out any fbos with the old id
        _readyFboQueue.clear();
 
        QSet<int> outdatedFbos;
        // FBOs that are locked will be removed as they are unlocked
        foreach(int texture, _fboMap.keys()) {
            if (!_fboLocks.count(texture)) {
                outdatedFbos.insert(texture);
            }
        }
        // Implicitly deletes the FBO via the shared pointer destruction mechanism
        foreach(int texture, outdatedFbos) {
            _fboMap.remove(texture);
        }
    });
}

const QSize& FboCache::getSize() {
    return _size;
}

