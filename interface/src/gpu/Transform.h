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
    Transform(const Mat4& raw);
    ~Transform() {}

    void setTranslation(const Vec3& translation) { invalidCache(); flagTranslation(); _translation = translation; }
    const Vec3& getTranslation() const { return _translation; }

    void preTranslate(const Vec3& translation) { invalidCache(); _translation += translation; }
    void postTranslate( Vec3 const & translation);

    void setRotation(const Quat& rotation) { invalidCache(); flagRotation(); _rotation = glm::normalize(rotation); }
    const Quat& getRotation() const { return _rotation; }

    void setNoScale() { invalidCache(); flagNoScaling(); _scale = Vec3(1.f); }
    void setScale(float scale) { invalidCache(); flagUniformScaling(); _scale = Vec3(scale); }
    void setScale(const Vec3& scale) { invalidCache(); flagNonUniformScaling(); _scale = scale; }
    const Vec3& getScale() const { return _scale; }

    const Mat4& getMatrix() const { updateCache(); return _matrix; }

    Mat4& evalRelativeTransform(Mat4& result, const Vec3& origin);

    Transform& evalInverseTranspose(Transform& result);
    void evalFromRawMatrix(const Mat4& matrix);
    void evalRotationScale(const Mat3& rotationScalematrix);

    static Transform& mult( Transform& result, const Transform& left, const Transform& right);

    bool isScaling() const { return _flags[FLAG_UNIFORM_SCALING] || _flags[FLAG_NON_UNIFORM_SCALING]; }
    bool isUniform() const { return !isNonUniform(); }
    bool isNonUniform() const { return _flags[FLAG_NON_UNIFORM_SCALING]; }

protected:

    enum Flag {
        FLAG_CACHE_INVALID = 0,

        FLAG_TRANSLATION,
        FLAG_ROTATION,
        FLAG_UNIFORM_SCALING,
        FLAG_NON_UNIFORM_SCALING,
        FLAG_SHEARING,

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
   
    void flagNoScaling() { _flags.set(FLAG_UNIFORM_SCALING, false); _flags.set(FLAG_NON_UNIFORM_SCALING, false); }
    void flagUniformScaling() { _flags.set(FLAG_UNIFORM_SCALING, true); _flags.set(FLAG_NON_UNIFORM_SCALING, false); }
    void flagNonUniformScaling() { _flags.set(FLAG_UNIFORM_SCALING, false); _flags.set(FLAG_NON_UNIFORM_SCALING, true); }

    void updateCache() const;
};

typedef QSharedPointer< Transform > TransformPointer;
typedef std::vector< TransformPointer > Transforms;

};


#endif
