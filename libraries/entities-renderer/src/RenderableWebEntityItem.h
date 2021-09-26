//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableWebEntityItem_h
#define hifi_RenderableWebEntityItem_h

#include <QtCore/QSharedPointer>
#include <WebEntityItem.h>
#include "RenderableEntityItem.h"

class OffscreenQmlSurface;
class PointerEvent;

namespace render { namespace entities {

class WebEntityRenderer : public TypedEntityRenderer<WebEntityItem> {
    Q_OBJECT
    using Parent = TypedEntityRenderer<WebEntityItem>;
    friend class EntityRenderer;

public:
    WebEntityRenderer(const EntityItemPointer& entity);
    ~WebEntityRenderer();

    Q_INVOKABLE void hoverEnterEntity(const PointerEvent& event);
    Q_INVOKABLE void hoverLeaveEntity(const PointerEvent& event);
    Q_INVOKABLE void handlePointerEvent(const PointerEvent& event);

    static const QString QML;
    static const char* URL_PROPERTY;
    static const char* SCRIPT_URL_PROPERTY;
    static const char* GLOBAL_POSITION_PROPERTY;
    static const char* USE_BACKGROUND_PROPERTY;
    static const char* USER_AGENT_PROPERTY;

    static void setAcquireWebSurfaceOperator(std::function<void(const QString&, bool, QSharedPointer<OffscreenQmlSurface>&, bool&)> acquireWebSurfaceOperator) { _acquireWebSurfaceOperator = acquireWebSurfaceOperator; }
    static void acquireWebSurface(const QString& url, bool htmlContent, QSharedPointer<OffscreenQmlSurface>& webSurface, bool& cachedWebSurface) {
        if (_acquireWebSurfaceOperator) {
            _acquireWebSurfaceOperator(url, htmlContent, webSurface, cachedWebSurface);
        }
    }

    static void setReleaseWebSurfaceOperator(std::function<void(QSharedPointer<OffscreenQmlSurface>&, bool&, std::vector<QMetaObject::Connection>&)> releaseWebSurfaceOperator) { _releaseWebSurfaceOperator = releaseWebSurfaceOperator; }
    static void releaseWebSurface(QSharedPointer<OffscreenQmlSurface>& webSurface, bool& cachedWebSurface, std::vector<QMetaObject::Connection>& connections) {
        if (_releaseWebSurfaceOperator) {
            _releaseWebSurfaceOperator(webSurface, cachedWebSurface, connections);
        }
    }

    virtual void setProxyWindow(QWindow* proxyWindow) override;
    virtual QObject* getEventHandler() override;

    gpu::TexturePointer getTexture() override { return _texture; }

protected:
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;
    virtual bool isTransparent() const override;

    virtual bool wantsHandControllerPointerEvents() const override { return true; }
    virtual bool wantsKeyboardFocus() const override { return true; }

    void handlePointerEventAsTouch(const PointerEvent& event);
    void handlePointerEventAsMouse(const PointerEvent& event);

private:
    void onTimeout();
    void buildWebSurface(const EntityItemPointer& entity, const QString& newSourceURL);
    void destroyWebSurface();
    glm::vec2 getWindowSize(const TypedEntityPointer& entity) const;

    int _geometryId{ 0 };
    enum class ContentType {
        NoContent,
        HtmlContent,
        QmlContent
    };
    static ContentType getContentType(const QString& urlString);
    ContentType _contentType { ContentType::NoContent };

    QSharedPointer<OffscreenQmlSurface> _webSurface { nullptr };
    bool _cachedWebSurface { false };
    gpu::TexturePointer _texture;
    QString _tryingToBuildURL;

    glm::u8vec3 _color;
    float _alpha { 1.0f };
    PulsePropertyGroup _pulseProperties;

    QString _sourceURL;
    uint16_t _dpi;
    QString _scriptURL;
    uint8_t _maxFPS;
    bool _useBackground;
    QString _userAgent;
    WebInputMode _inputMode;

    glm::vec3 _contextPosition;

    QTimer _timer;
    uint64_t _lastRenderTime { 0 };

    std::vector<QMetaObject::Connection> _connections;

    static std::function<void(QString, bool, QSharedPointer<OffscreenQmlSurface>&, bool&)> _acquireWebSurfaceOperator;
    static std::function<void(QSharedPointer<OffscreenQmlSurface>&, bool&, std::vector<QMetaObject::Connection>&)> _releaseWebSurfaceOperator;

public slots:
    void emitScriptEvent(const QVariant& scriptMessage) override;

signals:
    void scriptEventReceived(const QVariant& message);
    void webEventReceived(const QVariant& message);
};

} }

#endif // hifi_RenderableWebEntityItem_h
