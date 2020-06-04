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

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/component_wise.hpp>

// Bring the most commonly used GLM types into the default namespace
using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::uvec2;
using glm::u8vec3;
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
const glm::vec3 IDENTITY_FORWARD = glm::vec3( 0.0f, 0.0f,-1.0f);

glm::quat safeMix(const glm::quat& q1, const glm::quat& q2, float alpha);

class Matrices {
public:
    static const mat4 IDENTITY;
    static const mat4 X_180;
    static const mat4 Y_180;
    static const mat4 Z_180;
};

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

bool closeEnough(float a, float b, float relativeError);

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
vec3 toGlm(const glm::u8vec3& color);
vec4 toGlm(const QColor& color);
ivec4 toGlm(const QRect& rect);
vec4 toGlm(const glm::u8vec3& color, float alpha);

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
glm::mat4 createMatFromScale(const glm::vec3& scale);
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

// assumes z-forward and y-up
glm::vec2 getFacingDir2D(const glm::quat& rot);

// assumes z-forward and y-up
glm::vec2 getFacingDir2D(const glm::mat4& m);

inline bool isNaN(const glm::vec3& value) { return isNaN(value.x) || isNaN(value.y) || isNaN(value.z); }
inline bool isNaN(const glm::quat& value) { return isNaN(value.w) || isNaN(value.x) || isNaN(value.y) || isNaN(value.z); }
inline bool isNaN(const glm::mat3& value) { return isNaN(value * glm::vec3(1.0f)); }

glm::mat4 orthoInverse(const glm::mat4& m);

//  Return a random vector of average length 1
glm::vec3 randVector();

bool isNonUniformScale(const glm::vec3& scale);

//
// Safe replacement of glm_mat4_mul() for unaligned arguments instead of __m128
//
inline void glm_mat4u_mul(const glm::mat4& m1, const glm::mat4& m2, glm::mat4& r) {

#if GLM_ARCH & GLM_ARCH_SSE2_BIT
    __m128 u0 = _mm_loadu_ps((float*)&m1[0][0]);
    __m128 u1 = _mm_loadu_ps((float*)&m1[1][0]);
    __m128 u2 = _mm_loadu_ps((float*)&m1[2][0]);
    __m128 u3 = _mm_loadu_ps((float*)&m1[3][0]);

    __m128 v0 = _mm_loadu_ps((float*)&m2[0][0]);
    __m128 v1 = _mm_loadu_ps((float*)&m2[1][0]);
    __m128 v2 = _mm_loadu_ps((float*)&m2[2][0]);
    __m128 v3 = _mm_loadu_ps((float*)&m2[3][0]);

    __m128 t0 = _mm_mul_ps(_mm_shuffle_ps(v0, v0, _MM_SHUFFLE(0,0,0,0)), u0);
    __m128 t1 = _mm_mul_ps(_mm_shuffle_ps(v0, v0, _MM_SHUFFLE(1,1,1,1)), u1);
    __m128 t2 = _mm_mul_ps(_mm_shuffle_ps(v0, v0, _MM_SHUFFLE(2,2,2,2)), u2);
    __m128 t3 = _mm_mul_ps(_mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3,3,3,3)), u3);
    v0 = _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3));

    t0 = _mm_mul_ps(_mm_shuffle_ps(v1, v1, _MM_SHUFFLE(0,0,0,0)), u0);
    t1 = _mm_mul_ps(_mm_shuffle_ps(v1, v1, _MM_SHUFFLE(1,1,1,1)), u1);
    t2 = _mm_mul_ps(_mm_shuffle_ps(v1, v1, _MM_SHUFFLE(2,2,2,2)), u2);
    t3 = _mm_mul_ps(_mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3,3,3,3)), u3);
    v1 = _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3));

    t0 = _mm_mul_ps(_mm_shuffle_ps(v2, v2, _MM_SHUFFLE(0,0,0,0)), u0);
    t1 = _mm_mul_ps(_mm_shuffle_ps(v2, v2, _MM_SHUFFLE(1,1,1,1)), u1);
    t2 = _mm_mul_ps(_mm_shuffle_ps(v2, v2, _MM_SHUFFLE(2,2,2,2)), u2);
    t3 = _mm_mul_ps(_mm_shuffle_ps(v2, v2, _MM_SHUFFLE(3,3,3,3)), u3);
    v2 = _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3));

    t0 = _mm_mul_ps(_mm_shuffle_ps(v3, v3, _MM_SHUFFLE(0,0,0,0)), u0);
    t1 = _mm_mul_ps(_mm_shuffle_ps(v3, v3, _MM_SHUFFLE(1,1,1,1)), u1);
    t2 = _mm_mul_ps(_mm_shuffle_ps(v3, v3, _MM_SHUFFLE(2,2,2,2)), u2);
    t3 = _mm_mul_ps(_mm_shuffle_ps(v3, v3, _MM_SHUFFLE(3,3,3,3)), u3);
    v3 = _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3));

    _mm_storeu_ps((float*)&r[0][0], v0);
    _mm_storeu_ps((float*)&r[1][0], v1);
    _mm_storeu_ps((float*)&r[2][0], v2);
    _mm_storeu_ps((float*)&r[3][0], v3);
