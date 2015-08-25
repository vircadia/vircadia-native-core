//
//  AnimVariant.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimVariant_h
#define hifi_AnimVariant_h

#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class AnimVariant {
public:
    enum Type {
        BoolType = 0,
        FloatType,
        Vec3Type,
        QuatType,
        Mat4Type,
        NumTypes
    };

    AnimVariant() : _type(BoolType) { memset(&_val, 0, sizeof(_val)); }
    AnimVariant(bool value) : _type(BoolType) { _val.boolVal = value; }
    AnimVariant(float value) : _type(FloatType) { _val.floats[0] = value; }
    AnimVariant(const glm::vec3& value) : _type(Vec3Type) { *reinterpret_cast<glm::vec3*>(&_val) = value; }
    AnimVariant(const glm::quat& value) : _type(QuatType) { *reinterpret_cast<glm::quat*>(&_val) = value; }
    AnimVariant(const glm::mat4& value) : _type(Mat4Type) { *reinterpret_cast<glm::mat4*>(&_val) = value; }

    bool isBool() const { return _type == BoolType; }
    bool isFloat() const { return _type == FloatType; }
    bool isVec3() const { return _type == Vec3Type; }
    bool isQuat() const { return _type == QuatType; }
    bool isMat4() const { return _type == Mat4Type; }

    void setBool(bool value) { assert(_type == BoolType); _val.boolVal = value; }
    void setFloat(float value) { assert(_type == FloatType); _val.floats[0] = value; }
    void setVec3(const glm::vec3& value) { assert(_type == Vec3Type); *reinterpret_cast<glm::vec3*>(&_val) = value; }
    void setQuat(const glm::quat& value) { assert(_type == QuatType); *reinterpret_cast<glm::quat*>(&_val) = value; }
    void setMat4(const glm::mat4& value) { assert(_type == Mat4Type); *reinterpret_cast<glm::mat4*>(&_val) = value; }

    bool getBool() { assert(_type == BoolType); return _val.boolVal; }
    float getFloat() { assert(_type == FloatType); return _val.floats[0]; }
    const glm::vec3& getVec3() { assert(_type == Vec3Type); return *reinterpret_cast<glm::vec3*>(&_val); }
    const glm::quat& getQuat() { assert(_type == QuatType); return *reinterpret_cast<glm::quat*>(&_val); }
    const glm::mat4& getMat4() { assert(_type == Mat4Type); return *reinterpret_cast<glm::mat4*>(&_val); }

protected:
    Type _type;
    union {
        bool boolVal;
        float floats[16];
    } _val;
};

typedef std::map<std::string, AnimVariant> AnimVarantMap;

#endif // hifi_AnimVariant_h
