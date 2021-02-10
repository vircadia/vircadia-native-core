//
//  Created by Sam Gondelman on 1/22/19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GizmoEntityItem_h
#define hifi_GizmoEntityItem_h

#include "EntityItem.h"

#include "RingGizmoPropertyGroup.h"

class GizmoEntityItem : public EntityItem {
    using Pointer = std::shared_ptr<GizmoEntityItem>;
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    GizmoEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    virtual void setUnscaledDimensions(const glm::vec3& value) override;

    // methods for getting/setting all properties of an entity
    EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    bool setSubClassProperties(const EntityItemProperties& properties) override;

    EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                            EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                            EntityPropertyFlags& requestedProperties,
                            EntityPropertyFlags& propertyFlags,
                            EntityPropertyFlags& propertiesDidntFit,
                            int& propertyCount,
                            OctreeElement::AppendState& appendState) const override;

    int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                         ReadBitstreamToTreeParams& args,
                                         EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                         bool& somethingChanged) override;

    bool supportsDetailedIntersection() const override;
    bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& distance,
        BoxFace& face, glm::vec3& surfaceNormal,
        QVariantMap& extraInfo, bool precisionPicking) const override;
    bool findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
        const glm::vec3& acceleration, const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
        float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal,
        QVariantMap& extraInfo, bool precisionPicking) const override;
    bool getRotateForPicking() const override { return getBillboardMode() != BillboardMode::NONE; }

    GizmoType getGizmoType() const;
    void setGizmoType(GizmoType value);

    RingGizmoPropertyGroup getRingProperties() const;

protected:
    GizmoType _gizmoType;
    RingGizmoPropertyGroup _ringProperties;

};

#endif // hifi_GizmoEntityItem_h
