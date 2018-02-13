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

#include "MaterialMode.h"
#include <model-networking/ModelCache.h>
#include <model-networking/MaterialCache.h>

class MaterialEntityItem : public EntityItem {
    using Pointer = std::shared_ptr<MaterialEntityItem>;
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    MaterialEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    void update(const quint64& now) override;
    bool needsToCallUpdate() const override { return true; }

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    // TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
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
    void setMaterialURL(const QString& materialURLString, bool userDataChanged = false);

    QString getCurrentMaterialName() const { return _currentMaterialName; }
    void setCurrentMaterialName(const QString& currentMaterialName);

    MaterialMode getMaterialMode() const { return _materialMode; }
    void setMaterialMode(MaterialMode mode) { _materialMode = mode; }

    quint16 getPriority() const { return _priority; }
    void setPriority(quint16 priority);

    QString getParentMaterialID() const { return _parentMaterialID; }
    void setParentMaterialID(const QString& parentMaterialID);

    glm::vec2 getMaterialPos() const { return _materialPos; }
    void setMaterialPos(const glm::vec2& materialPos);
    glm::vec2 getMaterialScale() const { return _materialScale; }
    void setMaterialScale(const glm::vec2& materialScale);
    float getMaterialRot() const { return _materialRot; }
    void setMaterialRot(const float& materialRot);

    std::shared_ptr<NetworkMaterial> getMaterial() const;

    void setUserData(const QString& userData) override;
    void setParentID(const QUuid& parentID) override;
    void setClientOnly(bool clientOnly) override;
    void setOwningAvatarID(const QUuid& owningAvatarID) override;

    void applyMaterial();
    void removeMaterial();

    void postAdd() override;
    void preDelete() override;
    void postParentFixup() override;

private:
    // URL for this material.  Currently, only JSON format is supported.  Set to "userData" to use the user data to live edit a material.
    // The following fields are supported in the JSON:
    // strings:
    //   name (NOT YET USED)
    // floats:
    //   opacity, roughness, metallic, scattering
    // colors (arrays of 3 floats 0-1.  Optional fourth value in array can be a boolean isSRGB):
    //   emissive, albedo, fresnel
    // urls to textures:
    //   emissiveMap, albedoMap, metallicMap, roughnessMap, normalMap, occlusionMap, lightmapMap, scatteringMap
    QString _materialURL;
    // Type of material.  "uv" or "projected".  NOT YET IMPLEMENTED, only UV is used
    MaterialMode _materialMode { UV };
    // Priority for this material when applying it to its parent.  Only the highest priority material will be used.  Materials with the same priority are (essentially) randomly sorted.
    // Base materials that come with models always have priority 0.
    quint16 _priority { 0 };
    // An identifier for choosing a submesh or submeshes within a parent.  If in the format "mat::<string>", all submeshes with material name "<string>" will be replaced.  Otherwise,
    // parentMaterialID will be parsed as an unsigned int (strings not starting with "mat::" will parse to 0), representing the mesh index to modify.
    QString _parentMaterialID { "0" };
    // Offset position in UV-space of top left of material, (0, 0) to (1, 1)
    glm::vec2 _materialPos { 0, 0 };
    // How much to scale this material within its parent's UV-space
    glm::vec2 _materialScale { 1, 1 };
    // How much to rotate this material within its parent's UV-space (degrees)
    float _materialRot { 0 };

    NetworkMaterialResourcePointer _networkMaterial;
    QHash<QString, std::shared_ptr<NetworkMaterial>> _materials;
    std::vector<QString> _materialNames;
    QString _currentMaterialName;

    bool _retryApply { false };
    bool _hasBeenAddedToOctree { false };

};

#endif // hifi_MaterialEntityItem_h
