//
//  Transform.h
//  interface/src/gpu
//
//  Created by Sam Gateau on 11/4/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Transform_h
#define hifi_gpu_Transform_h

#include <assert.h>
#include "InterfaceConfig.h"

#include "gpu/Format.h"


#include <QSharedPointer>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <bitset>

namespace gpu {

class Transform {
public:
    typedef glm::mat4 Mat4;
    typedef glm::mat3 Mat3;
    typedef glm::vec4 Vec4;
    typedef glm::vec3 Vec3;
    typedef glm::vec2 Vec2;
    typedef glm::quat Quat;

    Transform();
    Transform(const Transform& transform) :
        _translation(transform._translation),
        _rotation(transform._rotation),
        _scale(transform._scale),
        _flags(transform._flags)
    {
        invalidCache();
    }
    Transform(const Mat4& raw);
    ~Transform() {}

    void setTranslation(const Vec3& translation) { invalidCache(); flagTranslation(); _translation = translation; }
    const Vec3& getTranslation() const { return _translation; }

    void preTranslate(const Vec3& translation) { invalidCache(); flagTranslation(); _translation += translation; }
    void postTranslate(const Vec3& translation) {
        invalidCache();
        flagTranslation();
        if (isRotating()) {
            _translation += glm::rotate(_rotation, translation * _scale);
        } else {
            _translation += translation * _scale;
        }
    }

    void setRotation(const Quat& rotation) { invalidCache(); flagRotation(); _rotation = rotation; }
    const Quat& getRotation() const { return _rotation; }

    void preRotate(const Quat& rotation) {
        invalidCache();
        if (isRotating()) {
            _rotation = rotation * _rotation;
        } else {
            _rotation = rotation;
        }
        flagRotation();
        _translation = glm::rotate(rotation, _translation);
    }
    void postRotate(const Quat& rotation) {
        invalidCache();
        if (isRotating()) {
            _rotation *= rotation;
        } else {
            _rotation = rotation;
        }
         flagRotation();
    }

    void setScale(float scale) { invalidCache(); flagScaling(); _scale = Vec3(scale); }
    void setScale(const Vec3& scale) { invalidCache(); flagScaling(); _scale = scale; }
    const Vec3& getScale() const { return _scale; }

    void postScale(const Vec3& scale) {
        invalidCache();
        if (isScaling()) {
            _scale *= scale;
        } else {
            _scale = scale;
        }
        flagScaling();
    }

    const Mat4& getMatrix() const { updateCache(); return _matrix; }

    Mat4& evalRelativeTransform(Mat4& result, const Vec3& origin);

    Transform& evalInverseTranspose(Transform& result);
    void evalFromRawMatrix(const Mat4& matrix);
    void evalRotationScale(const Mat3& rotationScalematrix);

    static Transform& mult( Transform& result, const Transform& left, const Transform& right) {
        result = left;
        if ( right.isTranslating()) result.postTranslate(right.getTranslation());
        if ( right.isRotating()) result.postRotate(right.getRotation());
        if (right.isScaling()) result.postScale(right.getScale());
        return result;
    }

    bool isTranslating() const { return _flags[FLAG_TRANSLATION]; }
    bool isRotating() const { return _flags[FLAG_ROTATION]; }
    bool isScaling() const { return _flags[FLAG_SCALING]; }

protected:

    enum Flag {
        FLAG_CACHE_INVALID = 0,

        FLAG_TRANSLATION,
        FLAG_ROTATION,
        FLAG_SCALING,

        FLAG_PROJECTION,

        NUM_FLAGS,
    };

    typedef std::bitset<NUM_FLAGS> Flags;


    // TRS
    Vec3 _translation;
    Quat _rotation;
    Vec3 _scale;

    mutable Flags _flags;

    // Cached transform
    mutable Mat4 _matrix;
    bool isCacheInvalid() const { return _flags[FLAG_CACHE_INVALID]; }
    void validCache() const { _flags.set(FLAG_CACHE_INVALID, false); }
    void invalidCache() const { _flags.set(FLAG_CACHE_INVALID, true); }

    void flagTranslation() { _flags.set(FLAG_TRANSLATION, true); }
    void flagRotation() { _flags.set(FLAG_ROTATION, true); }
   
    void flagScaling() { _flags.set(FLAG_SCALING, true); }

    void updateCache() const {
        if (isCacheInvalid()) {
            if (isRotating()) {
                glm::mat3x3 rot = glm::mat3_cast(_rotation);

                if ((_scale.x != 1.f) || (_scale.y != 1.f) || (_scale.z != 1.f)) {
                    rot[0] *= _scale.x;
                    rot[1] *= _scale.y;
                    rot[2] *= _scale.z;
                }

                _matrix[0] = Vec4(rot[0], 0.f);
                _matrix[1] = Vec4(rot[1], 0.f);
                _matrix[2] = Vec4(rot[2], 0.f);
            } else {
                _matrix[0] = Vec4(_scale.x, 0.f, 0.f, 0.f);
                _matrix[1] = Vec4(0.f, _scale.y, 0.f, 0.f);
                _matrix[2] = Vec4(0.f, 0.f, _scale.z, 0.f);
            }

            _matrix[3] = Vec4(_translation, 1.f);
            validCache();
        }
    }
};

typedef QSharedPointer< Transform > TransformPointer;
typedef std::vector< TransformPointer > Transforms;

};


#endif
