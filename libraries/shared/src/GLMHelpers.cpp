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
#include <glm/gtc/matrix_transform.hpp>
#include "NumericalConstants.h"

const vec3 Vectors::UNIT_X{ 1.0f, 0.0f, 0.0f };
const vec3 Vectors::UNIT_Y{ 0.0f, 1.0f, 0.0f };
const vec3 Vectors::UNIT_Z{ 0.0f, 0.0f, 1.0f };
const vec3 Vectors::UNIT_NEG_X{ -1.0f, 0.0f, 0.0f };
const vec3 Vectors::UNIT_NEG_Y{ 0.0f, -1.0f, 0.0f };
const vec3 Vectors::UNIT_NEG_Z{ 0.0f, 0.0f, -1.0f };
const vec3 Vectors::UNIT_XY{ glm::normalize(UNIT_X + UNIT_Y) };
const vec3 Vectors::UNIT_XZ{ glm::normalize(UNIT_X + UNIT_Z) };
const vec3 Vectors::UNIT_YZ{ glm::normalize(UNIT_Y + UNIT_Z) };
const vec3 Vectors::UNIT_XYZ{ glm::normalize(UNIT_X + UNIT_Y + UNIT_Z) };
const vec3 Vectors::MAX{ FLT_MAX };
const vec3 Vectors::MIN{ -FLT_MAX };
const vec3 Vectors::ZERO{ 0.0f };
const vec3 Vectors::ONE{ 1.0f };
const vec3 Vectors::TWO{ 2.0f };
const vec3 Vectors::HALF{ 0.5f };
const vec3& Vectors::RIGHT = Vectors::UNIT_X;
const vec3& Vectors::UP = Vectors::UNIT_Y;
const vec3& Vectors::FRONT = Vectors::UNIT_NEG_Z;

const quat Quaternions::IDENTITY{ 1.0f, 0.0f, 0.0f, 0.0f };
const quat Quaternions::X_180{ 0.0f, 1.0f, 0.0f, 0.0f };
const quat Quaternions::Y_180{ 0.0f, 0.0f, 1.0f, 0.0f };
const quat Quaternions::Z_180{ 0.0f, 0.0f, 0.0f, 1.0f };

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

// Allows sending of fixed-point numbers: radix 1 makes 15.1 number, radix 8 makes 8.8 number, etc
int packFloatScalarToSignedOneByteFixed(unsigned char* buffer, float scalar, int radix) {
    uint8_t outVal = (uint8_t)(scalar * (float)(1 << radix));
    memcpy(buffer, &outVal, sizeof(uint8_t));
    return sizeof(outVal);
}

