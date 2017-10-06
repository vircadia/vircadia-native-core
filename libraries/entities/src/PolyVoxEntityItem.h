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

    // TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
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

    // never have a ray intersection pick a PolyVoxEntityItem.
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                             bool& keepSearching, OctreeElementPointer& element, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal,
                                             void** intersectedObject, bool precisionPicking) const override { return false; }

    virtual void debugDump() const override;

    virtual void setVoxelVolumeSize(const vec3& voxelVolumeSize);
    virtual glm::vec3 getVoxelVolumeSize() const;

    virtual void setVoxelData(const QByteArray& voxelData);
    virtual QByteArray getVoxelData() const;

    virtual int getOnCount() const { return 0; }

    enum PolyVoxSurfaceStyle {
        SURFACE_MARCHING_CUBES,
        SURFACE_CUBIC,
        SURFACE_EDGED_CUBIC,
        SURFACE_EDGED_MARCHING_CUBES
    };
    static bool isEdged(PolyVoxSurfaceStyle surfaceStyle);


    virtual void setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) { _voxelSurfaceStyle = voxelSurfaceStyle; }
    // this other version of setVoxelSurfaceStyle is needed for SET_ENTITY_PROPERTY_FROM_PROPERTIES
    void setVoxelSurfaceStyle(uint16_t voxelSurfaceStyle) { setVoxelSurfaceStyle((PolyVoxSurfaceStyle) voxelSurfaceStyle); }
    virtual PolyVoxSurfaceStyle getVoxelSurfaceStyle() const { return _voxelSurfaceStyle; }

    static const glm::vec3 DEFAULT_VOXEL_VOLUME_SIZE;
    static const float MAX_VOXEL_DIMENSION;

    static const QByteArray DEFAULT_VOXEL_DATA;
    static const PolyVoxSurfaceStyle DEFAULT_VOXEL_SURFACE_STYLE;

    glm::vec3 voxelCoordsToWorldCoords(const glm::vec3& voxelCoords) const;
    glm::vec3 worldCoordsToVoxelCoords(const glm::vec3& worldCoords) const;
    glm::vec3 voxelCoordsToLocalCoords(const glm::vec3& voxelCoords) const;
    glm::vec3 localCoordsToVoxelCoords(const glm::vec3& localCoords) const;

    // coords are in voxel-volume space
    virtual bool setSphereInVolume(const vec3& center, float radius, uint8_t toValue) { return false; }
    virtual bool setVoxelInVolume(const vec3& position, uint8_t toValue) { return false; }
    // coords are in world-space
    virtual bool setSphere(const vec3& center, float radius, uint8_t toValue) { return false; }
    virtual bool setCapsule(const vec3& startWorldCoords, const vec3& endWorldCoords,
                            float radiusWorldCoords, uint8_t toValue) { return false; }
    virtual bool setAll(uint8_t toValue) { return false; }
    virtual bool setCuboid(const vec3& lowPosition, const vec3& cuboidSize, int value) { return false; }

    virtual uint8_t getVoxel(int x, int y, int z) const final { return getVoxel({ x, y, z }); }
    virtual bool setVoxel(int x, int y, int z, uint8_t toValue) final{ return setVoxel({ x, y, z }, toValue); }

    virtual uint8_t getVoxel(const ivec3& v) const { return 0; }
    virtual bool setVoxel(const ivec3& v, uint8_t toValue) { return false; }

    static QByteArray makeEmptyVoxelData(quint16 voxelXSize = 16, quint16 voxelYSize = 16, quint16 voxelZSize = 16);

    static const QString DEFAULT_X_TEXTURE_URL;
    void setXTextureURL(const QString& xTextureURL);
    QString getXTextureURL() const;

    static const QString DEFAULT_Y_TEXTURE_URL;
    void setYTextureURL(const QString& yTextureURL);
    QString getYTextureURL() const;

    static const QString DEFAULT_Z_TEXTURE_URL;
    void setZTextureURL(const QString& zTextureURL);
    QString getZTextureURL() const;

    virtual void setXNNeighborID(const EntityItemID& xNNeighborID);
    void setXNNeighborID(const QString& xNNeighborID);
    EntityItemID getXNNeighborID() const;
    virtual void setYNNeighborID(const EntityItemID& yNNeighborID);
    void setYNNeighborID(const QString& yNNeighborID);
    EntityItemID getYNNeighborID() const;
    virtual void setZNNeighborID(const EntityItemID& zNNeighborID);
    void setZNNeighborID(const QString& zNNeighborID);
    EntityItemID getZNNeighborID() const;

    std::array<EntityItemID, 3> getNNeigborIDs() const;
    

    virtual void setXPNeighborID(const EntityItemID& xPNeighborID);
    void setXPNeighborID(const QString& xPNeighborID);
    EntityItemID getXPNeighborID() const;
    virtual void setYPNeighborID(const EntityItemID& yPNeighborID);
    void setYPNeighborID(const QString& yPNeighborID);
    EntityItemID getYPNeighborID() const;
    virtual void setZPNeighborID(const EntityItemID& zPNeighborID);
    void setZPNeighborID(const QString& zPNeighborID);
    EntityItemID getZPNeighborID() const;

    std::array<EntityItemID, 3> getPNeigborIDs() const;

    glm::vec3 getSurfacePositionAdjustment() const;

    virtual ShapeType getShapeType() const override;
    virtual bool shouldBePhysical() const override { return !isDead(); }

    bool isEdged() const;

    glm::mat4 voxelToWorldMatrix() const;
    glm::mat4 worldToVoxelMatrix() const;
    glm::mat4 voxelToLocalMatrix() const;
    glm::mat4 localToVoxelMatrix() const;

 protected:
    void setVoxelDataDirty(bool value) { withWriteLock([&] { _voxelDataDirty = value; }); }

    glm::vec3 _voxelVolumeSize { DEFAULT_VOXEL_VOLUME_SIZE }; // this is always 3 bytes

    QByteArray _voxelData { DEFAULT_VOXEL_DATA };
    bool _voxelDataDirty { true }; // _voxelData has changed, things that depend on it should be updated

    PolyVoxSurfaceStyle _voxelSurfaceStyle { DEFAULT_VOXEL_SURFACE_STYLE };

    QString _xTextureURL { DEFAULT_X_TEXTURE_URL };
    QString _yTextureURL { DEFAULT_Y_TEXTURE_URL };
    QString _zTextureURL { DEFAULT_Z_TEXTURE_URL };

    // for non-edged surface styles, these are used to compute the high-axis edges
    EntityItemID _xNNeighborID{UNKNOWN_ENTITY_ID};
    EntityItemID _yNNeighborID{UNKNOWN_ENTITY_ID};
    EntityItemID _zNNeighborID{UNKNOWN_ENTITY_ID};

    EntityItemID _xPNeighborID{UNKNOWN_ENTITY_ID};
    EntityItemID _yPNeighborID{UNKNOWN_ENTITY_ID};
    EntityItemID _zPNeighborID{UNKNOWN_ENTITY_ID};
};

#endif // hifi_PolyVoxEntityItem_h
