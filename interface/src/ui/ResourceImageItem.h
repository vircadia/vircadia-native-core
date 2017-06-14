//
// ResourceImageItem.h
//
// Created by David Kelly and Howard Stearns on 2017/06/08
// Copyright 2017 High Fidelity, Inc.

// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_ResourceImageItem_h
#define hifi_ResourceImageItem_h

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>

class ResourceImageItemRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size);
    void render();
};

class ResourceImageItem : public QQuickFramebufferObject {
    QQuickFramebufferObject::Renderer* createRenderer() const { return new ResourceImageItemRenderer; }
};

#endif // hifi_ResourceImageItem_h
