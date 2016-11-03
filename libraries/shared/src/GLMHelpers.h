//
//  GLMHelpers.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-08-07.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GLMHelpers_h
#define hifi_GLMHelpers_h

#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// Bring the most commonly used GLM types into the default namespace
using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::uvec2;
using glm::uvec3;
using glm::uvec4;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#include <QtCore/QByteArray>
#include <QtGui/QMatrix4x4>
#include <QtGui/QColor>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "SharedUtil.h"

// this is where the coordinate system is represented
const glm::vec3 IDENTITY_RIGHT = glm::vec3( 1.0f, 0.0f, 0.0f);
const glm::vec3 IDENTITY_UP    = glm::vec3( 0.0f, 1.0f, 0.0f);
const glm::vec3 IDENTITY_FRONT = glm::vec3( 0.0f, 0.0f,-1.0f);

glm::quat safeMix(const glm::quat& q1, const glm::quat& q2, float alpha);


class Quaternions {
 public:
    static const quat IDENTITY;
    static const quat X_180;
    static const quat Y_180;
    static const quat Z_180;
};

class Vectors {
public:
    static const vec3 UNIT_X;
    static const vec3 UNIT_Y;
    static const vec3 UNIT_Z;
    static const vec3 UNIT_NEG_X;
    static const vec3 UNIT_NEG_Y;
    static const vec3 UNIT_NEG_Z;
    static const vec3 UNIT_XY;
    static const vec3 UNIT_XZ;
    static const vec3 UNIT_YZ;
    static const vec3 UNIT_XYZ;
    static const vec3 MAX;
    static const vec3 MIN;
    static const vec3 ZERO;
    static const vec3 ONE;
    static const vec3 TWO;
    static const vec3 HALF;
    static const vec3& RIGHT;
    static const vec3& UP;
    static const vec3& FRONT;
    static const vec3 ZERO4;
};

// These pack/unpack functions are designed to start specific known types in as efficient a manner
// as possible. Taking advantage of the known characteristics of the semantic types.

// Angles are known to be between 0 and 360 degrees, this allows us to encode in 16bits with great accuracy
int packFloatAngleToTwoByte(unsigned char* buffer, float degrees);
int unpackFloatAngleFromTwoByte(const uint16_t* byteAnglePointer, float* destinationPointer);

// Orientation Quats are known to have 4 normalized components be between -1.0 and 1.0
// this allows us to encode each component in 16bits with great accuracy
int packOrientationQuatToBytes(unsigned char* buffer, const glm::quat& quatInput);
int unpackOrientationQuatFromBytes(const unsigned char* buffer, glm::quat& quatOutput);

// alternate compression method that picks the smallest three quaternion components.
// and packs them into 15 bits each. An additional 2 bits are used to encode which component
// was omitted.  Also because the components are encoded from the -1/sqrt(2) to 1/sqrt(2) which
// gives us some extra precision over the -1 to 1 range.  The final result will have a maximum
// error of +- 4.3e-5 error per compoenent.
int packOrientationQuatToSixBytes(unsigned char* buffer, const glm::quat& quatInput);
int unpackOrientationQuatFromSixBytes(const unsigned char* buffer, glm::quat& quatOutput);

// Ratios need the be highly accurate when less than 10, but not very accurate above 10, and they
// are never greater than 1000 to 1, this allows us to encode each component in 16bits
int packFloatRatioToTwoByte(unsigned char* buffer, float ratio);
int unpackFloatRatioFromTwoByte(const unsigned char* buffer, float& ratio);

// Near/Far Clip values need the be highly accurate when less than 10, but only integer accuracy above 10 and
// they are never greater than 16,000, this allows us to encode each component in 16bits
int packClipValueToTwoByte(unsigned char* buffer, float clipValue);
int unpackClipValueFromTwoByte(const unsigned char* buffer, float& clipValue);

// Positive floats that don't need to be very precise
int packFloatToByte(unsigned char* buffer, float value, float scaleBy);
int unpackFloatFromByte(const unsigned char* buffer, float& value, float scaleBy);

// Allows sending of fixed-point numbers: radix 1 makes 15.1 number, radix 8 makes 8.8 number, etc
int packFloatScalarToSignedTwoByteFixed(unsigned char* buffer, float scalar, int radix);
int unpackFloatScalarFromSignedTwoByteFixed(const int16_t* byteFixedPointer, float* destinationPointer, int radix);

// A convenience for sending vec3's as fixed-point floats
int packFloatVec3ToSignedTwoByteFixed(unsigned char* destBuffer, const glm::vec3& srcVector, int radix);
int unpackFloatVec3FromSignedTwoByteFixed(const unsigned char* sourceBuffer, glm::vec3& destination, int radix);

/// \return vec3 with euler angles in radians
glm::vec3 safeEulerAngles(const glm::quat& q);

float angleBetween(const glm::vec3& v1, const glm::vec3& v2);

glm::quat rotationBetween(const glm::vec3& v1, const glm::vec3& v2);

