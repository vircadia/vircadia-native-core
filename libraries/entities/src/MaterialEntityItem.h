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

    float getBlendFactor() const { return _blendFactor; }
    void setBlendFactor(float blendFactor);

    quint16 getPriority() const { return _priority; }
    void setPriority(quint16 priority);

    quint16 getShapeID() const { return _shapeID; }
    void setShapeID(quint16 shapeID);

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
    QString _materialURL;
    MaterialMode _materialMode { UV };
    float _blendFactor { 1.0f };
    quint16 _priority { 0 };
    quint16 _shapeID { 0 };
    glm::vec2 _materialPos { 0, 0 };
    glm::vec2 _materialScale { 1, 1 };
    float _materialRot { 0 };
    
    QHash<QString, std::shared_ptr<NetworkMaterial>> _materials;
    std::vector<QString> _materialNames;
    QString _currentMaterialName;

    bool _retryApply { false };

    void parseJSONMaterial(const QJsonObject& materialJSON);
    static bool parseJSONColor(const QJsonValue& array, glm::vec3& color, bool& isSRGB);

};

#endif // hifi_MaterialEntityItem_h
