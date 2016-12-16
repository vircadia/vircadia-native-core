//
//  PolyLineEntityItem.h
//  libraries/entities/src
//
//  Created by Eric Levin on 8/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PolyLineEntityItem_h
#define hifi_PolyLineEntityItem_h

#include "EntityItem.h"

class PolyLineEntityItem : public EntityItem {
 public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    PolyLineEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    // TODO: eventually only include properties changed since the params.lastViewFrustumSent time
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

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }

    void setColor(const rgbColor& value) {
        memcpy(_color, value, sizeof(_color));
    }
    void setColor(const xColor& value) {
        _color[RED_INDEX] = value.red;
        _color[GREEN_INDEX] = value.green;
        _color[BLUE_INDEX] = value.blue;
    }

    void setLineWidth(float lineWidth){ _lineWidth = lineWidth; }
    float getLineWidth() const{ return _lineWidth; }

    bool setLinePoints(const QVector<glm::vec3>& points);
    bool appendPoint(const glm::vec3& point);
    const QVector<glm::vec3>& getLinePoints() const{ return _points; }

    bool setNormals(const QVector<glm::vec3>& normals);
    const QVector<glm::vec3>& getNormals() const{ return _normals; }

    bool setStrokeWidths(const QVector<float>& strokeWidths);
    const QVector<float>& getStrokeWidths() const{ return _strokeWidths; }

    const QString& getTextures() const { return _textures; }
    void setTextures(const QString& textures) {
        if (_textures != textures) {
            _textures = textures;
            _texturesChangedFlag = true;
        }
    }

    virtual bool needsToCallUpdate() const override { return true; }

    virtual ShapeType getShapeType() const override { return SHAPE_TYPE_NONE; }

    // never have a ray intersection pick a PolyLineEntityItem.
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                             bool& keepSearching, OctreeElementPointer& element, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal,
                                             void** intersectedObject, bool precisionPicking) const override { return false; }

    virtual void debugDump() const override;
    static const float DEFAULT_LINE_WIDTH;
    static const int MAX_POINTS_PER_LINE;

 protected:
    rgbColor _color;
    float _lineWidth;
    bool _pointsChanged;
    bool _normalsChanged;
    bool _strokeWidthsChanged;
    QVector<glm::vec3> _points;
    QVector<glm::vec3> _normals;
    QVector<float> _strokeWidths;
    QString _textures;
    bool _texturesChangedFlag { false };
    mutable QReadWriteLock _quadReadWriteLock;
};

#endif // hifi_PolyLineEntityItem_h
