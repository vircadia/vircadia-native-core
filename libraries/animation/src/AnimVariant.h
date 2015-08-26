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
#include <map>

class AnimVariant {
public:
    enum Type {
        BoolType = 0,
        IntType,
        FloatType,
        Vec3Type,
        QuatType,
        Mat4Type,
        NumTypes
    };

    AnimVariant() : _type(BoolType) { memset(&_val, 0, sizeof(_val)); }
    AnimVariant(bool value) : _type(BoolType) { _val.boolVal = value; }
    AnimVariant(int value) : _type(IntType) { _val.intVal = value; }
    AnimVariant(float value) : _type(FloatType) { _val.floats[0] = value; }
    AnimVariant(const glm::vec3& value) : _type(Vec3Type) { *reinterpret_cast<glm::vec3*>(&_val) = value; }
    AnimVariant(const glm::quat& value) : _type(QuatType) { *reinterpret_cast<glm::quat*>(&_val) = value; }
    AnimVariant(const glm::mat4& value) : _type(Mat4Type) { *reinterpret_cast<glm::mat4*>(&_val) = value; }

    bool isBool() const { return _type == BoolType; }
    bool isInt() const { return _type == IntType; }
    bool isFloat() const { return _type == FloatType; }
    bool isVec3() const { return _type == Vec3Type; }
    bool isQuat() const { return _type == QuatType; }
    bool isMat4() const { return _type == Mat4Type; }

    void setBool(bool value) { assert(_type == BoolType); _val.boolVal = value; }
    void setInt(int value) { assert(_type == IntType); _val.intVal = value; }
    void setFloat(float value) { assert(_type == FloatType); _val.floats[0] = value; }
    void setVec3(const glm::vec3& value) { assert(_type == Vec3Type); *reinterpret_cast<glm::vec3*>(&_val) = value; }
    void setQuat(const glm::quat& value) { assert(_type == QuatType); *reinterpret_cast<glm::quat*>(&_val) = value; }
    void setMat4(const glm::mat4& value) { assert(_type == Mat4Type); *reinterpret_cast<glm::mat4*>(&_val) = value; }

    bool getBool() const { assert(_type == BoolType); return _val.boolVal; }
    int getInt() const { assert(_type == IntType); return _val.intVal; }
    float getFloat() const { assert(_type == FloatType); return _val.floats[0]; }
    const glm::vec3& getVec3() const { assert(_type == Vec3Type); return *reinterpret_cast<const glm::vec3*>(&_val); }
    const glm::quat& getQuat() const { assert(_type == QuatType); return *reinterpret_cast<const glm::quat*>(&_val); }
    const glm::mat4& getMat4() const { assert(_type == Mat4Type); return *reinterpret_cast<const glm::mat4*>(&_val); }

protected:
    Type _type;
    union {
        bool boolVal;
        int intVal;
        float floats[16];
    } _val;
};

class AnimVariantMap {
public:

    bool lookup(const std::string& key, bool defaultValue) const {
        if (key.empty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getBool() : defaultValue;
        }
    }

    int lookup(const std::string& key, int defaultValue) const {
        if (key.empty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getInt() : defaultValue;
        }
    }

    float lookup(const std::string& key, float defaultValue) const {
        if (key.empty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getFloat() : defaultValue;
        }
    }

    const glm::vec3& lookup(const std::string& key, const glm::vec3& defaultValue) const {
        if (key.empty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getVec3() : defaultValue;
        }
    }

    const glm::quat& lookup(const std::string& key, const glm::quat& defaultValue) const {
        if (key.empty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getQuat() : defaultValue;
        }
    }

    const glm::mat4& lookup(const std::string& key, const glm::mat4& defaultValue) const {
        if (key.empty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getMat4() : defaultValue;
        }
    }

    void set(const std::string& key, bool value) { _map[key] = AnimVariant(value); }
    void set(const std::string& key, int value) { _map[key] = AnimVariant(value); }
    void set(const std::string& key, float value) { _map[key] = AnimVariant(value); }
    void set(const std::string& key, const glm::vec3& value) { _map[key] = AnimVariant(value); }
    void set(const std::string& key, const glm::quat& value) { _map[key] = AnimVariant(value); }
    void set(const std::string& key, const glm::mat4& value) { _map[key] = AnimVariant(value); }

protected:
    std::map<std::string, AnimVariant> _map;
};

#endif // hifi_AnimVariant_h
