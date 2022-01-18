//
//  ZoneEntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ZoneEntityItem_h
#define hifi_ZoneEntityItem_h

#include <ComponentMode.h>
#include <model-networking/ModelCache.h>

#include "KeyLightPropertyGroup.h"
#include "AmbientLightPropertyGroup.h"
#include "EntityItem.h"
#include "EntityTree.h"
#include "SkyboxPropertyGroup.h"
#include "HazePropertyGroup.h"
#include "BloomPropertyGroup.h"

class ZoneEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ZoneEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    virtual bool setSubClassProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;



    static bool getZonesArePickable() { return _zonesArePickable; }
    static void setZonesArePickable(bool value) { _zonesArePickable = value; }

    static bool getDrawZoneBoundaries() { return _drawZoneBoundaries; }
    static void setDrawZoneBoundaries(bool value) { _drawZoneBoundaries = value; }

    virtual bool isReadyToComputeShape() const override { return false; }
    virtual void setShapeType(ShapeType type) override;
    virtual ShapeType getShapeType() const override;
    bool shouldBePhysical() const override { return false; }

    QString getCompoundShapeURL() const;
    virtual void setCompoundShapeURL(const QString& url);

    virtual bool matchesJSONFilters(const QJsonObject& jsonFilters) const override;

    KeyLightPropertyGroup getKeyLightProperties() const { return resultWithReadLock<KeyLightPropertyGroup>([&] { return _keyLightProperties; }); }
    AmbientLightPropertyGroup getAmbientLightProperties() const { return resultWithReadLock<AmbientLightPropertyGroup>([&] { return _ambientLightProperties; }); }

    void setHazeMode(const uint32_t value);
    uint32_t getHazeMode() const;

    void setKeyLightMode(uint32_t value);
    uint32_t getKeyLightMode() const;

    void setAmbientLightMode(uint32_t value);
    uint32_t getAmbientLightMode() const;

    void setSkyboxMode(uint32_t value);
    uint32_t getSkyboxMode() const;

    void setBloomMode(const uint32_t value);
    uint32_t getBloomMode() const;

    SkyboxPropertyGroup getSkyboxProperties() const { return resultWithReadLock<SkyboxPropertyGroup>([&] { return _skyboxProperties; }); }
    
    const HazePropertyGroup& getHazeProperties() const { return _hazeProperties; }
    const BloomPropertyGroup& getBloomProperties() const { return _bloomProperties; }

    bool getFlyingAllowed() const { return _flyingAllowed; }
    void setFlyingAllowed(bool value) { _flyingAllowed = value; }
    bool getGhostingAllowed() const { return _ghostingAllowed; }
    void setGhostingAllowed(bool value) { _ghostingAllowed = value; }
    QString getFilterURL() const;
    void setFilterURL(const QString url); 

    uint32_t getAvatarPriority() const { return _avatarPriority; }
    void setAvatarPriority(uint32_t value) { _avatarPriority = value; }

    uint32_t getScreenshare() const { return _screenshare; }
    void setScreenshare(uint32_t value) { _screenshare = value; }

    void setUserData(const QString& value) override;

    bool keyLightPropertiesChanged() const { return _keyLightPropertiesChanged; }
    bool ambientLightPropertiesChanged() const { return _ambientLightPropertiesChanged; }
    bool skyboxPropertiesChanged() const { return _skyboxPropertiesChanged; }
    bool hazePropertiesChanged() const { return _hazePropertiesChanged; }
    bool bloomPropertiesChanged() const { return _bloomPropertiesChanged; }

    void resetRenderingPropertiesChanged();

    virtual bool supportsDetailedIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& distance,
                         BoxFace& face, glm::vec3& surfaceNormal,
                         QVariantMap& extraInfo, bool precisionPicking) const override;
    virtual bool findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                         const glm::vec3& acceleration, const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                         float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal,
                         QVariantMap& extraInfo, bool precisionPicking) const override;

    bool contains(const glm::vec3& point) const override;

    virtual void debugDump() const override;

    static const ShapeType DEFAULT_SHAPE_TYPE;
    static const QString DEFAULT_COMPOUND_SHAPE_URL;
    static const bool DEFAULT_FLYING_ALLOWED;
    static const bool DEFAULT_GHOSTING_ALLOWED;
    static const QString DEFAULT_FILTER_URL;

protected:
    KeyLightPropertyGroup _keyLightProperties;
    AmbientLightPropertyGroup _ambientLightProperties;

    ShapeType _shapeType { DEFAULT_SHAPE_TYPE };
    QString _compoundShapeURL;

    // The following 3 values are the defaults for zone creation
    uint32_t _keyLightMode { COMPONENT_MODE_INHERIT };
    uint32_t _skyboxMode { COMPONENT_MODE_INHERIT };
    uint32_t _ambientLightMode { COMPONENT_MODE_INHERIT };

    uint32_t _hazeMode { COMPONENT_MODE_INHERIT };
    uint32_t _bloomMode { COMPONENT_MODE_INHERIT };

    SkyboxPropertyGroup _skyboxProperties;
    HazePropertyGroup _hazeProperties;
    BloomPropertyGroup _bloomProperties;

    bool _flyingAllowed { DEFAULT_FLYING_ALLOWED };
    bool _ghostingAllowed { DEFAULT_GHOSTING_ALLOWED };
    QString _filterURL { DEFAULT_FILTER_URL };

    // Avatar-updates priority
    uint32_t _avatarPriority { COMPONENT_MODE_INHERIT };

    // Screen-sharing zone
    uint32_t _screenshare { COMPONENT_MODE_INHERIT };

    // Dirty flags turn true when either keylight properties is changing values.
    bool _keyLightPropertiesChanged { false };
    bool _ambientLightPropertiesChanged { false };
    bool _skyboxPropertiesChanged { false };
    bool _hazePropertiesChanged{ false };
    bool _bloomPropertiesChanged { false };

    static bool _drawZoneBoundaries;
    static bool _zonesArePickable;

    void fetchCollisionGeometryResource();
    GeometryResource::Pointer _shapeResource;

};

#endif // hifi_ZoneEntityItem_h
