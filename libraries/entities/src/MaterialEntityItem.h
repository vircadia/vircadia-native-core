//
//  Created by Sam Gondelman on 1/12/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MaterialEntityItem_h
#define hifi_MaterialEntityItem_h

#include "EntityItem.h"

#include "MaterialMappingMode.h"
#include <model-networking/ModelCache.h>
#include <model-networking/MaterialCache.h>

class MaterialEntityItem : public EntityItem {
    using Pointer = std::shared_ptr<MaterialEntityItem>;
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    MaterialEntityItem(const EntityItemID& entityItemID);
    ~MaterialEntityItem();

    ALLOW_INSTANTIATION // This class can be instantiated

    void update(const quint64& now) override;
    bool needsToCallUpdate() const override { return true; }

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;


    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                 bool& somethingChanged) override;

    void debugDump() const override;

    virtual void setUnscaledDimensions(const glm::vec3& value) override;

    QString getMaterialURL() const { return _materialURL; }
    void setMaterialURL(const QString& materialURLString, bool materialDataChanged = false);

    void setCurrentMaterialName(const std::string& currentMaterialName);

    MaterialMappingMode getMaterialMappingMode() const { return _materialMappingMode; }
    void setMaterialMappingMode(MaterialMappingMode mode) { _materialMappingMode = mode; }

    quint16 getPriority() const { return _priority; }
    void setPriority(quint16 priority);

    QString getParentMaterialName() const { return _parentMaterialName; }
    void setParentMaterialName(const QString& parentMaterialName);

    glm::vec2 getMaterialMappingPos() const { return _materialMappingPos; }
    void setMaterialMappingPos(const glm::vec2& materialMappingPos);
    glm::vec2 getMaterialMappingScale() const { return _materialMappingScale; }
    void setMaterialMappingScale(const glm::vec2& materialMappingScale);
    float getMaterialMappingRot() const { return _materialMappingRot; }
    void setMaterialMappingRot(const float& materialMappingRot);

    QString getMaterialData() const { return _materialData; }
    void setMaterialData(const QString& materialData);

    std::shared_ptr<NetworkMaterial> getMaterial() const;

    void setParentID(const QUuid& parentID) override;

    void applyMaterial();
    void removeMaterial();

    void postParentFixup() override;

    AACube calculateInitialQueryAACube(bool& success) override;

private:
    // URL for this material.  Currently, only JSON format is supported.  Set to "materialData" to use the material data to live edit a material.
    // The following fields are supported in the JSON:
    // materialVersion: a uint for the version of this network material (currently, only 1 is supported)
    // materials, which is either an object or an array of objects, each with the following properties:
    //   strings:
    //     name (NOT YET USED), model (NOT YET USED, should use "hifi_pbr")
    //   floats:
    //     opacity, roughness, metallic, scattering
    //   bool:
    //     unlit
    //   colors (arrays of 3 floats 0-1.  Optional fourth value in array can be a boolean isSRGB):
    //     emissive, albedo
    //   urls to textures:
    //     emissiveMap, albedoMap (set opacityMap = albedoMap for transparency), metallicMap or specularMap, roughnessMap or glossMap,
    //     normalMap or bumpMap, occlusionMap, lightmapMap (broken, FIXME), scatteringMap (only works if normal mapped)
    QString _materialURL;
    // Type of material.  "uv" or "projected".  NOT YET IMPLEMENTED, only UV is used
    MaterialMappingMode _materialMappingMode { UV };
    // Priority for this material when applying it to its parent.  Only the highest priority material will be used.  Materials with the same priority are (essentially) randomly sorted.
    // Base materials that come with models always have priority 0.
    quint16 _priority { 0 };
    // An identifier for choosing a submesh or submeshes within a parent.  If in the format "mat::<string>", all submeshes with material name "<string>" will be replaced.  Otherwise,
    // parentMaterialName will be parsed as an unsigned int (strings not starting with "mat::" will parse to 0), representing the mesh index to modify.
    QString _parentMaterialName { "0" };
    // Offset position in UV-space of top left of material, (0, 0) to (1, 1)
    glm::vec2 _materialMappingPos { 0, 0 };
    // How much to scale this material within its parent's UV-space
    glm::vec2 _materialMappingScale { 1, 1 };
    // How much to rotate this material within its parent's UV-space (degrees)
    float _materialMappingRot { 0 };
    QString _materialData;

    NetworkMaterialResourcePointer _networkMaterial;
    NetworkMaterialResource::ParsedMaterials _parsedMaterials;
    std::string _currentMaterialName;

    bool _retryApply { false };

};

#endif // hifi_MaterialEntityItem_h
