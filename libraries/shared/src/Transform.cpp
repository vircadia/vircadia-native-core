//
//  Transform.cpp
//  shared/src/gpu
//
//  Created by Sam Gateau on 11/4/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Transform.h"


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
    const float ACCURACY_THREASHOLD = 0.00001f;

    // Extract the rotation component - this is done using polar decompostion, where
    // we successively average the matrix with its inverse transpose until there is
    // no/a very small difference between successive averages
    float norm;
    int count = 0;
    Mat3 rotationMat = rotationScaleMatrix;
    do {
        Mat3 nextRotation;
        Mat3 currInvTranspose = 
          glm::inverse(glm::transpose(rotationMat));
        
        // Go through every component in the matrices and find the next matrix
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j <3; j++) {
                nextRotation[j][i] = 0.5f * 
                  (rotationMat[j][i] + currInvTranspose[j][i]);
            }
        }

        norm = 0.0;
        for (int i = 0; i < 3; i++) {
            float n = static_cast<float>(
                         fabs(rotationMat[0][i] - nextRotation[0][i]) +
                         fabs(rotationMat[1][i] - nextRotation[1][i]) +
                         fabs(rotationMat[2][i] - nextRotation[2][i]));
            norm = (norm > n ? norm : n);
        }
        rotationMat = nextRotation;
    } while (count < 100 && norm > ACCURACY_THREASHOLD);


    // extract scale of the matrix as the length of each axis
    Mat3 scaleMat = glm::inverse(rotationMat) * rotationScaleMatrix;

    Vec3 scale2(glm::length(rotationScaleMatrix[0]), glm::length(rotationScaleMatrix[1]), glm::length(rotationScaleMatrix[2]));
    Vec3 scale(scaleMat[0][0], scaleMat[1][1], scaleMat[2][2]);
    if (scale.x < ACCURACY_THREASHOLD) scale.x = ACCURACY_THREASHOLD;
    if (scale.y < ACCURACY_THREASHOLD) scale.y = ACCURACY_THREASHOLD;
    if (scale.z < ACCURACY_THREASHOLD) scale.z = ACCURACY_THREASHOLD;

    
    // Let's work on a local matrix containing rotation only
    Mat3 matRot(
        rotationScaleMatrix[0] / scale.x,
        rotationScaleMatrix[1] / scale.y,
        rotationScaleMatrix[2] / scale.z);

    // Beware!!! needs to detect for the case there is a negative scale
    // Based on the determinant sign we just can flip the scale sign of one component: we choose X axis
    float determinant = glm::determinant(matRot);
    if (determinant < 0.f) {
        scale.x = -scale.x;
       // matRot[0] *= -1.f;
    }

    // Beware: even though the matRot is supposed to be normalized at that point,
    // glm::quat_cast doesn't always return a normalized quaternion...
    setRotation(glm::normalize(glm::quat_cast(matRot)));

    // and assign the scale
    setScale(scale);
}

void Transform::evalFromRawMatrix(const Mat4& matrix) {
    // for now works only in the case of TRS transformation
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


 