int unpackFloatScalarFromSignedOneByteFixed(const uint8_t* byteFixedPointer, float* destinationPointer, int radix) {
    *destinationPointer = *byteFixedPointer / (float)(1 << radix);
    return sizeof(uint8_t);
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

#define HI_BYTE(x) (uint8_t)(x >> 8)
#define LO_BYTE(x) (uint8_t)(0xff & x)

int packOrientationQuatToSixBytes(unsigned char* buffer, const glm::quat& quatInput) {

    // find largest component
    uint8_t largestComponent = 0;
    for (int i = 1; i < 4; i++) {
        if (fabs(quatInput[i]) > fabs(quatInput[largestComponent])) {
            largestComponent = i;
        }
    }

    // ensure that the sign of the dropped component is always negative.
    glm::quat q = quatInput[largestComponent] > 0 ? -quatInput : quatInput;

    const float MAGNITUDE = 1.0f / sqrtf(2.0f);
    const uint32_t NUM_BITS_PER_COMPONENT = 15;
    const uint32_t RANGE = (1 << NUM_BITS_PER_COMPONENT) - 1;

    // quantize the smallest three components into integers
    uint16_t components[3];
    for (int i = 0, j = 0; i < 4; i++) {
        if (i != largestComponent) {
            // transform component into 0..1 range.
            float value = (q[i] + MAGNITUDE) / (2.0f * MAGNITUDE);

            // quantize 0..1 into 0..range
            components[j] = (uint16_t)(value * RANGE);
            j++;
        }
    }

    // encode the largestComponent into the high bits of the first two components
    components[0] = (0x7fff & components[0]) | ((0x01 & largestComponent) << 15);
    components[1] = (0x7fff & components[1]) | ((0x02 & largestComponent) << 14);

    buffer[0] = HI_BYTE(components[0]);
    buffer[1] = LO_BYTE(components[0]);
    buffer[2] = HI_BYTE(components[1]);
    buffer[3] = LO_BYTE(components[1]);
    buffer[4] = HI_BYTE(components[2]);
    buffer[5] = LO_BYTE(components[2]);

    return 6;
}

int unpackOrientationQuatFromSixBytes(const unsigned char* buffer, glm::quat& quatOutput) {

    uint16_t components[3];
    components[0] = ((uint16_t)(0x7f & buffer[0]) << 8) | buffer[1];
    components[1] = ((uint16_t)(0x7f & buffer[2]) << 8) | buffer[3];
    components[2] = ((uint16_t)(0x7f & buffer[4]) << 8) | buffer[5];

    // largestComponent is encoded into the highest bits of the first 2 components
    uint8_t largestComponent = ((0x80 & buffer[2]) >> 6) | ((0x80 & buffer[0]) >> 7);

    const uint32_t NUM_BITS_PER_COMPONENT = 15;
    const float RANGE = (float)((1 << NUM_BITS_PER_COMPONENT) - 1);
    const float MAGNITUDE = 1.0f / sqrtf(2.0f);
    float floatComponents[3];
    for (int i = 0; i < 3; i++) {
        floatComponents[i] = ((float)components[i] / RANGE) * (2.0f * MAGNITUDE) - MAGNITUDE;
    }

    // missingComponent is always negative.
    float missingComponent = -sqrtf(1.0f - floatComponents[0] * floatComponents[0] - floatComponents[1] * floatComponents[1] - floatComponents[2] * floatComponents[2]);

    for (int i = 0, j = 0; i < 4; i++) {
        if (i != largestComponent) {
            quatOutput[i] = floatComponents[j];
            j++;
        } else {
            quatOutput[i] = missingComponent;
        }
    }

    return 6;
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
    return glm::rotation(glm::normalize(v1), glm::normalize(v2));
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

glm::quat glmExtractRotation(const glm::mat4& matrix) {
    glm::vec3 scale = extractScale(matrix);
    // quat_cast doesn't work so well with scaled matrices, so cancel it out.
    glm::mat4 tmp = glm::scale(matrix, 1.0f / scale);
    return glm::normalize(glm::quat_cast(tmp));
}

glm::vec3 extractScale(const glm::mat4& matrix) {
    glm::mat3 m(matrix);
    float det = glm::determinant(m);
    if (det < 0.0f) {
        // left handed matrix, flip sign to compensate.
        return glm::vec3(-glm::length(m[0]), glm::length(m[1]), glm::length(m[2]));
    } else {
        return glm::vec3(glm::length(m[0]), glm::length(m[1]), glm::length(m[2]));
    }
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

glm::uvec2 toGlm(const QSize& size) {
    return glm::uvec2(size.width(), size.height());
}

glm::ivec2 toGlm(const QPoint& pt) {
    return glm::ivec2(pt.x(), pt.y());
}

glm::vec2 toGlm(const QPointF& pt) {
    return glm::vec2(pt.x(), pt.y());
}

glm::vec3 toGlm(const xColor& color) {
    static const float MAX_COLOR = 255.0f;
    return glm::vec3(color.red, color.green, color.blue) / MAX_COLOR;
}

glm::vec4 toGlm(const QColor& color) {
    return glm::vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

ivec4 toGlm(const QRect& rect) {
    return ivec4(rect.x(), rect.y(), rect.width(), rect.height());
}

QMatrix4x4 fromGlm(const glm::mat4 & m) {
  return QMatrix4x4(&m[0][0]).transposed();
}

QSize fromGlm(const glm::ivec2 & v) {
    return QSize(v.x, v.y);
}

vec4 toGlm(const xColor& color, float alpha) {
    return vec4((float)color.red / 255.0f, (float)color.green / 255.0f, (float)color.blue / 255.0f, alpha);
}

QRectF glmToRect(const glm::vec2 & pos, const glm::vec2 & size) {
    QRectF result(pos.x, pos.y, size.x, size.y);
    return result;
}

// create matrix from orientation and position
glm::mat4 createMatFromQuatAndPos(const glm::quat& q, const glm::vec3& p) {
    glm::mat4 m = glm::mat4_cast(q);
    m[3] = glm::vec4(p, 1.0f);
    return m;
}

// create matrix from a non-uniform scale, orientation and position
glm::mat4 createMatFromScaleQuatAndPos(const glm::vec3& scale, const glm::quat& rot, const glm::vec3& trans) {
    glm::vec3 xAxis = rot * glm::vec3(scale.x, 0.0f, 0.0f);
    glm::vec3 yAxis = rot * glm::vec3(0.0f, scale.y, 0.0f);
    glm::vec3 zAxis = rot * glm::vec3(0.0f, 0.0f, scale.z);
    return glm::mat4(glm::vec4(xAxis, 0.0f), glm::vec4(yAxis, 0.0f),
                     glm::vec4(zAxis, 0.0f), glm::vec4(trans, 1.0f));
}

// cancel out roll 
glm::quat cancelOutRoll(const glm::quat& q) {
    glm::vec3 forward = q * Vectors::FRONT;
    return glm::quat_cast(glm::inverse(glm::lookAt(Vectors::ZERO, forward, Vectors::UP)));
}

// cancel out roll and pitch
glm::quat cancelOutRollAndPitch(const glm::quat& q) {
    glm::vec3 zAxis = q * glm::vec3(0.0f, 0.0f, 1.0f);

    // cancel out the roll and pitch
    glm::vec3 newZ = (zAxis.x == 0 && zAxis.z == 0.0f) ? vec3(1.0f, 0.0f, 0.0f) : glm::normalize(vec3(zAxis.x, 0.0f, zAxis.z));
    glm::vec3 newX = glm::cross(vec3(0.0f, 1.0f, 0.0f), newZ);
    glm::vec3 newY = glm::cross(newZ, newX);

    glm::mat4 temp(glm::vec4(newX, 0.0f), glm::vec4(newY, 0.0f), glm::vec4(newZ, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    return glm::quat_cast(temp);
}

// cancel out roll and pitch
glm::mat4 cancelOutRollAndPitch(const glm::mat4& m) {
    glm::vec3 zAxis = glm::vec3(m[2]);

    // cancel out the roll and pitch
    glm::vec3 newZ = (zAxis.x == 0.0f && zAxis.z == 0.0f) ? vec3(1.0f, 0.0f, 0.0f) : glm::normalize(vec3(zAxis.x, 0.0f, zAxis.z));
    glm::vec3 newX = glm::cross(vec3(0.0f, 1.0f, 0.0f), newZ);
    glm::vec3 newY = glm::cross(newZ, newX);

    glm::mat4 temp(glm::vec4(newX, 0.0f), glm::vec4(newY, 0.0f), glm::vec4(newZ, 0.0f), m[3]);
    return temp;
}

glm::vec3 transformPoint(const glm::mat4& m, const glm::vec3& p) {
    glm::vec4 temp = m * glm::vec4(p, 1.0f);
    if (temp.w != 1.0f) {
        temp *= (1.0f / temp.w);
    }
    return glm::vec3(temp);
}

// does not handle non-uniform scale correctly, but it's faster then transformVectorFull
glm::vec3 transformVectorFast(const glm::mat4& m, const glm::vec3& v) {
    glm::mat3 rot(m);
    return rot * v;
}

// handles non-uniform scale.
glm::vec3 transformVectorFull(const glm::mat4& m, const glm::vec3& v) {
    glm::mat3 rot(m);
    return glm::inverse(glm::transpose(rot)) * v;
}

void generateBasisVectors(const glm::vec3& primaryAxis, const glm::vec3& secondaryAxis,
                          glm::vec3& uAxisOut, glm::vec3& vAxisOut, glm::vec3& wAxisOut) {

    // primaryAxis & secondaryAxis must not be zero.
#ifndef NDEBUG
    const float MIN_LENGTH_SQUARED = 1.0e-6f;
#endif
    assert(glm::length2(primaryAxis) > MIN_LENGTH_SQUARED);
    assert(glm::length2(secondaryAxis) > MIN_LENGTH_SQUARED);

    uAxisOut = glm::normalize(primaryAxis);
    glm::vec3 normSecondary = glm::normalize(secondaryAxis);

    // if secondaryAxis is parallel with the primaryAxis, pick another axis.
    const float EPSILON = 1.0e-4f;
    if (fabsf(fabsf(glm::dot(uAxisOut, secondaryAxis)) - 1.0f) > EPSILON) {
        // pick a better secondaryAxis.
        normSecondary = glm::vec3(1.0f, 0.0f, 0.0f);
        if (fabsf(fabsf(glm::dot(uAxisOut, secondaryAxis)) - 1.0f) > EPSILON) {
            normSecondary = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    wAxisOut = glm::normalize(glm::cross(uAxisOut, secondaryAxis));
    vAxisOut = glm::cross(wAxisOut, uAxisOut);
}

glm::vec2 getFacingDir2D(const glm::quat& rot) {
    glm::vec3 facing3D = rot * Vectors::UNIT_NEG_Z;
    glm::vec2 facing2D(facing3D.x, facing3D.z);
    const float ALMOST_ZERO = 0.0001f;
    if (glm::length(facing2D) < ALMOST_ZERO) {
        return glm::vec2(1.0f, 0.0f);
    } else {
        return glm::normalize(facing2D);
    }
}

glm::vec2 getFacingDir2D(const glm::mat4& m) {
    glm::vec3 facing3D = transformVectorFast(m, Vectors::UNIT_NEG_Z);
    glm::vec2 facing2D(facing3D.x, facing3D.z);
    const float ALMOST_ZERO = 0.0001f;
    if (glm::length(facing2D) < ALMOST_ZERO) {
        return glm::vec2(1.0f, 0.0f);
    } else {
        return glm::normalize(facing2D);
    }
}

glm::mat4 orthoInverse(const glm::mat4& m) {
    glm::mat4 r = m;
    r[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    r = glm::transpose(r);
    r[3] = -(r * m[3]);
    r[3][3] = 1.0f;
    return r;
}
