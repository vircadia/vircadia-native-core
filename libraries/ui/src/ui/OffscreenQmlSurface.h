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

class QWindow;
class QMyQuickRenderControl;
class OffscreenGLCanvas;
class QOpenGLContext;
class QQmlEngine;
class QQmlContext;
class QQmlComponent;
class QQuickWindow;
class QQuickItem;

// GPU resources are typically buffered for one copy being used by the renderer, 
// one copy in flight, and one copy being used by the receiver
#define GPU_RESOURCE_BUFFER_SIZE 3

class OffscreenQmlSurface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool focusText READ isFocusText NOTIFY focusTextChanged)
public:
    static void setSharedContext(QOpenGLContext* context);

    OffscreenQmlSurface();
    virtual ~OffscreenQmlSurface();

    using MouseTranslator = std::function<QPoint(const QPointF&)>;

    virtual void create();
    void resize(const QSize& size, bool forceResize = false);
    QSize size() const;

    Q_INVOKABLE void load(const QUrl& qmlSource, bool createNewContext, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback = [](QQmlContext*, QObject*) {});
    Q_INVOKABLE void loadInNewContext(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback = [](QQmlContext*, QObject*) {});
    Q_INVOKABLE void load(const QUrl& qmlSource, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback = [](QQmlContext*, QObject*) {});
    Q_INVOKABLE void load(const QString& qmlSourceFile, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback = [](QQmlContext*, QObject*) {}) {
        return load(QUrl(qmlSourceFile), onQmlLoadedCallback);
    }
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

    void setBaseUrl(const QUrl& baseUrl);
    QQuickItem* getRootItem();
    QQuickWindow* getWindow();
    QObject* getEventHandler();
    QQmlContext* getSurfaceContext();

    QPointF mapToVirtualScreen(const QPointF& originalPoint, QObject* originalWidget);
    bool eventFilter(QObject* originalDestination, QEvent* event) override;

    void setKeyboardRaised(QObject* object, bool raised, bool numeric = false);
    Q_INVOKABLE void synthesizeKeyPress(QString key);

    using TextureAndFence = std::pair<uint32_t, void*>;
    // Checks to see if a new texture is available.  If one is, the function returns true and 
    // textureAndFence will be populated with the texture ID and a fence which will be signalled 
    // when the texture is safe to read.
    // Returns false if no new texture is available
    bool fetchTexture(TextureAndFence& textureAndFence);

    static std::function<void(uint32_t, void*)> getDiscardLambda();
    static size_t getUsedTextureMemory();

signals:
    void focusObjectChanged(QObject* newFocus);
    void focusTextChanged(bool focusText);

public slots:
    void onAboutToQuit();
    void focusDestroyed(QObject *obj);

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

    void finishQmlLoad(QQmlComponent* qmlComponent, QQmlContext* qmlContext, std::function<void(QQmlContext*, QObject*)> onQmlLoadedCallback);
    QPointF mapWindowToUi(const QPointF& sourcePosition, QObject* sourceObject);
    void setupFbo();
    bool allowNewFrame(uint8_t fps);
    void render();
    void cleanup();
    QJsonObject getGLContextData();

private slots:
    void updateQuick();
    void onFocusObjectChanged(QObject* newFocus);

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

    // Texture management
    TextureAndFence _latestTextureAndFence { 0, 0 };

    bool _render { false };
    bool _polish { true };
    bool _paused { true };
    bool _focusText { false };
    uint8_t _maxFps { 60 };
    MouseTranslator _mouseTranslator { [](const QPointF& p) { return p.toPoint();  } };
    QWindow* _proxyWindow { nullptr };

    QQuickItem* _currentFocusItem { nullptr };
};

#endif
