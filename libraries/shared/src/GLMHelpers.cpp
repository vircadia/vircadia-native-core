//
//  GLMHelpers.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-08-07.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLMHelpers.h"

#include "NumericalConstants.h"

//  Safe version of glm::mix; based on the code in Nick Bobick's article,
//  http://www.gamasutra.com/features/19980703/quaternions_01.htm (via Clyde,
//  https://github.com/threerings/clyde/blob/master/src/main/java/com/threerings/math/Quaternion.java)
glm::quat safeMix(const glm::quat& q1, const glm::quat& q2, float proportion) {
    float cosa = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
    float ox = q2.x, oy = q2.y, oz = q2.z, ow = q2.w, s0, s1;
    
    // adjust signs if necessary
    if (cosa < 0.0f) {
        cosa = -cosa;
        ox = -ox;
        oy = -oy;
        oz = -oz;
        ow = -ow;
    }
    
    // calculate coefficients; if the angle is too close to zero, we must fall back
    // to linear interpolation
    if ((1.0f - cosa) > EPSILON) {
        float angle = acosf(cosa), sina = sinf(angle);
        s0 = sinf((1.0f - proportion) * angle) / sina;
        s1 = sinf(proportion * angle) / sina;
        
    } else {
        s0 = 1.0f - proportion;
        s1 = proportion;
    }
    
    return glm::normalize(glm::quat(s0 * q1.w + s1 * ow, s0 * q1.x + s1 * ox, s0 * q1.y + s1 * oy, s0 * q1.z + s1 * oz));
}

// Allows sending of fixed-point numbers: radix 1 makes 15.1 number, radix 8 makes 8.8 number, etc
int packFloatScalarToSignedTwoByteFixed(unsigned char* buffer, float scalar, int radix) {
    int16_t outVal = (int16_t)(scalar * (float)(1 << radix));
    memcpy(buffer, &outVal, sizeof(uint16_t));
    return sizeof(uint16_t);
}

int unpackFloatScalarFromSignedTwoByteFixed(const int16_t* byteFixedPointer, float* destinationPointer, int radix) {
    *destinationPointer = *byteFixedPointer / (float)(1 << radix);
    return sizeof(int16_t);
}

int packFloatVec3ToSignedTwoByteFixed(unsigned char* destBuffer, const glm::vec3& srcVector, int radix) {
    const unsigned char* startPosition = destBuffer;
    destBuffer += packFloatScalarToSignedTwoByteFixed(destBuffer, srcVector.x, radix);
    destBuffer += packFloatScalarToSignedTwoByteFixed(destBuffer, srcVector.y, radix);
    destBuffer += packFloatScalarToSignedTwoByteFixed(destBuffer, srcVector.z, radix);
    return destBuffer - startPosition;
}

int unpackFloatVec3FromSignedTwoByteFixed(const unsigned char* sourceBuffer, glm::vec3& destination, int radix) {
    const unsigned char* startPosition = sourceBuffer;
    sourceBuffer += unpackFloatScalarFromSignedTwoByteFixed((int16_t*) sourceBuffer, &(destination.x), radix);
    sourceBuffer += unpackFloatScalarFromSignedTwoByteFixed((int16_t*) sourceBuffer, &(destination.y), radix);
    sourceBuffer += unpackFloatScalarFromSignedTwoByteFixed((int16_t*) sourceBuffer, &(destination.z), radix);
    return sourceBuffer - startPosition;
}


int packFloatAngleToTwoByte(unsigned char* buffer, float degrees) {
    const float ANGLE_CONVERSION_RATIO = (std::numeric_limits<uint16_t>::max() / 360.0f);
    
    uint16_t angleHolder = floorf((degrees + 180.0f) * ANGLE_CONVERSION_RATIO);
    memcpy(buffer, &angleHolder, sizeof(uint16_t));
    
    return sizeof(uint16_t);
}

int unpackFloatAngleFromTwoByte(const uint16_t* byteAnglePointer, float* destinationPointer) {
    *destinationPointer = (*byteAnglePointer / (float) std::numeric_limits<uint16_t>::max()) * 360.0f - 180.0f;
    return sizeof(uint16_t);
}

