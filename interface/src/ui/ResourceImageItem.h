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

#include "Application.h"

#include <QQuickFramebufferObject>
#include <QQuickWindow>
#include <QTimer>

#include <TextureCache.h>

class ResourceImageItemRenderer : public QObject, public QQuickFramebufferObject::Renderer {
    Q_OBJECT
public:
    ResourceImageItemRenderer();
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;
    void synchronize(QQuickFramebufferObject* item) override;
    void render() override;
private:
    bool _ready;
    QString _url;
    bool _visible;

    NetworkTexturePointer _networkTexture;
    QQuickWindow* _window;
    QMutex _fboMutex;
    QOpenGLFramebufferObject* _copyFbo { nullptr };
    GLsync _fenceSync { 0 };
    QTimer _updateTimer;
public slots:
    void onUpdateTimer();
};

class ResourceImageItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(QString url READ getUrl WRITE setUrl)
    Q_PROPERTY(bool ready READ getReady WRITE setReady)
public:
    ResourceImageItem();
    QString getUrl() const { return m_url; }
    void setUrl(const QString& url);
    bool getReady() const { return m_ready; }
    void setReady(bool ready);
    QQuickFramebufferObject::Renderer* createRenderer() const override { return new ResourceImageItemRenderer; }

private:
    QString m_url;
    bool m_ready { false };

};

#endif // hifi_ResourceImageItem_h