#else
    r = m1 * m2;
#endif
}

//
// Fast replacement of glm::packSnorm3x10_1x2()
// The SSE2 version quantizes using round to nearest even.
// The glm version quantizes using round away from zero.
//
inline uint32_t glm_packSnorm3x10_1x2(vec4 const& v) {

    union i10i10i10i2 {
        struct {
            int x : 10;
            int y : 10;
            int z : 10;
            int w : 2;
        } data;
        uint32_t pack;
    } Result;

#if GLM_ARCH & GLM_ARCH_SSE2_BIT
    __m128 vclamp = _mm_min_ps(_mm_max_ps(_mm_loadu_ps((float*)&v[0]), _mm_set1_ps(-1.0f)), _mm_set1_ps(1.0f));
    __m128i vpack = _mm_cvtps_epi32(_mm_mul_ps(vclamp, _mm_setr_ps(511.f, 511.f, 511.f, 1.f)));

    Result.data.x = _mm_cvtsi128_si32(vpack);
    Result.data.y = _mm_cvtsi128_si32(_mm_shuffle_epi32(vpack, _MM_SHUFFLE(1,1,1,1)));
    Result.data.z = _mm_cvtsi128_si32(_mm_shuffle_epi32(vpack, _MM_SHUFFLE(2,2,2,2)));
    Result.data.w = _mm_cvtsi128_si32(_mm_shuffle_epi32(vpack, _MM_SHUFFLE(3,3,3,3)));
#else
    ivec4 const Pack(round(clamp(v, -1.0f, 1.0f) * vec4(511.f, 511.f, 511.f, 1.f)));

    Result.data.x = Pack.x;
    Result.data.y = Pack.y;
    Result.data.z = Pack.z;
    Result.data.w = Pack.w;
#endif
    return Result.pack;
}

// convert float to int, using round-to-nearest-even (undefined on overflow)
inline int fastLrintf(float x) {
#if GLM_ARCH & GLM_ARCH_SSE2_BIT
    return _mm_cvt_ss2si(_mm_set_ss(x));
#else
    // return lrintf(x);
    static_assert(std::numeric_limits<double>::is_iec559, "Requires IEEE-754 double precision format");
    union { double d; int64_t i; } bits = { (double)x };
    bits.d += (3ULL << 51);
    return (int)bits.i;
#endif
}

// returns the FOV from the projection matrix
inline glm::vec4 extractFov( const glm::mat4& m) {
    static const std::array<glm::vec4, 4> CLIPS{ {
                                                { 1, 0, 0, 1 },
                                                { -1, 0, 0, 1 },
                                                { 0, 1, 0, 1 },
                                                { 0, -1, 0, 1 }
                                            } };

    glm::mat4 mt = glm::transpose(m);
    glm::vec4 v, result;
    // Left
    v = mt * CLIPS[0];
    result.x = -atanf(v.z / v.x);
    // Right
    v = mt * CLIPS[1];
    result.y = atanf(v.z / v.x);
    // Down
    v = mt * CLIPS[2];
    result.z = -atanf(v.z / v.y);
    // Up
    v = mt * CLIPS[3];
    result.w = atanf(v.z / v.y);
    return result;
}

#endif // hifi_GLMHelpers_h
