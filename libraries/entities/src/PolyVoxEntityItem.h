//
//  PolyVoxEntityItem.h
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PolyVoxEntityItem_h
#define hifi_PolyVoxEntityItem_h

#include "EntityItem.h"

class PolyVoxEntityItem : public EntityItem {
 public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    PolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

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

    // never have a ray intersection pick a PolyVoxEntityItem.
    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const { return false; }

    virtual void debugDump() const;

    virtual void setVoxelVolumeSize(glm::vec3 voxelVolumeSize);
    virtual const glm::vec3& getVoxelVolumeSize() const { return _voxelVolumeSize; }

    virtual void setVoxelData(QByteArray voxelData) { _voxelData = voxelData; }
    virtual const QByteArray& getVoxelData() const { return _voxelData; }

    enum PolyVoxSurfaceStyle {
        SURFACE_MARCHING_CUBES,
        SURFACE_CUBIC,
        SURFACE_EDGED_CUBIC,
        SURFACE_EDGED_MARCHING_CUBES
    };

    void setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle);
    // this other version of setVoxelSurfaceStyle is needed for SET_ENTITY_PROPERTY_FROM_PROPERTIES
    void setVoxelSurfaceStyle(uint16_t voxelSurfaceStyle) { setVoxelSurfaceStyle((PolyVoxSurfaceStyle) voxelSurfaceStyle); }
    virtual PolyVoxSurfaceStyle getVoxelSurfaceStyle() const { return _voxelSurfaceStyle; }

    static const glm::vec3 DEFAULT_VOXEL_VOLUME_SIZE;
    static const float MAX_VOXEL_DIMENSION;

    static const QByteArray DEFAULT_VOXEL_DATA;
    static const PolyVoxSurfaceStyle DEFAULT_VOXEL_SURFACE_STYLE;

    // coords are in voxel-volume space
    virtual bool setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue) { return false; }
    virtual bool setVoxelInVolume(glm::vec3 position, uint8_t toValue) { return false; }

    virtual glm::vec3 voxelCoordsToWorldCoords(glm::vec3 voxelCoords) { return glm::vec3(0.0f); }
    virtual glm::vec3 worldCoordsToVoxelCoords(glm::vec3 worldCoords) { return glm::vec3(0.0f); }
    virtual glm::vec3 voxelCoordsToLocalCoords(glm::vec3 voxelCoords) { return glm::vec3(0.0f); }
    virtual glm::vec3 localCoordsToVoxelCoords(glm::vec3 localCoords) { return glm::vec3(0.0f); }

    // coords are in world-space
    virtual bool setSphere(glm::vec3 center, float radius, uint8_t toValue) { return false; }
    virtual bool setAll(uint8_t toValue) { return false; }

    virtual uint8_t getVoxel(int x, int y, int z) { return 0; }
    virtual bool setVoxel(int x, int y, int z, uint8_t toValue) { return false; }

    static QByteArray makeEmptyVoxelData(quint16 voxelXSize = 16, quint16 voxelYSize = 16, quint16 voxelZSize = 16);

    static const QString DEFAULT_X_TEXTURE_URL;
    virtual void setXTextureURL(QString xTextureURL) { _xTextureURL = xTextureURL; }
    virtual const QString& getXTextureURL() const { return _xTextureURL; }

    static const QString DEFAULT_Y_TEXTURE_URL;
    virtual void setYTextureURL(QString yTextureURL) { _yTextureURL = yTextureURL; }
    virtual const QString& getYTextureURL() const { return _yTextureURL; }

    static const QString DEFAULT_Z_TEXTURE_URL;
    virtual void setZTextureURL(QString zTextureURL) { _zTextureURL = zTextureURL; }
    virtual const QString& getZTextureURL() const { return _zTextureURL; }

 protected:
    virtual void updateVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) {
        _voxelSurfaceStyle = voxelSurfaceStyle;
    }

    glm::vec3 _voxelVolumeSize; // this is always 3 bytes
    QByteArray _voxelData;
    PolyVoxSurfaceStyle _voxelSurfaceStyle;

    QString _xTextureURL;
    QString _yTextureURL;
    QString _zTextureURL;

};

#endif // hifi_PolyVoxEntityItem_h
