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

class Referential {
public:
    enum Type {
        MODEL,
        JOINT,
        AVATAR
    };
    
    Referential(const unsigned char*& sourceBuffer, AvatarData* avatar);
    virtual ~Referential();
    
    Type type() { return _type; }
    quint64 createdAt() { return _createdAt; }
    bool isValid() { return _isValid; }
    bool hasExtraData() { return !_extraDataBuffer.isEmpty(); }
    
    virtual void update() {}
    int packReferential(unsigned char* destinationBuffer);
    int unpackReferential(const unsigned char* sourceBuffer);
    
protected:
    Referential(Type type, AvatarData* avatar);
    
    // packs the base class data
    int pack(unsigned char* destinationBuffer);
    int unpack(const unsigned char* sourceBuffer);
    // virtual functions that pack fthe extra data of subclasses (needs to be reimplemented in subclass)
    virtual int packExtraData(unsigned char* destinationBuffer) { return 0; }
    virtual int unpackExtraData(const unsigned char* sourceBuffer);
    
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