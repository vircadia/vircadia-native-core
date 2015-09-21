//
//  AnimVariant.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
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
#include <set>

class AnimVariant {
public:
    enum class Type {
        Bool = 0,
        Int,
        Float,
        Vec3,
        Quat,
        Mat4,
        String,
        NumTypes
    };

    AnimVariant() : _type(Type::Bool) { memset(&_val, 0, sizeof(_val)); }
    AnimVariant(bool value) : _type(Type::Bool) { _val.boolVal = value; }
    AnimVariant(int value) : _type(Type::Int) { _val.intVal = value; }
    AnimVariant(float value) : _type(Type::Float) { _val.floats[0] = value; }
    AnimVariant(const glm::vec3& value) : _type(Type::Vec3) { *reinterpret_cast<glm::vec3*>(&_val) = value; }
    AnimVariant(const glm::quat& value) : _type(Type::Quat) { *reinterpret_cast<glm::quat*>(&_val) = value; }
    AnimVariant(const glm::mat4& value) : _type(Type::Mat4) { *reinterpret_cast<glm::mat4*>(&_val) = value; }
    AnimVariant(const QString& value) : _type(Type::String) { _stringVal = value; }

    bool isBool() const { return _type == Type::Bool; }
    bool isInt() const { return _type == Type::Int; }
    bool isFloat() const { return _type == Type::Float; }
    bool isVec3() const { return _type == Type::Vec3; }
    bool isQuat() const { return _type == Type::Quat; }
    bool isMat4() const { return _type == Type::Mat4; }
    bool isString() const { return _type == Type::String; }

    void setBool(bool value) { assert(_type == Type::Bool); _val.boolVal = value; }
    void setInt(int value) { assert(_type == Type::Int); _val.intVal = value; }
    void setFloat(float value) { assert(_type == Type::Float); _val.floats[0] = value; }
    void setVec3(const glm::vec3& value) { assert(_type == Type::Vec3); *reinterpret_cast<glm::vec3*>(&_val) = value; }
    void setQuat(const glm::quat& value) { assert(_type == Type::Quat); *reinterpret_cast<glm::quat*>(&_val) = value; }
    void setMat4(const glm::mat4& value) { assert(_type == Type::Mat4); *reinterpret_cast<glm::mat4*>(&_val) = value; }
    void setString(const QString& value) { assert(_type == Type::String); _stringVal = value; }

    bool getBool() const { assert(_type == Type::Bool); return _val.boolVal; }
    int getInt() const { assert(_type == Type::Int); return _val.intVal; }
    float getFloat() const { assert(_type == Type::Float); return _val.floats[0]; }
    const glm::vec3& getVec3() const { assert(_type == Type::Vec3); return *reinterpret_cast<const glm::vec3*>(&_val); }
    const glm::quat& getQuat() const { assert(_type == Type::Quat); return *reinterpret_cast<const glm::quat*>(&_val); }
    const glm::mat4& getMat4() const { assert(_type == Type::Mat4); return *reinterpret_cast<const glm::mat4*>(&_val); }
    const QString& getString() const { assert(_type == Type::String); return _stringVal; }

protected:
    Type _type;
    QString _stringVal;
    union {
        bool boolVal;
        int intVal;
        float floats[16];
    } _val;
};

class AnimVariantMap {
public:

    bool lookup(const QString& key, bool defaultValue) const {
        // check triggers first, then map
        if (key.isEmpty()) {
            return defaultValue;
        } else if (_triggers.find(key) != _triggers.end()) {
            return true;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getBool() : defaultValue;
        }
    }

    int lookup(const QString& key, int defaultValue) const {
        if (key.isEmpty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getInt() : defaultValue;
        }
    }

    float lookup(const QString& key, float defaultValue) const {
        if (key.isEmpty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getFloat() : defaultValue;
        }
    }

    const glm::vec3& lookup(const QString& key, const glm::vec3& defaultValue) const {
        if (key.isEmpty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getVec3() : defaultValue;
        }
    }

    const glm::quat& lookup(const QString& key, const glm::quat& defaultValue) const {
        if (key.isEmpty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getQuat() : defaultValue;
        }
    }

    const glm::mat4& lookup(const QString& key, const glm::mat4& defaultValue) const {
        if (key.isEmpty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getMat4() : defaultValue;
        }
    }

    const QString& lookup(const QString& key, const QString& defaultValue) const {
        if (key.isEmpty()) {
            return defaultValue;
        } else {
            auto iter = _map.find(key);
            return iter != _map.end() ? iter->second.getString() : defaultValue;
        }
    }

    void set(const QString& key, bool value) { _map[key] = AnimVariant(value); }
    void set(const QString& key, int value) { _map[key] = AnimVariant(value); }
    void set(const QString& key, float value) { _map[key] = AnimVariant(value); }
    void set(const QString& key, const glm::vec3& value) { _map[key] = AnimVariant(value); }
    void set(const QString& key, const glm::quat& value) { _map[key] = AnimVariant(value); }
    void set(const QString& key, const glm::mat4& value) { _map[key] = AnimVariant(value); }
    void set(const QString& key, const QString& value) { _map[key] = AnimVariant(value); }
    void unset(const QString& key) { _map.erase(key); }

    void setTrigger(const QString& key) { _triggers.insert(key); }
    void clearTriggers() { _triggers.clear(); }

    bool hasKey(const QString& key) const { return _map.find(key) != _map.end(); }

protected:
    std::map<QString, AnimVariant> _map;
    std::set<QString> _triggers;
};

#endif // hifi_AnimVariant_h
