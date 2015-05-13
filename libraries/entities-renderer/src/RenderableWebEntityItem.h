//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableWebEntityItem_h
#define hifi_RenderableWebEntityItem_h

#include <WebEntityItem.h>

class OffscreenQmlSurface;

class RenderableWebEntityItem : public WebEntityItem  {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableWebEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    virtual void render(RenderArgs* args);

    virtual void setSourceUrl(const QString& value);

private:
    void updateQmlSourceUrl();

    OffscreenQmlSurface* _webSurface{ nullptr };
    uint32_t _texture{ 0 };
};


#endif // hifi_RenderableWebEntityItem_h
