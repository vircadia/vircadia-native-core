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

class OffscreenQmlRenderThread;


class WebEntityAPIHelper : public QObject {
    Q_OBJECT
public:
    void setRenderableWebEntityItem(RenderableWebEntityItem* renderableWebEntityItem) {
        _renderableWebEntityItem = renderableWebEntityItem;
    }
    Q_INVOKABLE void synthesizeKeyPress(QString key);

    // event bridge
public slots:
    void emitScriptEvent(const QVariant& scriptMessage);
    void emitWebEvent(const QVariant& webMessage);
signals:
    void scriptEventReceived(const QVariant& message);
    void webEventReceived(const QVariant& message);

protected:
    RenderableWebEntityItem* _renderableWebEntityItem{ nullptr };
};


class OffscreenQmlSurface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool focusText READ isFocusText NOTIFY focusTextChanged)
public:
    OffscreenQmlSurface();
    virtual ~OffscreenQmlSurface();

    using MouseTranslator = std::function<QPoint(const QPointF&)>;

    virtual void create(QOpenGLContext* context);
    void resize(const QSize& size, bool forceResize = false);
    QSize size() const;
    Q_INVOKABLE QObject* load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {});
    Q_INVOKABLE QObject* load(const QString& qmlSourceFile, std::function<void(QQmlContext*, QObject*)> f = [](QQmlContext*, QObject*) {}) {
        return load(QUrl(qmlSourceFile), f);
    }

    Q_INVOKABLE void executeOnUiThread(std::function<void()> function, bool blocking = false);
    Q_INVOKABLE QVariant returnFromUiThread(std::function<QVariant()> function);

    void setMaxFps(uint8_t maxFps) { _maxFps = maxFps; }
    // Optional values for event handling
    void setProxyWindow(QWindow* window);
    void setMouseTranslator(MouseTranslator mouseTranslator) {
        _mouseTranslator = mouseTranslator;
    }

    bool isFocusText() const { return _focusText; }
    void pause();
    void resume();
    bool isPaused() const;

    void setBaseUrl(const QUrl& baseUrl);
    QQuickItem* getRootItem();
    QQuickWindow* getWindow();
    QObject* getEventHandler();
    QQmlContext* getRootContext();

    QPointF mapToVirtualScreen(const QPointF& originalPoint, QObject* originalWidget);
    bool eventFilter(QObject* originalDestination, QEvent* event) override;

    // XXX make private
    WebEntityAPIHelper* _webEntityAPIHelper;

signals:
    void textureUpdated(unsigned int texture);
    void focusObjectChanged(QObject* newFocus);
    void focusTextChanged(bool focusText);

public slots:
    void requestUpdate();
    void requestRender();
    void onAboutToQuit();

protected:
    bool filterEnabled(QObject* originalDestination, QEvent* event) const;
    void setFocusText(bool newFocusText);

private:
    QObject* finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject);

private slots:
    void updateQuick();
    void onFocusObjectChanged(QObject* newFocus);

private:
    friend class OffscreenQmlRenderThread;
    OffscreenQmlRenderThread* _renderer{ nullptr };
    QQmlEngine* _qmlEngine{ nullptr };
    QQmlComponent* _qmlComponent{ nullptr };
    QQuickItem* _rootItem{ nullptr };
    QTimer _updateTimer;
    uint32_t _currentTexture{ 0 };
    bool _render{ false };
    bool _polish{ true };
    bool _paused{ true };
    bool _focusText { false };
    uint8_t _maxFps{ 60 };
    MouseTranslator _mouseTranslator{ [](const QPointF& p) { return p.toPoint();  } };
    QWindow* _proxyWindow { nullptr };
};

#endif
