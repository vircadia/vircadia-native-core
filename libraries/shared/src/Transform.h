//
//  Transform.h
//  shared/src/gpu
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

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <bitset>

class Transform {
public:
    typedef glm::mat4 Mat4;
    typedef glm::mat3 Mat3;
    typedef glm::vec4 Vec4;
    typedef glm::vec3 Vec3;
    typedef glm::vec2 Vec2;
    typedef glm::quat Quat;

    Transform() :
        _translation(0),
        _rotation(1.0f, 0, 0, 0),
        _scale(1.0f),
        _flags(FLAG_CACHE_INVALID_BITSET) // invalid cache
    {
    }
    Transform(const Transform& transform) :
        _translation(transform._translation),
        _rotation(transform._rotation),
        _scale(transform._scale),
        _flags(transform._flags)
    {
        invalidCache();
    }
    Transform(const Mat4& raw) {
        evalFromRawMatrix(raw);
    }
    ~Transform() {}

    void setIdentity();

    const Vec3& getTranslation() const;
    void setTranslation(const Vec3& translation);
    void preTranslate(const Vec3& translation);
    void postTranslate(const Vec3& translation);

    const Quat& getRotation() const;
    void setRotation(const Quat& rotation);
    void preRotate(const Quat& rotation);
    void postRotate(const Quat& rotation);

    const Vec3& getScale() const;
    void setScale(float scale);
    void setScale(const Vec3& scale);
    void postScale(float scale);
    void postScale(const Vec3& scale);

    bool isIdentity() const { return (_flags & ~Flags(FLAG_CACHE_INVALID_BITSET)).none(); }
    bool isTranslating() const { return _flags[FLAG_TRANSLATION]; }
    bool isRotating() const { return _flags[FLAG_ROTATION]; }
    bool isScaling() const { return _flags[FLAG_SCALING]; }
    bool isUniform() const { return !isNonUniform(); }
    bool isNonUniform() const { return _flags[FLAG_NON_UNIFORM]; }

    void evalFromRawMatrix(const Mat4& matrix);
    void evalFromRawMatrix(const Mat3& rotationScalematrix);

    Mat4& getMatrix(Mat4& result) const;
    Mat4& getInverseMatrix(Mat4& result) const;

    Transform& evalInverse(Transform& result) const;

    static void evalRotationScale(Quat& rotation, Vec3& scale, const Mat3& rotationScaleMatrix);

    static Transform& mult(Transform& result, const Transform& left, const Transform& right);

    // Left will be inversed before the multiplication
    static Transform& inverseMult(Transform& result, const Transform& left, const Transform& right);


protected:

    enum Flag {
        FLAG_CACHE_INVALID = 0,

        FLAG_TRANSLATION,
        FLAG_ROTATION,
        FLAG_SCALING,
        FLAG_NON_UNIFORM,
        FLAG_ZERO_SCALE,

        FLAG_PROJECTION,

        NUM_FLAGS,

        FLAG_CACHE_INVALID_BITSET = 1,
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
    void unflagScaling() { _flags.set(FLAG_SCALING, false); }


    void flagUniform() { _flags.set(FLAG_NON_UNIFORM, false); }
    void flagNonUniform() { _flags.set(FLAG_NON_UNIFORM, true); }

