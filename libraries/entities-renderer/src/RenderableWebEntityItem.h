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

class OffscreenQmlSurface;

class RenderableWebEntityItem : public WebEntityItem  {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableWebEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    ~RenderableWebEntityItem();

    virtual void render(RenderArgs* args);
    virtual void setSourceUrl(const QString& value);

private:
    OffscreenQmlSurface* _webSurface{ nullptr };
    QMetaObject::Connection _connection;
    uint32_t _texture{ 0 };
    QMutex _textureLock;
};


#endif // hifi_RenderableWebEntityItem_h