int packOrientationQuatToBytes(unsigned char* buffer, const glm::quat& quatInput) {
    glm::quat quatNormalized = glm::normalize(quatInput);
    const float QUAT_PART_CONVERSION_RATIO = (std::numeric_limits<uint16_t>::max() / 2.0f);
    uint16_t quatParts[4];
    quatParts[0] = floorf((quatNormalized.x + 1.0f) * QUAT_PART_CONVERSION_RATIO);
    quatParts[1] = floorf((quatNormalized.y + 1.0f) * QUAT_PART_CONVERSION_RATIO);
    quatParts[2] = floorf((quatNormalized.z + 1.0f) * QUAT_PART_CONVERSION_RATIO);
    quatParts[3] = floorf((quatNormalized.w + 1.0f) * QUAT_PART_CONVERSION_RATIO);
    
    memcpy(buffer, &quatParts, sizeof(quatParts));
    return sizeof(quatParts);
}

int unpackOrientationQuatFromBytes(const unsigned char* buffer, glm::quat& quatOutput) {
    uint16_t quatParts[4];
    memcpy(&quatParts, buffer, sizeof(quatParts));
    
    quatOutput.x = ((quatParts[0] / (float) std::numeric_limits<uint16_t>::max()) * 2.0f) - 1.0f;
    quatOutput.y = ((quatParts[1] / (float) std::numeric_limits<uint16_t>::max()) * 2.0f) - 1.0f;
    quatOutput.z = ((quatParts[2] / (float) std::numeric_limits<uint16_t>::max()) * 2.0f) - 1.0f;
    quatOutput.w = ((quatParts[3] / (float) std::numeric_limits<uint16_t>::max()) * 2.0f) - 1.0f;
    
    return sizeof(quatParts);
}

//  Safe version of glm::eulerAngles; uses the factorization method described in David Eberly's
//  http://www.geometrictools.com/Documentation/EulerAngles.pdf (via Clyde,
// https://github.com/threerings/clyde/blob/master/src/main/java/com/threerings/math/Quaternion.java)
glm::vec3 safeEulerAngles(const glm::quat& q) {
    float sy = 2.0f * (q.y * q.w - q.x * q.z);
    glm::vec3 eulers;
    if (sy < 1.0f - EPSILON) {
        if (sy > -1.0f + EPSILON) {
            eulers = glm::vec3(
                               atan2f(q.y * q.z + q.x * q.w, 0.5f - (q.x * q.x + q.y * q.y)),
                               asinf(sy),
                               atan2f(q.x * q.y + q.z * q.w, 0.5f - (q.y * q.y + q.z * q.z)));
            
        } else {
            // not a unique solution; x + z = atan2(-m21, m11)
            eulers = glm::vec3(
                               0.0f,
                               - PI_OVER_TWO,
                               atan2f(q.x * q.w - q.y * q.z, 0.5f - (q.x * q.x + q.z * q.z)));
        }
    } else {
        // not a unique solution; x - z = atan2(-m21, m11)
        eulers = glm::vec3(
                           0.0f,
                           PI_OVER_TWO,
                           -atan2f(q.x * q.w - q.y * q.z, 0.5f - (q.x * q.x + q.z * q.z)));
    }
    
    // adjust so that z, rather than y, is in [-pi/2, pi/2]
    if (eulers.z < -PI_OVER_TWO) {
        if (eulers.x < 0.0f) {
            eulers.x += PI;
        } else {
            eulers.x -= PI;
        }
        eulers.y = -eulers.y;
        if (eulers.y < 0.0f) {
            eulers.y += PI;
        } else {
            eulers.y -= PI;
        }
        eulers.z += PI;
        
    } else if (eulers.z > PI_OVER_TWO) {
        if (eulers.x < 0.0f) {
            eulers.x += PI;
        } else {
            eulers.x -= PI;
        }
        eulers.y = -eulers.y;
        if (eulers.y < 0.0f) {
            eulers.y += PI;
        } else {
            eulers.y -= PI;
        }
        eulers.z -= PI;
    }
    return eulers;
}

//  Helper function returns the positive angle (in radians) between two 3D vectors
float angleBetween(const glm::vec3& v1, const glm::vec3& v2) {
    return acosf((glm::dot(v1, v2)) / (glm::length(v1) * glm::length(v2)));
}

