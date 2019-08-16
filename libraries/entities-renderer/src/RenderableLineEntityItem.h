//
//  RenderableLineEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableLineEntityItem_h
#define hifi_RenderableLineEntityItem_h

#include "RenderableEntityItem.h"
#include <LineEntityItem.h>
#include <GeometryCache.h>


namespace render { namespace entities { 

class LineEntityRenderer : public TypedEntityRenderer<LineEntityItem> {
    using Parent = TypedEntityRenderer<LineEntityItem>;
    friend class EntityRenderer;

public:
    LineEntityRenderer(const EntityItemPointer& entity) : Parent(entity) { }

protected:
    virtual void onRemoveFromSceneTyped(const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;

private:
    int _lineVerticesID { GeometryCache::UNKNOWN_ID };
    QVector<glm::vec3> _linePoints;
};

} } // namespace 

#endif // hifi_RenderableLineEntityItem_h
