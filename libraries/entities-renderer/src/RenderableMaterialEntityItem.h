//
//  Created by Sam Gondelman on 1/18/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableMaterialEntityItem_h
#define hifi_RenderableMaterialEntityItem_h

#include "RenderableEntityItem.h"

#include <MaterialEntityItem.h>

#include <procedural/ProceduralMaterialCache.h>

class NetworkMaterial;

namespace render { namespace entities { 

class MaterialEntityRenderer : public TypedEntityRenderer<MaterialEntityItem> {
    using Parent = TypedEntityRenderer<MaterialEntityItem>;
    using Pointer = std::shared_ptr<MaterialEntityRenderer>;
public:
    MaterialEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {}
    ~MaterialEntityRenderer() { deleteMaterial(_parentID, _parentMaterialName); }

    graphics::MaterialPointer getTopMaterial() override { return getMaterial(); }

private:
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;

    ItemKey getKey() override;
    ShapeKey getShapeKey() override;

    QString _materialURL;
    QString _materialData;
    QString _parentMaterialName;
    quint16 _priority;
    QUuid _parentID;

    MaterialMappingMode _materialMappingMode { UNSET_MATERIAL_MAPPING_MODE };
    bool _materialRepeat { false };
    glm::vec2 _materialMappingPos;
    glm::vec2 _materialMappingScale;
    float _materialMappingRot;
    Transform _transform;
    glm::vec3 _dimensions;

    bool _texturesLoaded { false };
    bool _retryApply { false };

    std::shared_ptr<NetworkMaterial> getMaterial() const;
    void setCurrentMaterialName(const std::string& currentMaterialName);

    void applyTextureTransform(std::shared_ptr<NetworkMaterial>& material);
    void applyMaterial(const TypedEntityPointer& entity);
    void deleteMaterial(const QUuid& oldParentID, const QString& oldParentMaterialName);

    NetworkMaterialResourcePointer _networkMaterial;
    NetworkMaterialResource::ParsedMaterials _parsedMaterials;
    std::shared_ptr<NetworkMaterial> _appliedMaterial;
    std::string _currentMaterialName;

};

} } 
#endif // hifi_RenderableMaterialEntityItem_h
