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

    PolyVoxEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                 bool& somethingChanged) override;

    // never have a ray intersection pick a PolyVoxEntityItem.
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                             bool& keepSearching, OctreeElementPointer& element, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal,
                                             void** intersectedObject, bool precisionPicking) const override { return false; }

    virtual void debugDump() const override;

    virtual void setVoxelVolumeSize(glm::vec3 voxelVolumeSize);
    virtual glm::vec3 getVoxelVolumeSize() const;

    virtual void setVoxelData(QByteArray voxelData);
    virtual const QByteArray getVoxelData() const;

    enum PolyVoxSurfaceStyle {
        SURFACE_MARCHING_CUBES,
        SURFACE_CUBIC,
        SURFACE_EDGED_CUBIC,
        SURFACE_EDGED_MARCHING_CUBES
    };

    virtual void setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) { _voxelSurfaceStyle = voxelSurfaceStyle; }
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

    virtual glm::vec3 voxelCoordsToWorldCoords(glm::vec3& voxelCoords) const { return glm::vec3(0.0f); }
    virtual glm::vec3 worldCoordsToVoxelCoords(glm::vec3& worldCoords) const { return glm::vec3(0.0f); }
    virtual glm::vec3 voxelCoordsToLocalCoords(glm::vec3& voxelCoords) const { return glm::vec3(0.0f); }
    virtual glm::vec3 localCoordsToVoxelCoords(glm::vec3& localCoords) const { return glm::vec3(0.0f); }

    // coords are in world-space
    virtual bool setSphere(glm::vec3 center, float radius, uint8_t toValue) { return false; }
    virtual bool setAll(uint8_t toValue) { return false; }
    virtual bool setCuboid(const glm::vec3& lowPosition, const glm::vec3& cuboidSize, int value) { return false; }

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

    virtual void setXNNeighborID(const EntityItemID& xNNeighborID) { _xNNeighborID = xNNeighborID; }
    void setXNNeighborID(const QString& xNNeighborID) { setXNNeighborID(QUuid(xNNeighborID)); }
    virtual const EntityItemID& getXNNeighborID() const { return _xNNeighborID; }
    virtual void setYNNeighborID(const EntityItemID& yNNeighborID) { _yNNeighborID = yNNeighborID; }
    void setYNNeighborID(const QString& yNNeighborID) { setYNNeighborID(QUuid(yNNeighborID)); }
    virtual const EntityItemID& getYNNeighborID() const { return _yNNeighborID; }
    virtual void setZNNeighborID(const EntityItemID& zNNeighborID) { _zNNeighborID = zNNeighborID; }
    void setZNNeighborID(const QString& zNNeighborID) { setZNNeighborID(QUuid(zNNeighborID)); }
    virtual const EntityItemID& getZNNeighborID() const { return _zNNeighborID; }

    virtual void setXPNeighborID(const EntityItemID& xPNeighborID) { _xPNeighborID = xPNeighborID; }
    void setXPNeighborID(const QString& xPNeighborID) { setXPNeighborID(QUuid(xPNeighborID)); }
    virtual const EntityItemID& getXPNeighborID() const { return _xPNeighborID; }
    virtual void setYPNeighborID(const EntityItemID& yPNeighborID) { _yPNeighborID = yPNeighborID; }
    void setYPNeighborID(const QString& yPNeighborID) { setYPNeighborID(QUuid(yPNeighborID)); }
    virtual const EntityItemID& getYPNeighborID() const { return _yPNeighborID; }
    virtual void setZPNeighborID(const EntityItemID& zPNeighborID) { _zPNeighborID = zPNeighborID; }
    void setZPNeighborID(const QString& zPNeighborID) { setZPNeighborID(QUuid(zPNeighborID)); }
    virtual const EntityItemID& getZPNeighborID() const { return _zPNeighborID; }

    virtual void rebakeMesh() {};

    void setVoxelDataDirty(bool value) { withWriteLock([&] { _voxelDataDirty = value; }); }
    virtual void getMesh() {}; // recompute mesh

 protected:
    glm::vec3 _voxelVolumeSize; // this is always 3 bytes

    QByteArray _voxelData;
    bool _voxelDataDirty; // _voxelData has changed, things that depend on it should be updated

    PolyVoxSurfaceStyle _voxelSurfaceStyle;

    QString _xTextureURL;
    QString _yTextureURL;
    QString _zTextureURL;

    // for non-edged surface styles, these are used to compute the high-axis edges
    EntityItemID _xNNeighborID{UNKNOWN_ENTITY_ID};
    EntityItemID _yNNeighborID{UNKNOWN_ENTITY_ID};
    EntityItemID _zNNeighborID{UNKNOWN_ENTITY_ID};

    EntityItemID _xPNeighborID{UNKNOWN_ENTITY_ID};
    EntityItemID _yPNeighborID{UNKNOWN_ENTITY_ID};
    EntityItemID _zPNeighborID{UNKNOWN_ENTITY_ID};
};

#endif // hifi_PolyVoxEntityItem_h
