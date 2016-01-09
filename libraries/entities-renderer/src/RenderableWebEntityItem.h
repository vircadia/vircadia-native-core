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

#include <WebEntityItem.h>

#include "RenderableEntityItem.h"

class OffscreenQmlSurface;
class QWindow;
class QObject;

class RenderableWebEntityItem : public WebEntityItem  {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableWebEntityItem(const EntityItemID& entityItemID);
    ~RenderableWebEntityItem();

    virtual void render(RenderArgs* args);
    virtual void setSourceUrl(const QString& value);
    
    void setProxyWindow(QWindow* proxyWindow);
    QObject* getEventHandler();

    SIMPLE_RENDERABLE();

private:
    OffscreenQmlSurface* _webSurface{ nullptr };
    QMetaObject::Connection _connection;
    uint32_t _texture{ 0 };
    ivec2  _lastPress{ INT_MIN };
    bool _pressed{ false };
    ivec2 _lastMove{ INT_MIN };

    QMetaObject::Connection _mousePressConnection;
    QMetaObject::Connection _mouseReleaseConnection;
    QMetaObject::Connection _mouseMoveConnection;
    QMetaObject::Connection _hoverLeaveConnection;
};


#endif // hifi_RenderableWebEntityItem_h