//  Helper function return the rotation from the first vector onto the second
glm::quat rotationBetween(const glm::vec3& v1, const glm::vec3& v2) {
    float angle = angleBetween(v1, v2);
    if (glm::isnan(angle) || angle < EPSILON) {
        return glm::quat();
    }
    glm::vec3 axis;
    if (angle > 179.99f * RADIANS_PER_DEGREE) { // 180 degree rotation; must use another axis
        axis = glm::cross(v1, glm::vec3(1.0f, 0.0f, 0.0f));
        float axisLength = glm::length(axis);
        if (axisLength < EPSILON) { // parallel to x; y will work
            axis = glm::normalize(glm::cross(v1, glm::vec3(0.0f, 1.0f, 0.0f)));
        } else {
            axis /= axisLength;
        }
    } else {
        axis = glm::normalize(glm::cross(v1, v2));
        // It is possible for axis to be nan even when angle is not less than EPSILON.
        // For example when angle is small but not tiny but v1 and v2 and have very short lengths.
        if (glm::isnan(glm::dot(axis, axis))) {
            // set angle and axis to values that will generate an identity rotation
            angle = 0.0f;
            axis = glm::vec3(1.0f, 0.0f, 0.0f);
        }
    }
    return glm::angleAxis(angle, axis);
}

bool isPointBehindTrianglesPlane(glm::vec3 point, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) {
    glm::vec3 v1 = p0 - p1, v2 = p2 - p1; // Non-collinear vectors contained in the plane
    glm::vec3 n = glm::cross(v1, v2); // Plane's normal vector, pointing out of the triangle
    float d = -glm::dot(n, p0); // Compute plane's equation constant
    return (glm::dot(n, point) + d) >= 0;
}

glm::vec3 extractTranslation(const glm::mat4& matrix) {
    return glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);
}

void setTranslation(glm::mat4& matrix, const glm::vec3& translation) {
    matrix[3][0] = translation.x;
    matrix[3][1] = translation.y;
    matrix[3][2] = translation.z;
}

glm::quat extractRotation(const glm::mat4& matrix, bool assumeOrthogonal) {
    // uses the iterative polar decomposition algorithm described by Ken Shoemake at
    // http://www.cs.wisc.edu/graphics/Courses/838-s2002/Papers/polar-decomp.pdf
    // code adapted from Clyde, https://github.com/threerings/clyde/blob/master/core/src/main/java/com/threerings/math/Matrix4f.java
    // start with the contents of the upper 3x3 portion of the matrix
    glm::mat3 upper = glm::mat3(matrix);
    if (!assumeOrthogonal) {
        for (int i = 0; i < 10; i++) {
            // store the results of the previous iteration
            glm::mat3 previous = upper;
            
            // compute average of the matrix with its inverse transpose
            float sd00 = previous[1][1] * previous[2][2] - previous[2][1] * previous[1][2];
            float sd10 = previous[0][1] * previous[2][2] - previous[2][1] * previous[0][2];
            float sd20 = previous[0][1] * previous[1][2] - previous[1][1] * previous[0][2];
            float det = previous[0][0] * sd00 + previous[2][0] * sd20 - previous[1][0] * sd10;
            if (fabsf(det) == 0.0f) {
                // determinant is zero; matrix is not invertible
                break;
            }
            float hrdet = 0.5f / det;
            upper[0][0] = +sd00 * hrdet + previous[0][0] * 0.5f;
            upper[1][0] = -sd10 * hrdet + previous[1][0] * 0.5f;
            upper[2][0] = +sd20 * hrdet + previous[2][0] * 0.5f;
            
            upper[0][1] = -(previous[1][0] * previous[2][2] - previous[2][0] * previous[1][2]) * hrdet + previous[0][1] * 0.5f;
            upper[1][1] = +(previous[0][0] * previous[2][2] - previous[2][0] * previous[0][2]) * hrdet + previous[1][1] * 0.5f;
            upper[2][1] = -(previous[0][0] * previous[1][2] - previous[1][0] * previous[0][2]) * hrdet + previous[2][1] * 0.5f;
            
            upper[0][2] = +(previous[1][0] * previous[2][1] - previous[2][0] * previous[1][1]) * hrdet + previous[0][2] * 0.5f;
            upper[1][2] = -(previous[0][0] * previous[2][1] - previous[2][0] * previous[0][1]) * hrdet + previous[1][2] * 0.5f;
            upper[2][2] = +(previous[0][0] * previous[1][1] - previous[1][0] * previous[0][1]) * hrdet + previous[2][2] * 0.5f;
            
            // compute the difference; if it's small enough, we're done
            glm::mat3 diff = upper - previous;
            if (diff[0][0] * diff[0][0] + diff[1][0] * diff[1][0] + diff[2][0] * diff[2][0] + diff[0][1] * diff[0][1] +
                diff[1][1] * diff[1][1] + diff[2][1] * diff[2][1] + diff[0][2] * diff[0][2] + diff[1][2] * diff[1][2] +
                diff[2][2] * diff[2][2] < EPSILON) {
                break;
            }
        }
    }
    
    // now that we have a nice orthogonal matrix, we can extract the rotation quaternion
    // using the method described in http://en.wikipedia.org/wiki/Rotation_matrix#Conversions
    float x2 = fabs(1.0f + upper[0][0] - upper[1][1] - upper[2][2]);
    float y2 = fabs(1.0f - upper[0][0] + upper[1][1] - upper[2][2]);
    float z2 = fabs(1.0f - upper[0][0] - upper[1][1] + upper[2][2]);
    float w2 = fabs(1.0f + upper[0][0] + upper[1][1] + upper[2][2]);
    return glm::normalize(glm::quat(0.5f * sqrtf(w2),
                                    0.5f * sqrtf(x2) * (upper[1][2] >= upper[2][1] ? 1.0f : -1.0f),
                                    0.5f * sqrtf(y2) * (upper[2][0] >= upper[0][2] ? 1.0f : -1.0f),
                                    0.5f * sqrtf(z2) * (upper[0][1] >= upper[1][0] ? 1.0f : -1.0f)));
}

