//
//  Created by Bradley Austin Davis on 2016/05/09
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableShapeEntityItem_h
#define hifi_RenderableShapeEntityItem_h

#include "RenderableEntityItem.h"

#include <procedural/Procedural.h>
#include <ShapeEntityItem.h>

namespace render { namespace entities { 

class ShapeEntityRenderer : public TypedEntityRenderer<ShapeEntityItem> {
    using Parent = TypedEntityRenderer<ShapeEntityItem>;
    using Pointer = std::shared_ptr<ShapeEntityRenderer>;
public:
    ShapeEntityRenderer(const EntityItemPointer& entity);

    virtual scriptable::ScriptableModelBase getScriptableModel() override;

protected:
    ShapeKey getShapeKey() override;
    Item::Bound getBound() override;

private:
    virtual bool needsRenderUpdate() const override;
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;
    virtual bool isTransparent() const override;

    enum Pipeline { SIMPLE, MATERIAL, PROCEDURAL };
    Pipeline getPipelineType(const graphics::MultiMaterial& materials) const;

    QString _proceduralData;
    entity::Shape _shape { entity::Sphere };

    PulsePropertyGroup _pulseProperties;
    std::shared_ptr<graphics::ProceduralMaterial> _material { std::make_shared<graphics::ProceduralMaterial>() };
    glm::vec3 _color { NAN };
    float _alpha;

    glm::vec3 _position;
    glm::vec3 _dimensions;
    glm::quat _orientation;
};

} } 
#endif // hifi_RenderableShapeEntityItem_h
