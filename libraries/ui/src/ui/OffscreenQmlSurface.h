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

#include <atomic>
#include <queue>
#include <map>
#include <functional>

#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include <GLMHelpers.h>
#include <ThreadHelpers.h>

#include <QTouchEvent>
#include "PointerEvent.h"

class QWindow;
class QMyQuickRenderControl;
class OffscreenGLCanvas;
class QOpenGLContext;
class QQmlEngine;
class QQmlContext;
class QQmlComponent;
class QQuickWindow;
class QQuickItem;
class QJSValue;

// GPU resources are typically buffered for one copy being used by the renderer, 
// one copy in flight, and one copy being used by the receiver
#define GPU_RESOURCE_BUFFER_SIZE 3

using QmlContextCallback = std::function<void(QQmlContext*)>;
using QmlContextObjectCallback = std::function<void(QQmlContext*, QQuickItem*)>;

class OffscreenQmlSurface : public QObject, public QEnableSharedFromThis<OffscreenQmlSurface> {
    Q_OBJECT
    Q_PROPERTY(bool focusText READ isFocusText NOTIFY focusTextChanged)
public:
    static void setSharedContext(QOpenGLContext* context);

    static QmlContextObjectCallback DEFAULT_CONTEXT_CALLBACK;
    static void addWhitelistContextHandler(const std::initializer_list<QUrl>& urls, const QmlContextCallback& callback);
    static void addWhitelistContextHandler(const QUrl& url, const QmlContextCallback& callback) { addWhitelistContextHandler({ { url } }, callback); };

    OffscreenQmlSurface();
    virtual ~OffscreenQmlSurface();

    using MouseTranslator = std::function<QPoint(const QPointF&)>;

    virtual void create();
    void resize(const QSize& size, bool forceResize = false);
    QSize size() const;

    // Usable from QML code as QmlSurface.load(url, parent, function(newItem){ ... })
    Q_INVOKABLE void load(const QUrl& qmlSource, QQuickItem* parent, const QJSValue& callback);

    // For C++ use 
    Q_INVOKABLE void load(const QUrl& qmlSource, bool createNewContext, const QmlContextObjectCallback& onQmlLoadedCallback = DEFAULT_CONTEXT_CALLBACK);
    Q_INVOKABLE void loadInNewContext(const QUrl& qmlSource, const QmlContextObjectCallback& onQmlLoadedCallback = DEFAULT_CONTEXT_CALLBACK);
    Q_INVOKABLE void load(const QUrl& qmlSource, const QmlContextObjectCallback& onQmlLoadedCallback = DEFAULT_CONTEXT_CALLBACK);
    Q_INVOKABLE void load(const QString& qmlSourceFile, const QmlContextObjectCallback& onQmlLoadedCallback = DEFAULT_CONTEXT_CALLBACK);

    void clearCache();
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
    bool getCleaned() { return _isCleaned; }

    QQuickItem* getRootItem();
    QQuickWindow* getWindow();
    QObject* getEventHandler();
    QQmlContext* getSurfaceContext();

    QPointF mapToVirtualScreen(const QPointF& originalPoint);
    bool eventFilter(QObject* originalDestination, QEvent* event) override;

    void setKeyboardRaised(QObject* object, bool raised, bool numeric = false, bool passwordField = false);
    Q_INVOKABLE void synthesizeKeyPress(QString key, QObject* targetOverride = nullptr);
    Q_INVOKABLE void lowerKeyboard();

    using TextureAndFence = std::pair<uint32_t, void*>;
    // Checks to see if a new texture is available.  If one is, the function returns true and 
    // textureAndFence will be populated with the texture ID and a fence which will be signalled 
    // when the texture is safe to read.
    // Returns false if no new texture is available
    bool fetchTexture(TextureAndFence& textureAndFence);

    static std::function<void(uint32_t, void*)> getDiscardLambda();
    static size_t getUsedTextureMemory();

    PointerEvent::EventType choosePointerEventType(QEvent::Type type);

    unsigned int deviceIdByTouchPoint(qreal x, qreal y);

signals:
    void focusObjectChanged(QObject* newFocus);
    void focusTextChanged(bool focusText);

public slots:
    void onAboutToQuit();
    void focusDestroyed(QObject *obj);

    // audio output device
public slots:
    void changeAudioOutputDevice(const QString& deviceName, bool isHtmlUpdate = false);
    void forceHtmlAudioOutputDeviceUpdate();
    void forceQmlAudioOutputDeviceUpdate();

signals:
    void audioOutputDeviceChanged(const QString& deviceName);

    // event bridge
public slots:
    void emitScriptEvent(const QVariant& scriptMessage);
    void emitWebEvent(const QVariant& webMessage);
signals:
    void scriptEventReceived(const QVariant& message);
    void webEventReceived(const QVariant& message);

    // qml event bridge
public slots:
    void sendToQml(const QVariant& message);
signals:
    void fromQml(QVariant message);

protected:
    bool filterEnabled(QObject* originalDestination, QEvent* event) const;
    void setFocusText(bool newFocusText);

private:
    static QOpenGLContext* getSharedContext();

    QQmlContext* contextForUrl(const QUrl& url, QQuickItem* parent, bool forceNewContext = false);
    void loadInternal(const QUrl& qmlSource, bool createNewContext, QQuickItem* parent, const QmlContextObjectCallback& onQmlLoadedCallback);
    void finishQmlLoad(QQmlComponent* qmlComponent, QQmlContext* qmlContext, QQuickItem* parent, const QmlContextObjectCallback& onQmlLoadedCallback);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject);
    bool allowNewFrame(uint8_t fps);
    void render();
    void cleanup();
    void disconnectAudioOutputTimer();

private slots:
    void updateQuick();
    void onFocusObjectChanged(QObject* newFocus);

public slots:
    void hoverBeginEvent(const PointerEvent& event, class QTouchDevice& device);
    void hoverEndEvent(const PointerEvent& event, class QTouchDevice& device);
    bool handlePointerEvent(const PointerEvent& event, class QTouchDevice& device, bool release = false);

private:
    QQuickWindow* _quickWindow { nullptr };
    QMyQuickRenderControl* _renderControl{ nullptr };
    QQmlContext* _qmlContext { nullptr };
    QQuickItem* _rootItem { nullptr };
    OffscreenGLCanvas* _canvas { nullptr };

    QTimer _updateTimer;
    uint32_t _fbo { 0 };
    uint32_t _depthStencil { 0 };
    uint64_t _lastRenderTime { 0 };
    uvec2 _size;

    QTimer _audioOutputUpdateTimer;
    QString _currentAudioOutputDevice;

    // Texture management
    TextureAndFence _latestTextureAndFence { 0, 0 };

    bool _render { false };
    bool _polish { true };
    bool _paused { true };
    bool _focusText { false };
    bool _isCleaned{ false };
    uint8_t _maxFps { 60 };
    MouseTranslator _mouseTranslator { [](const QPointF& p) { return p.toPoint();  } };
    QWindow* _proxyWindow { nullptr };

    QQuickItem* _currentFocusItem { nullptr };

    struct TouchState {
        QTouchEvent::TouchPoint touchPoint;
        bool hovering { false };
        bool pressed { false };
    };

    bool _pressed;
    bool _touchBeginAccepted { false };
    std::map<uint32_t, TouchState> _activeTouchPoints;
};

#endif
