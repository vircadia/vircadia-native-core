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
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderablePolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        PolyVoxEntityItem(entityItemID, properties) { }

    virtual ~RenderablePolyVoxEntityItem();

    virtual void somethingChangedNotification() {
        // This gets called from EnityItem::readEntityDataFromBuffer every time a packet describing
        // this entity comes from the entity-server.  It gets called even if nothing has actually changed
        // (see the comment in EntityItem.cpp).  If that gets fixed, this could be used to know if we
        // need to redo the voxel data.
        // _needsModelReload = true;
    }


    void render(RenderArgs* args);
    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject, bool precisionPicking) const;

    void getModel();

    virtual void setVoxelData(QByteArray voxelData);

    virtual void setVoxelVolumeSize(glm::vec3 voxelVolumeSize);
    glm::mat4 voxelToWorldMatrix() const;
    glm::mat4 worldToVoxelMatrix() const;

    // coords are in voxel-volume space
    virtual void setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue);

    // coords are in world-space
    virtual void setSphere(glm::vec3 center, float radius, uint8_t toValue);

private:
    void compressVolumeData();
    void decompressVolumeData();

    PolyVox::SimpleVolume<uint8_t>* _volData = nullptr;
    model::Geometry _modelGeometry;
    bool _needsModelReload = true;
};


#endif // hifi_RenderablePolyVoxEntityItem_h
