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

#include <EnvironmentData.h>

#include "AtmospherePropertyGroup.h"
#include "EntityItem.h"
#include "EntityTree.h"

class ZoneEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    ZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    
    ALLOW_INSTANTIATION // This class can be instantiated
    
    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties() const;
    virtual bool setProperties(const EntityItemProperties& properties);

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);

    xColor getKeyLightColor() const { xColor color = { _keyLightColor[RED_INDEX], _keyLightColor[GREEN_INDEX], _keyLightColor[BLUE_INDEX] }; return color; }
    void setKeyLightColor(const xColor& value) {
        _keyLightColor[RED_INDEX] = value.red;
        _keyLightColor[GREEN_INDEX] = value.green;
        _keyLightColor[BLUE_INDEX] = value.blue;
    }

    void setKeyLightColor(const rgbColor& value) {
        _keyLightColor[RED_INDEX] = value[RED_INDEX];
        _keyLightColor[GREEN_INDEX] = value[GREEN_INDEX];
        _keyLightColor[BLUE_INDEX] = value[BLUE_INDEX];
    }

    glm::vec3 getKeyLightColorVec3() const {
        const quint8 MAX_COLOR = 255;
        glm::vec3 color = { (float)_keyLightColor[RED_INDEX] / (float)MAX_COLOR,
                            (float)_keyLightColor[GREEN_INDEX] / (float)MAX_COLOR,
                            (float)_keyLightColor[BLUE_INDEX] / (float)MAX_COLOR };
        return color;
    }

    
    float getKeyLightIntensity() const { return _keyLightIntensity; }
    void setKeyLightIntensity(float value) { _keyLightIntensity = value; }

    float getKeyLightAmbientIntensity() const { return _keyLightAmbientIntensity; }
    void setKeyLightAmbientIntensity(float value) { _keyLightAmbientIntensity = value; }

    const glm::vec3& getKeyLightDirection() const { return _keyLightDirection; }
    void setKeyLightDirection(const glm::vec3& value) { _keyLightDirection = value; }

    static bool getZonesArePickable() { return _zonesArePickable; }
    static void setZonesArePickable(bool value) { _zonesArePickable = value; }

    static bool getDrawZoneBoundaries() { return _drawZoneBoundaries; }
    static void setDrawZoneBoundaries(bool value) { _drawZoneBoundaries = value; }
    
    virtual bool isReadyToComputeShape() { return false; }
    void updateShapeType(ShapeType type) { _shapeType = type; }
    virtual ShapeType getShapeType() const;
    
    virtual bool hasCompoundShapeURL() const { return !_compoundShapeURL.isEmpty(); }
    const QString getCompoundShapeURL() const { return _compoundShapeURL; }
    virtual void setCompoundShapeURL(const QString& url);

    void setBackgroundMode(BackgroundMode value) { _backgroundMode = value; }
    BackgroundMode getBackgroundMode() const { return _backgroundMode; }

    EnvironmentData getEnvironmentData() const;
    const AtmospherePropertyGroup& getAtmosphereProperties() const { return _atmosphereProperties; }
    const SkyboxPropertyGroup& getSkyboxProperties() const { return _skyboxProperties; }
    const StagePropertyGroup& getStageProperties() const { return _stageProperties; }

    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElementPointer& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const;

    virtual void debugDump() const;

    static const xColor DEFAULT_KEYLIGHT_COLOR;
    static const float DEFAULT_KEYLIGHT_INTENSITY;
    static const float DEFAULT_KEYLIGHT_AMBIENT_INTENSITY;
    static const glm::vec3 DEFAULT_KEYLIGHT_DIRECTION;
    static const ShapeType DEFAULT_SHAPE_TYPE;
    static const QString DEFAULT_COMPOUND_SHAPE_URL;
    
protected:
    // properties of the "sun" in the zone
    rgbColor _keyLightColor;
    float _keyLightIntensity;
    float _keyLightAmbientIntensity;
    glm::vec3 _keyLightDirection;
    
    ShapeType _shapeType = DEFAULT_SHAPE_TYPE;
    QString _compoundShapeURL;
    
    BackgroundMode _backgroundMode = BACKGROUND_MODE_INHERIT;

    StagePropertyGroup _stageProperties;
    AtmospherePropertyGroup _atmosphereProperties;
    SkyboxPropertyGroup _skyboxProperties;

    static bool _drawZoneBoundaries;
    static bool _zonesArePickable;
};

#endif // hifi_ZoneEntityItem_h
