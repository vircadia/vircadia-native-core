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

Transform::Mat4& Transform::evalRelativeTransform( Mat4& result, const Vec3& origin) {
    updateCache();
    result = _matrix;
    result[3] = Vec4(_translation - origin, 1.f);
    return result;
}

void Transform::evalRotationScale(const Mat3& rotationScaleMatrix) {
    Vec3 scale(glm::length(rotationScaleMatrix[0]), glm::length(rotationScaleMatrix[1]), glm::length(rotationScaleMatrix[2]));
    if (scale.x < 0.00001f) scale.x = 0.00001f;
    if (scale.y < 0.00001f) scale.y = 0.00001f;
    if (scale.z < 0.00001f) scale.z = 0.00001f;

    Mat3 matRotScale(
        rotationScaleMatrix[0] / scale.x,
        rotationScaleMatrix[1] / scale.y,
        rotationScaleMatrix[2] / scale.z);
    setRotation(glm::quat_cast(matRotScale));

    float determinant = glm::determinant(matRotScale);
    if (determinant < 0.f) {
        scale.x = -scale.x;
    }

    setScale(scale);
}

void Transform::evalFromRawMatrix(const Mat4& matrix) {
    if ((matrix[0][3] == 0) && (matrix[1][3] == 0) && (matrix[2][3] == 0) && (matrix[3][3] == 1.f)) {
        setTranslation(Vec3(matrix[3]));

        evalRotationScale(Mat3(matrix));
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


 
