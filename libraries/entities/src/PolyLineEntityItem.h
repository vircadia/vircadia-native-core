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
    virtual EntityItemProperties getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const override;
    virtual bool setSubClassProperties(const EntityItemProperties& properties) override;

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

    glm::u8vec3 getColor() const;
    void setColor(const glm::u8vec3& value);

    static const int MAX_POINTS_PER_LINE;
    void setLinePoints(const QVector<glm::vec3>& points);
    QVector<glm::vec3> getLinePoints() const;

    static const float DEFAULT_LINE_WIDTH;
    void setStrokeWidths(const QVector<float>& strokeWidths);
    QVector<float> getStrokeWidths() const;

    void setNormals(const QVector<glm::vec3>& normals);
    QVector<glm::vec3> getNormals() const;

    void setStrokeColors(const QVector<glm::vec3>& strokeColors);
    QVector<glm::vec3> getStrokeColors() const;

    void setIsUVModeStretch(bool isUVModeStretch);
    bool getIsUVModeStretch() const{ return _isUVModeStretch; }

    QString getTextures() const;
    void setTextures(const QString& textures);

    void setGlow(bool glow);
    bool getGlow() const { return _glow; }

    void setFaceCamera(bool faceCamera);
    bool getFaceCamera() const { return _faceCamera; }

    bool pointsChanged() const { return _pointsChanged; } 
    bool normalsChanged() const { return _normalsChanged; }
    bool colorsChanged() const { return _colorsChanged; }
    bool widthsChanged() const { return _widthsChanged; }
    bool texturesChanged() const { return _texturesChanged; }

    void resetTexturesChanged() { _texturesChanged = false; }
    void resetPolyLineChanged() { _colorsChanged = _widthsChanged = _normalsChanged = _pointsChanged = false; }

    // never have a ray intersection pick a PolyLineEntityItem.
    virtual bool supportsDetailedIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                             const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal,
                                             QVariantMap& extraInfo, bool precisionPicking) const override { return false; }
    virtual bool findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                                                  const glm::vec3& acceleration, const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                                                  float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal,
                                                  QVariantMap& extraInfo, bool precisionPicking) const override { return false; }

    void computeTightLocalBoundingBox(AABox& box) const;

    virtual void debugDump() const override;
private:
    void computeAndUpdateDimensions();

 protected:
    glm::u8vec3 _color;
    QVector<glm::vec3> _points;
    QVector<glm::vec3> _normals;
    QVector<glm::vec3> _colors;
    QVector<float> _widths;
    QString _textures;
    bool _isUVModeStretch;
    bool _glow;
    bool _faceCamera;

    bool _pointsChanged { false };
    bool _normalsChanged { false };
    bool _colorsChanged { false };
    bool _widthsChanged { false };
    bool _texturesChanged { false };
};

#endif // hifi_PolyLineEntityItem_h
