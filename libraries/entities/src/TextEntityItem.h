//
//  TextEntityItem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextEntityItem_h
#define hifi_TextEntityItem_h

#include "EntityItem.h"

class TextEntityItem : public EntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    TextEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    /// set dimensions in domain scale units (0.0 - 1.0) this will also reset radius appropriately
    virtual void setDimensions(const glm::vec3& value) override;
    virtual ShapeType getShapeType() const override { return SHAPE_TYPE_BOX; }

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

    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElementPointer& element, float& distance,
                         BoxFace& face, glm::vec3& surfaceNormal,
                         void** intersectedObject, bool precisionPicking) const override;

    static const QString DEFAULT_TEXT;
    void setText(const QString& value);
    QString getText() const;

    static const float DEFAULT_LINE_HEIGHT;
    void setLineHeight(float value);
    float getLineHeight() const;

    static const xColor DEFAULT_TEXT_COLOR;
    // FIXME should not return a reference because of thread safety, but can't return an array
    const rgbColor& getTextColor() const;
    xColor getTextColorX() const;

    void setTextColor(const rgbColor& value);
    void setTextColor(const xColor& value);

    static const xColor DEFAULT_BACKGROUND_COLOR;
    // FIXME should not return a reference because of thread safety, but can't return an array
    const rgbColor& getBackgroundColor() const;
    xColor getBackgroundColorX() const;

    void setBackgroundColor(const rgbColor& value);
    void setBackgroundColor(const xColor& value);

    static const bool DEFAULT_FACE_CAMERA;
    bool getFaceCamera() const;
    void setFaceCamera(bool value);

private:
    QString _text;
    float _lineHeight;
    rgbColor _textColor;
    rgbColor _backgroundColor;
    bool _faceCamera;
};

#endif // hifi_TextEntityItem_h
