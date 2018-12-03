//
//  Created by Sam Gondelman on 11/29/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableGridEntityItem_h
#define hifi_RenderableGridEntityItem_h

#include "RenderableEntityItem.h"

#include <GridEntityItem.h>

namespace render { namespace entities { 

class GridEntityRenderer : public TypedEntityRenderer<GridEntityItem> {
    using Parent = TypedEntityRenderer<GridEntityItem>;
    using Pointer = std::shared_ptr<GridEntityRenderer>;
public:
    GridEntityRenderer(const EntityItemPointer& entity);

protected:
    ItemKey getKey() override;
    ShapeKey getShapeKey() override;

private:
    virtual bool needsRenderUpdate() const override;
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;
};

} } 
#endif // hifi_RenderableGridEntityItem_h
