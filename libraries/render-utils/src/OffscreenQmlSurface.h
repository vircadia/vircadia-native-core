//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_OffscreenQmlSurface_h
#define hifi_OffscreenQmlSurface_h

#include <QTimer>
#include <QUrl>
#include <atomic>
#include <functional>

#include <GLMHelpers.h>
#include <ThreadHelpers.h>

class QWindow;
class QMyQuickRenderControl;
class QOpenGLContext;
class QQmlEngine;
class QQmlContext;
class QQmlComponent;
class QQuickWindow;
class QQuickItem;

class OffscreenQmlRenderer;

class OffscreenQmlSurface : public QObject {
    Q_OBJECT

public:
    OffscreenQmlSurface();
    virtual ~OffscreenQmlSurface();

    using MouseTranslator = std::function<QPointF(const QPointF&)>;

    void create(QOpenGLContext* context);
    void resize(const QSize& size);
    QSize size() const;
    QObject* load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    QObject* load(const QString& qmlSourceFile, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}) {
        return load(QUrl(qmlSourceFile), f);
    }

    void setMaxFps(uint8_t maxFps) { _maxFps = maxFps; }
    // Optional values for event handling
    void setProxyWindow(QWindow* window);
    void setMouseTranslator(MouseTranslator mouseTranslator) {
        _mouseTranslator = mouseTranslator;
    }

    void pause();
    void resume();
    bool isPaused() const;

    void setBaseUrl(const QUrl& baseUrl);
    QQuickItem* getRootItem();
    QQuickWindow* getWindow();
    QObject* getEventHandler();

    QPointF mapToVirtualScreen(const QPointF& originalPoint, QObject* originalWidget);
    virtual bool eventFilter(QObject* originalDestination, QEvent* event);

signals:
    void textureUpdated(unsigned int texture);

public slots:
    void requestUpdate();
    void requestRender();

private:
    QObject* finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject);

private slots:
    void updateQuick();

private:
    friend class OffscreenQmlRenderer;
    OffscreenQmlRenderer* _renderer{ nullptr };
    QQmlEngine* _qmlEngine{ nullptr };
    QQmlComponent* _qmlComponent{ nullptr };
    QQuickItem* _rootItem{ nullptr };
    QTimer _updateTimer;
    uint32_t _currentTexture{ 0 };
    bool _render{ false };
    bool _polish{ true };
    bool _paused{ true };
    uint8_t _maxFps{ 60 };
    MouseTranslator _mouseTranslator{ [](const QPointF& p) { return p;  } };

};

#endif