    void updateCache() const;
};

inline void Transform::setIdentity() {
    _translation = Vec3(0);
    _rotation = Quat(1.0f, 0, 0, 0);
    _scale = Vec3(1.0f);
    _flags = Flags(FLAG_CACHE_INVALID_BITSET);
}

inline const Transform::Vec3& Transform::getTranslation() const {
    return _translation;
}

inline void Transform::setTranslation(const Vec3& translation) {
    invalidCache();
    flagTranslation();
    _translation = translation;
}

inline void Transform::preTranslate(const Vec3& translation) {
    invalidCache();
    flagTranslation();
    _translation += translation;
}

inline void Transform::postTranslate(const Vec3& translation) {
    invalidCache();
    flagTranslation();

    Vec3 scaledT = translation;
    if (isScaling()) scaledT *= _scale;

    if (isRotating()) {
        _translation += glm::rotate(_rotation, scaledT);
    } else {
        _translation += scaledT;
    }
}

inline const Transform::Quat& Transform::getRotation() const {
    return _rotation;
}

inline void Transform::setRotation(const Quat& rotation) {
    invalidCache();
    flagRotation();
    _rotation = rotation;
}

inline void Transform::preRotate(const Quat& rotation) {
    invalidCache();
    if (isRotating()) {
        _rotation = rotation * _rotation;
    } else {
        _rotation = rotation;
    }
    flagRotation();
    _translation = glm::rotate(rotation, _translation);
}

inline void Transform::postRotate(const Quat& rotation) {
    invalidCache();

    if (isNonUniform()) {
        Quat newRot;
        Vec3 newScale;
        Mat3 scaleRot(glm::mat3_cast(rotation));
        scaleRot[0] *= _scale;
        scaleRot[1] *= _scale;
        scaleRot[2] *= _scale;
        evalRotationScale(newRot, newScale, scaleRot);

        if (isRotating()) {
            _rotation *= newRot;
        } else {
            _rotation = newRot;
        }
        setScale(newScale);
    } else {
        if (isRotating()) {
            _rotation *= rotation;
        } else {
            _rotation = rotation;
        }
    }
    flagRotation();
}

inline const Transform::Vec3& Transform::getScale() const {
    return _scale;
}

inline void Transform::setScale(float scale) {
    invalidCache();
    flagUniform();
    if (scale == 1.0f) {
        unflagScaling();
    } else {
        flagScaling();
    }
    _scale = Vec3(scale);
}

inline void Transform::setScale(const Vec3& scale) {
    if ((scale.x == scale.y) && (scale.x == scale.z)) {
        setScale(scale.x);
    } else {
        invalidCache();
        flagScaling();
        flagNonUniform();
        _scale = scale;
    }
}

inline void Transform::postScale(float scale) {
    if (scale == 1.0f) return;
    if (isScaling()) {
        // if already scaling, just invalid cache and aply uniform scale
        invalidCache();
        _scale *= scale;
    } else {
        setScale(scale);
    }
}

inline void Transform::postScale(const Vec3& scale) {
    invalidCache();
    if (isScaling()) {
        _scale *= scale;
    } else {
        _scale = scale;
    }
    flagScaling();
}

inline Transform::Mat4& Transform::getMatrix(Transform::Mat4& result) const {
    updateCache();
    result = _matrix;
    return result;
}

inline Transform::Mat4& Transform::getInverseMatrix(Transform::Mat4& result) const {
    return evalInverse(Transform()).getMatrix(result);
}

inline void Transform::evalFromRawMatrix(const Mat4& matrix) {
    // for now works only in the case of TRS transformation
    if ((matrix[0][3] == 0) && (matrix[1][3] == 0) && (matrix[2][3] == 0) && (matrix[3][3] == 1.f)) {
        setTranslation(Vec3(matrix[3]));
        evalFromRawMatrix(Mat3(matrix));
    }
}

inline void Transform::evalFromRawMatrix(const Mat3& rotationScaleMatrix) {
    Quat rotation;
    Vec3 scale;
    evalRotationScale(rotation, scale, rotationScaleMatrix);
    setRotation(rotation);
    setScale(scale);
}

inline Transform& Transform::evalInverse(Transform& inverse) const {
    inverse.setIdentity();
    if (isScaling()) {
        // TODO: At some point we will face the case when scale is 0 and so 1/0 will blow up...
        // WHat should we do for this one?
        assert(_scale.x != 0);
        assert(_scale.y != 0);
        assert(_scale.z != 0);
        if (isNonUniform()) {
            inverse.setScale(Vec3(1.0f/_scale.x, 1.0f/_scale.y, 1.0f/_scale.z));
        } else {
            inverse.setScale(1.0f/_scale.x);
        }
    }
    if (isRotating()) {
        inverse.postRotate(glm::conjugate(_rotation));
    }
    if (isTranslating()) {
        inverse.postTranslate(-_translation);
    }
    return inverse;
}

inline Transform& Transform::mult( Transform& result, const Transform& left, const Transform& right) {
    result = left;
    if (right.isTranslating()) {
        result.postTranslate(right.getTranslation());
    }
    if (right.isRotating()) {
        result.postRotate(right.getRotation());
    }
    if (right.isScaling()) {
        result.postScale(right.getScale());
    }

    // HACK: In case of an issue in the Transform multiplication results, to make sure this code is
    // working properly uncomment the next 2 lines and compare the results, they should be the same...
    // Transform::Mat4 mv = left.getMatrix() * right.getMatrix();
    // Transform::Mat4 mv2 = result.getMatrix();

    return result;
}

inline Transform& Transform::inverseMult( Transform& result, const Transform& left, const Transform& right) {
    result.setIdentity();

    if (left.isScaling()) {
        const Vec3& s = left.getScale();
        result.setScale(Vec3(1.0f / s.x, 1.0f / s.y, 1.0f / s.z));
    }
    if (left.isRotating()) {
        result.postRotate(glm::conjugate(left.getRotation()));
    }
    if (left.isTranslating() || right.isTranslating()) {
        result.postTranslate(right.getTranslation() - left.getTranslation());
    }
    if (right.isRotating()) {
        result.postRotate(right.getRotation());
    }
    if (right.isScaling()) {
        result.postScale(right.getScale());
    }

    // HACK: In case of an issue in the Transform multiplication results, to make sure this code is
    // working properly uncomment the next 2 lines and compare the results, they should be the same...
    // Transform::Mat4 mv = left.getMatrix() * right.getMatrix();
    // Transform::Mat4 mv2 = result.getMatrix();

    return result;
}

inline void Transform::updateCache() const {
    if (isCacheInvalid()) {
        if (isRotating()) {
            Mat3 rot = glm::mat3_cast(_rotation);

            if (isScaling()) {
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

        _matrix[3] = Vec4(_translation, 1.0f);
        validCache();
    }
}

#endif