bool isPointBehindTrianglesPlane(glm::vec3 point, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2);

glm::vec3 extractTranslation(const glm::mat4& matrix);

void setTranslation(glm::mat4& matrix, const glm::vec3& translation);

glm::quat extractRotation(const glm::mat4& matrix, bool assumeOrthogonal = false);
glm::quat glmExtractRotation(const glm::mat4& matrix);

glm::vec3 extractScale(const glm::mat4& matrix);

float extractUniformScale(const glm::mat4& matrix);

float extractUniformScale(const glm::vec3& scale);

QByteArray createByteArray(const glm::vec3& vector);
QByteArray createByteArray(const glm::quat& quat);

/// \return bool are two orientations similar to each other
const float ORIENTATION_SIMILAR_ENOUGH = 5.0f; // 10 degrees in any direction
bool isSimilarOrientation(const glm::quat& orientionA, const glm::quat& orientionB,
                          float similarEnough = ORIENTATION_SIMILAR_ENOUGH);
const float POSITION_SIMILAR_ENOUGH = 0.1f; // 0.1 meter
bool isSimilarPosition(const glm::vec3& positionA, const glm::vec3& positionB, float similarEnough = POSITION_SIMILAR_ENOUGH);

uvec2 toGlm(const QSize& size);
ivec2 toGlm(const QPoint& pt);
vec2 toGlm(const QPointF& pt);
vec3 toGlm(const xColor& color);
vec4 toGlm(const QColor& color);
ivec4 toGlm(const QRect& rect);
vec4 toGlm(const xColor& color, float alpha);

QSize fromGlm(const glm::ivec2 & v);
QMatrix4x4 fromGlm(const glm::mat4 & m);

QRectF glmToRect(const glm::vec2 & pos, const glm::vec2 & size);

template <typename T>
float aspect(const T& t) {
    return (float)t.x / (float)t.y;
}

// Take values in an arbitrary range [0, size] and convert them to the range [0, 1]
template <typename T>
T toUnitScale(const T& value, const T& size) {
    return value / size;
}

// Take values in an arbitrary range [0, size] and convert them to the range [0, 1]
template <typename T>
T toNormalizedDeviceScale(const T& value, const T& size) {
    vec2 result = toUnitScale(value, size);
    result *= 2.0f;
    result -= 1.0f;
    return result;
}

#define YAW(euler) euler.y
#define PITCH(euler) euler.x
#define ROLL(euler) euler.z

// float - linear interpolate
inline float lerp(float x, float y, float a) {
    return x * (1.0f - a) + (y * a);
}

// vec2 lerp - linear interpolate
template<typename T, glm::precision P>
glm::tvec2<T, P> lerp(const glm::tvec2<T, P>& x, const glm::tvec2<T, P>& y, T a) {
    return x * (T(1) - a) + (y * a);
}

// vec3 lerp - linear interpolate
template<typename T, glm::precision P>
glm::tvec3<T, P> lerp(const glm::tvec3<T, P>& x, const glm::tvec3<T, P>& y, T a) {
    return x * (T(1) - a) + (y * a);
}

// vec4 lerp - linear interpolate
template<typename T, glm::precision P>
glm::tvec4<T, P> lerp(const glm::tvec4<T, P>& x, const glm::tvec4<T, P>& y, T a) {
    return x * (T(1) - a) + (y * a);
}

glm::mat4 createMatFromQuatAndPos(const glm::quat& q, const glm::vec3& p);
glm::mat4 createMatFromScaleQuatAndPos(const glm::vec3& scale, const glm::quat& rot, const glm::vec3& trans);
glm::quat cancelOutRoll(const glm::quat& q);
glm::quat cancelOutRollAndPitch(const glm::quat& q);
glm::mat4 cancelOutRollAndPitch(const glm::mat4& m);
glm::vec3 transformPoint(const glm::mat4& m, const glm::vec3& p);
glm::vec3 transformVectorFast(const glm::mat4& m, const glm::vec3& v);
glm::vec3 transformVectorFull(const glm::mat4& m, const glm::vec3& v);

// Calculate an orthogonal basis from a primary and secondary axis.
// The uAxis, vAxis & wAxis will form an orthognal basis.
// The primary axis will be the uAxis.
// The vAxis will be as close as possible to to the secondary axis.
void generateBasisVectors(const glm::vec3& primaryAxis, const glm::vec3& secondaryAxis,
                          glm::vec3& uAxisOut, glm::vec3& vAxisOut, glm::vec3& wAxisOut);

glm::vec2 getFacingDir2D(const glm::quat& rot);
glm::vec2 getFacingDir2D(const glm::mat4& m);

inline bool isNaN(const glm::vec3& value) { return isNaN(value.x) || isNaN(value.y) || isNaN(value.z); }
inline bool isNaN(const glm::quat& value) { return isNaN(value.w) || isNaN(value.x) || isNaN(value.y) || isNaN(value.z); }

glm::mat4 orthoInverse(const glm::mat4& m);

#endif // hifi_GLMHelpers_h
