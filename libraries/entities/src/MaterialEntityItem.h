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

    const QString& getMaterialURL() { return _materialURL; }
    void setMaterialURL(const QString& materialURL) { _materialURL = materialURL; }

    MaterialMode getMaterialType() { return _materialMode; }
    void setMaterialMode(MaterialMode mode);

    float getBlendFactor() { return _blendFactor; }
    void setBlendFactor(float blendFactor) { _blendFactor = blendFactor; }

    int getPriority() { return _priority; }
    void setPriority(int priority) { _priority = priority; }

    int getShapeID() { return _shapeID; }
    void setShapeID(int shapeID) { _shapeID = shapeID; }

    const glm::vec4& getMaterialBounds() { return _materialBounds; }
    void setMaterialBounds(const glm::vec4& materialBounds) { _materialBounds = materialBounds; }

protected:
    QString _materialURL;
    MaterialMode _materialMode { UV };
    float _blendFactor { 1.0f };
    int _priority { 0 };
    int _shapeID { 0 };
    glm::vec4 _materialBounds { 0, 0, 1, 1 };
    
    //NetworkMaterial _material;

};

#endif // hifi_MaterialEntityItem_h
