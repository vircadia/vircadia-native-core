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
#include <functional>

#include <QtCore/QJsonObject>
#include <QTimer>
#include <QUrl>


#include <GLMHelpers.h>
#include <ThreadHelpers.h>
#include "TextureRecycler.h"

class QWindow;
class QMyQuickRenderControl;
class OffscreenGLCanvas;
class QOpenGLContext;
class QQmlEngine;
class QQmlContext;
class QQmlComponent;
class QQuickWindow;
class QQuickItem;

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
    void clearCache();

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

    using TextureAndFence = std::pair<uint32_t, void*>;
    // Checks to see if a new texture is available.  If one is, the function returns true and 
    // textureAndFence will be populated with the texture ID and a fence which will be signalled 
    // when the texture is safe to read.
    // Returns false if no new texture is available
    bool fetchTexture(TextureAndFence& textureAndFence);
    // Release a previously acquired texture, along with a fence which indicates when reads from the 
    // texture have completed.
    void releaseTexture(const TextureAndFence& textureAndFence);

signals:
    void focusObjectChanged(QObject* newFocus);
    void focusTextChanged(bool focusText);

public slots:
    void onAboutToQuit();

protected:
    bool filterEnabled(QObject* originalDestination, QEvent* event) const;
    void setFocusText(bool newFocusText);

private:
    QObject* finishQmlLoad(std::function<void(QQmlContext*, QObject*)> f);
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
    QQmlEngine* _qmlEngine { nullptr };
    QQmlComponent* _qmlComponent { nullptr };
    QQuickItem* _rootItem { nullptr };
    OffscreenGLCanvas* _canvas { nullptr };
    QJsonObject _glData;

    QTimer _updateTimer;
    uint32_t _fbo { 0 };
    uint32_t _depthStencil { 0 };
    uint64_t _lastRenderTime { 0 };
    uvec2 _size { 1920, 1080 };
    TextureRecycler _textures { true };

    // Texture management
    std::mutex _textureMutex;
    TextureAndFence _latestTextureAndFence { 0, 0 };
    std::list<TextureAndFence> _returnedTextures;


    bool _render { false };
    bool _polish { true };
    bool _paused { true };
    bool _focusText { false };
    uint8_t _maxFps { 60 };
    MouseTranslator _mouseTranslator { [](const QPointF& p) { return p.toPoint();  } };
    QWindow* _proxyWindow { nullptr };
};

#endif
