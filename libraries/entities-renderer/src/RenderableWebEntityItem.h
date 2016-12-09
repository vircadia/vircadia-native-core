//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableWebEntityItem_h
#define hifi_RenderableWebEntityItem_h

#include <QSharedPointer>
#include <QMouseEvent>
#include <QTouchEvent>
#include <PointerEvent.h>
#include <gl/OffscreenQmlSurface.h>

#include <WebEntityItem.h>
#include <gl/OffscreenQmlSurface.h>

#include "RenderableEntityItem.h"


class QWindow;
class QObject;
class EntityTreeRenderer;
class RenderableWebEntityItem;


class RenderableWebEntityItem : public WebEntityItem  {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableWebEntityItem(const EntityItemID& entityItemID);
    ~RenderableWebEntityItem();

    virtual void render(RenderArgs* args) override;
    void loadSourceURL();
    virtual void setSourceUrl(const QString& value) override;

    virtual bool wantsHandControllerPointerEvents() const override { return true; }
    virtual bool wantsKeyboardFocus() const override { return true; }
    virtual void setProxyWindow(QWindow* proxyWindow) override;
    virtual QObject* getEventHandler() override;

    void handlePointerEvent(const PointerEvent& event);

    void update(const quint64& now) override;
    bool needsToCallUpdate() const override { return _webSurface != nullptr; }

    virtual void emitScriptEvent(const QVariant& message) override;

    SIMPLE_RENDERABLE();

    virtual bool isTransparent() override;

 public:

    virtual QObject* getRootItem() override;

private:
    bool buildWebSurface(QSharedPointer<EntityTreeRenderer> renderer);
    void destroyWebSurface();
    glm::vec2 getWindowSize() const;

    QSharedPointer<OffscreenQmlSurface> _webSurface;
    QMetaObject::Connection _connection;
    gpu::TexturePointer _texture;
    ivec2  _lastPress { INT_MIN };
    bool _pressed{ false };
    QTouchEvent _lastTouchEvent { QEvent::TouchUpdate };
    uint64_t _lastRenderTime{ 0 };
    QTouchDevice _touchDevice;

    QMetaObject::Connection _mousePressConnection;
    QMetaObject::Connection _mouseReleaseConnection;
    QMetaObject::Connection _mouseMoveConnection;
    QMetaObject::Connection _hoverLeaveConnection;

    QString _javaScriptToInject;

    enum contentType {
        htmlContent,
        qmlContent
    };
    contentType _contentType;
    int _geometryId { 0 };
};

#endif // hifi_RenderableWebEntityItem_h