glm::vec3 extractScale(const glm::mat4& matrix) {
    return glm::vec3(glm::length(matrix[0]), glm::length(matrix[1]), glm::length(matrix[2]));
}

float extractUniformScale(const glm::mat4& matrix) {
    return extractUniformScale(extractScale(matrix));
}

float extractUniformScale(const glm::vec3& scale) {
    return (scale.x + scale.y + scale.z) / 3.0f;
}

QByteArray createByteArray(const glm::vec3& vector) {
    return QByteArray::number(vector.x) + ',' + QByteArray::number(vector.y) + ',' + QByteArray::number(vector.z);
}

QByteArray createByteArray(const glm::quat& quat) {
    return QByteArray::number(quat.x) + ',' + QByteArray::number(quat.y) + "," + QByteArray::number(quat.z) + ","
        + QByteArray::number(quat.w);
}

bool isSimilarOrientation(const glm::quat& orientionA, const glm::quat& orientionB, float similarEnough) {
    // Compute the angular distance between the two orientations
    float angleOrientation = orientionA == orientionB ? 0.0f : glm::degrees(glm::angle(orientionA * glm::inverse(orientionB)));
    if (isNaN(angleOrientation)) {
        angleOrientation = 0.0f;
    }
    return (angleOrientation <= similarEnough);
}

bool isSimilarPosition(const glm::vec3& positionA, const glm::vec3& positionB, float similarEnough) {
    // Compute the distance between the two points
    float positionDistance = glm::distance(positionA, positionB);
    return (positionDistance <= similarEnough);
}

glm::uvec2 toGlm(const QSize & size) {
    return glm::uvec2(size.width(), size.height());
}

glm::ivec2 toGlm(const QPoint & pt) {
    return glm::ivec2(pt.x(), pt.y());
}

glm::vec2 toGlm(const QPointF & pt) {
    return glm::vec2(pt.x(), pt.y());
}

glm::vec3 toGlm(const xColor & color) {
    static const float MAX_COLOR = 255.0f;
    return glm::vec3(color.red, color.green, color.blue) / MAX_COLOR;
}

glm::vec4 toGlm(const QColor & color) {
    return glm::vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

QMatrix4x4 fromGlm(const glm::mat4 & m) {
  return QMatrix4x4(&m[0][0]).transposed();
}

QSize fromGlm(const glm::ivec2 & v) {
    return QSize(v.x, v.y);
}

QRectF glmToRect(const glm::vec2 & pos, const glm::vec2 & size) {
    QRectF result(pos.x, pos.y, size.x, size.y);
    return result;
}


