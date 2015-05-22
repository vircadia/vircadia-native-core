//
//  RenderablePolyVoxEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/19/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderablePolyVoxEntityItem_h
#define hifi_RenderablePolyVoxEntityItem_h

#include <PolyVoxCore/SimpleVolume.h>

#include "PolyVoxEntityItem.h"
#include "RenderableDebugableEntityItem.h"

class RenderablePolyVoxEntityItem : public PolyVoxEntityItem {
public:
    static EntityItem* factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderablePolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        PolyVoxEntityItem(entityItemID, properties) { }

    virtual ~RenderablePolyVoxEntityItem();

    void render(RenderArgs* args);
    virtual bool hasModel() const { return true; }
    void getModel();

    virtual void setVoxelVolumeSize(glm::vec3 voxelVolumeSize);

    glm::vec3 metersToVoxelCoordinates(glm::vec3 metersOffCenter);
    glm::vec3 voxelCoordinatesToMeters(glm::vec3 voxelCoords);

    void setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue);
    void createSphereInVolume(glm::vec3 center, float radius);
    void eraseSphereInVolume(glm::vec3 center, float radius);

private:
    PolyVox::SimpleVolume<uint8_t>* _volData = nullptr;
    model::Geometry _modelGeometry;
    bool _needsModelReload = true;
};


#endif // hifi_RenderablePolyVoxEntityItem_h
