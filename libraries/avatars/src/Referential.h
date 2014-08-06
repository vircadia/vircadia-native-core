//
//  Referential.h
//
//
//  Created by Clement on 7/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Referential_h
#define hifi_Referential_h

#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

class AvatarData;

/// Stores and enforce the relative position of an avatar to a given referential (ie. model, joint, ...)
class Referential {
public:
    enum Type {
        MODEL,
        JOINT,
        AVATAR
    };
    
    Referential(const unsigned char*& sourceBuffer, AvatarData* avatar);
    virtual ~Referential();
    
    Type type() const { return _type; }
    quint64 createdAt() const { return _createdAt; }
    bool isValid() const { return _isValid; }
    bool hasExtraData() const { return !_extraDataBuffer.isEmpty(); }
    
    glm::vec3 getTranslation() const { return _translation; }
    glm::quat getRotation() const { return _rotation; }
    float getScale() const {return _scale; }
    QByteArray getExtraData() const { return _extraDataBuffer; }
    
    virtual void update() {}
    int packReferential(unsigned char* destinationBuffer) const;
    int unpackReferential(const unsigned char* sourceBuffer);
    
protected:
    Referential(Type type, AvatarData* avatar);
    
    // packs the base class data
    int pack(unsigned char* destinationBuffer) const;
    int unpack(const unsigned char* sourceBuffer);
    // virtual functions that pack fthe extra data of subclasses (needs to be reimplemented in subclass)
    virtual int packExtraData(unsigned char* destinationBuffer) const;
    virtual int unpackExtraData(const unsigned char* sourceBuffer, int size);
    
    Type _type;
    quint64 _createdAt;
    bool _isValid;
    AvatarData* _avatar;
    QByteArray _extraDataBuffer;
    
    glm::vec3 _refPosition;
    glm::quat _refRotation;
    float _refScale;
    
    glm::vec3 _translation;
    glm::quat _rotation;
    float _scale;
};


#endif // hifi_Referential_h