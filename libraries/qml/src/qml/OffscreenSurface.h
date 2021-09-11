//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_qml_OffscreenSurface_h
#define hifi_qml_OffscreenSurface_h

#include <atomic>
#include <queue>
#include <map>
#include <functional>

#include <QtCore/QUrl>
#include <QtCore/QSize>
#include <QtCore/QPointF>
#include <QtCore/QTimer>

#include <QtQml/QJSValue>

class QWindow;
class QOpenGLContext;
class QQmlContext;
class QQmlEngine;
class QQmlComponent;
class QQuickWindow;
class QQuickItem;
class OffscreenQmlSharedObject;
class QQmlFileSelector;

namespace hifi { namespace qml {

namespace impl {
class SharedObject;
}

using QmlContextCallback = ::std::function<void(QQmlContext*)>;
using QmlContextObjectCallback = ::std::function<void(QQmlContext*, QQuickItem*)>;
using QmlUrlValidator = std::function<bool(const QUrl&)>;

class OffscreenSurface : public QObject {
    Q_OBJECT

public:
    static const QmlContextObjectCallback DEFAULT_CONTEXT_OBJECT_CALLBACK;
    static const QmlContextCallback DEFAULT_CONTEXT_CALLBACK;
    static QmlUrlValidator validator;
    using TextureAndFence = std::pair<uint32_t, void*>;
    using MouseTranslator = std::function<QPoint(const QPointF&)>;


    static const QmlUrlValidator& getUrlValidator() { return validator; }
    static void setUrlValidator(const QmlUrlValidator& newValidator) { validator = newValidator; }
    static void setSharedContext(QOpenGLContext* context);

    OffscreenSurface();
    virtual ~OffscreenSurface();

    QSize size() const;
    virtual void resize(const QSize& size);
    void clearCache();

    void setMaxFps(uint8_t maxFps);
    // Optional values for event handling
    void setProxyWindow(QWindow* window);
    void setMouseTranslator(const MouseTranslator& mouseTranslator) { _mouseTranslator = mouseTranslator; }

    void pause();
    void resume();
    bool isPaused() const;

    QQuickItem* getRootItem();
    QQuickWindow* getWindow();
    QObject* getEventHandler();
    QQmlContext* getSurfaceContext();
    QQmlFileSelector* getFileSelector();

    // Checks to see if a new texture is available.  If one is, the function returns true and
    // textureAndFence will be populated with the texture ID and a fence which will be signalled
    // when the texture is safe to read.
    // Returns false if no new texture is available
    bool fetchTexture(TextureAndFence& textureAndFence);

    static std::function<void(uint32_t, void*)> getDiscardLambda();
    static size_t getUsedTextureMemory();
    QPointF mapToVirtualScreen(const QPointF& originalPoint);

    // For use from QML/JS
    Q_INVOKABLE void load(const QUrl& qmlSource, QQuickItem* parent, const QJSValue& callback);

    // For use from C++
    Q_INVOKABLE void load(const QUrl& qmlSource, const QmlContextObjectCallback& callback = DEFAULT_CONTEXT_OBJECT_CALLBACK);
    Q_INVOKABLE void load(const QUrl& qmlSource,
                          bool createNewContext,
                          const QmlContextObjectCallback& callback = DEFAULT_CONTEXT_OBJECT_CALLBACK);
    Q_INVOKABLE void load(const QString& qmlSourceFile,
                          const QmlContextObjectCallback& callback = DEFAULT_CONTEXT_OBJECT_CALLBACK);
    Q_INVOKABLE void loadInNewContext(const QUrl& qmlSource,
                                      const QmlContextObjectCallback& callback = DEFAULT_CONTEXT_OBJECT_CALLBACK,
                                      const QmlContextCallback& contextCallback = DEFAULT_CONTEXT_CALLBACK);

public slots:
    virtual void onFocusObjectChanged(QObject* newFocus) {}

signals:
    void rootContextCreated(QQmlContext* rootContext);
    void rootItemCreated(QQuickItem* rootContext);

protected:
    virtual void loadFromQml(const QUrl& qmlSource, QQuickItem* parent, const QJSValue& callback);
    bool eventFilter(QObject* originalDestination, QEvent* event) override;
    bool filterEnabled(QObject* originalDestination, QEvent* event) const;

    virtual void initializeEngine(QQmlEngine* engine);
    virtual void loadInternal(const QUrl& qmlSource,
                              bool createNewContext,
                              QQuickItem* parent,
                              const QmlContextObjectCallback& callback,
                              const QmlContextCallback& contextCallback = DEFAULT_CONTEXT_CALLBACK) final;
    virtual void finishQmlLoad(QQmlComponent* qmlComponent,
                               QQmlContext* qmlContext,
                               QQuickItem* parent,
                               const QmlContextObjectCallback& onQmlLoadedCallback) final;

    virtual void onRootCreated() {}
    virtual void onItemCreated(QQmlContext* context, QQuickItem* newItem) {}
    virtual void onRootContextCreated(QQmlContext* qmlContext) {}

    virtual QQmlContext* contextForUrl(const QUrl& qmlSource, QQuickItem* parent, bool forceNewContext);

private:
    MouseTranslator _mouseTranslator{ [](const QPointF& p) { return p.toPoint(); } };
    friend class hifi::qml::impl::SharedObject;
    impl::SharedObject* const _sharedObject;
};

}}  // namespace hifi::qml

#endif
