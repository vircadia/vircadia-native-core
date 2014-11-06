//
//  Transform.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 11/4/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Transform.h"

#include <glm/gtx/quaternion.hpp>

using namespace gpu;

Transform::Transform() :
    _translation(0),
    _rotation(1.f, 0, 0, 0),
    _scale(1.f),
    _flags(1) // invalid cache
{
}
Transform::Transform(const Mat4& raw) {
    evalFromRawMatrix(raw);
}

void Transform::updateCache() const {
    if (isCacheInvalid()) {
        glm::mat3x3 rot = glm::mat3_cast(_rotation);

        if ((_scale.x != 1.f) || (_scale.y != 1.f) || (_scale.z != 1.f)) {
            rot[0] *= _scale.x;
            rot[1] *= _scale.y;
            rot[2] *= _scale.z;
        }

        _matrix[0] = Vec4(rot[0], 0.f);
        _matrix[1] = Vec4(rot[1], 0.f);
        _matrix[2] = Vec4(rot[2], 0.f);

        _matrix[3] = Vec4(_translation, 0.f);

        validCache();
    }
}

void Transform::localTranslate( Vec3 const & translation) {
    Vec3 tt = glm::rotate(_rotation, translation);
    _translation += tt * _scale;
}

Transform::Mat4& Transform::evalRelativeTransform( Mat4& result, const Vec3& origin) {
    updateCache();
    result = _matrix;
    result[3] = Vec4(_translation - origin, 0.f);
    return result;
}

void Transform::evalFromRawMatrix(const Mat4& matrix) {
    if ((matrix[0][3] == 0) && (matrix[1][3] == 0) && (matrix[2][3] == 0) && (matrix[3][3] == 1.f)) {
        setTranslation(Vec3(matrix[3]));

        Mat3 matRS = Mat3(matrix);

        Vec3 scale(glm::length(matRS[0]), glm::length(matRS[1]), glm::length(matRS[2]));

        matRS[0] *= 1/scale.x;
        matRS[1] *= 1/scale.y;
        matRS[2] *= 1/scale.z;

        setRotation(glm::quat_cast(matRS));
        setScale(scale);
    }
}

Transform& Transform::evalInverseTranspose(Transform& result) {
    result.setTranslation(-_translation);
    result.setRotation(-_rotation);
    
    if (isScaling()) {
        result.setScale(Vec3(1.f/_scale.x, 1.f/_scale.y, 1.f/_scale.z));
    }
    return result;
}

Transform& Transform::mult( Transform& result, const Transform& left, const Transform& right) {
    right.updateCache();
    left.updateCache();

    result.setTranslation(Vec3(left.getMatrix() * Vec4(right.getTranslation(), 1.f)));
    
    Mat4 mat = left.getMatrix() * right.getMatrix();
    Mat3 matRS = Mat3(mat);

    Vec3 scale(glm::length(matRS[0]), glm::length(matRS[1]), glm::length(matRS[2]));

    matRS[0] *= 1/scale.x;
    matRS[1] *= 1/scale.y;
    matRS[2] *= 1/scale.z;

    result.setRotation(glm::quat_cast(matRS));
    result.setScale(scale);

    return result;
}

 
