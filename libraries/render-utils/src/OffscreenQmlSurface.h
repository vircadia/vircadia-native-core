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

#include "OffscreenGlCanvas.h"

class QWindow;
class QMyQuickRenderControl;
class QQmlEngine;
class QQmlContext;
class QQmlComponent;
class QQuickWindow;
class QQuickItem;
class FboCache;

class OffscreenQmlSurface : public OffscreenGlCanvas {
    Q_OBJECT

public:
    OffscreenQmlSurface();
    virtual ~OffscreenQmlSurface();

    using MouseTranslator = std::function<QPointF(const QPointF&)>;

    void create(QOpenGLContext* context);
    void resize(const QSize& size);
    QObject* load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    QObject* load(const QString& qmlSourceFile, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}) {
        return load(QUrl(qmlSourceFile), f);
    }

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

    virtual bool eventFilter(QObject* originalDestination, QEvent* event);

signals:
    void textureUpdated(unsigned int texture);

public slots:
    void requestUpdate();
    void requestRender();
    void lockTexture(int texture);
    void releaseTexture(int texture);

private:
    QObject* finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject);

private slots:
    void updateQuick();

protected:
    QQuickWindow* _quickWindow{ nullptr };

private:
    QMyQuickRenderControl* _renderControl{ nullptr };
    QQmlEngine* _qmlEngine{ nullptr };
    QQmlComponent* _qmlComponent{ nullptr };
    QQuickItem* _rootItem{ nullptr };
    QTimer _updateTimer;
    FboCache* _fboCache;
    bool _polish{ true };
    bool _paused{ true };
    MouseTranslator _mouseTranslator{ [](const QPointF& p) { return p;  } };
};

#endif
