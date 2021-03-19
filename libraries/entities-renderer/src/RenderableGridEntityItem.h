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
    ~GridEntityRenderer();

protected:
    Item::Bound getBound(RenderArgs* args) override;
    ShapeKey getShapeKey() override;

    bool isTransparent() const override;

private:
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;

    glm::u8vec3 _color;
    float _alpha { NAN };
    PulsePropertyGroup _pulseProperties;

    bool _followCamera;
    uint32_t _majorGridEvery;
    float _minorGridEvery;

    glm::vec3 _dimensions;

    int _geometryId { 0 };

};

} } 
#endif // hifi_RenderableGridEntityItem_h
