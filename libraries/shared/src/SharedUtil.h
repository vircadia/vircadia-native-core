//
//  SharedUtil.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SharedUtil_h
#define hifi_SharedUtil_h

#include <math.h>
#include <stdint.h>

#ifndef _WIN32
#include <unistd.h> // not on windows, not needed for mac or windows
#endif

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QDebug>

const int BYTES_PER_COLOR = 3;
const int BYTES_PER_FLAGS = 1;
typedef unsigned char rgbColor[BYTES_PER_COLOR];
typedef unsigned char colorPart;
typedef unsigned char nodeColor[BYTES_PER_COLOR + BYTES_PER_FLAGS];
typedef unsigned char rgbColor[BYTES_PER_COLOR];

struct xColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

static const float ZERO             = 0.0f;
static const float ONE              = 1.0f;
static const float ONE_HALF			= 0.5f;
static const float ONE_THIRD        = 0.333333f;

static const float PI                 = 3.14159265358979f;
static const float TWO_PI             = 2.f * PI;
static const float PI_OVER_TWO        = ONE_HALF * PI;
static const float RADIANS_PER_DEGREE = PI / 180.0f;
static const float DEGREES_PER_RADIAN = 180.0f / PI;

static const float EPSILON          = 0.000001f;	//smallish positive number - used as margin of error for some computations
static const float SQUARE_ROOT_OF_2 = (float)sqrt(2.f);
static const float SQUARE_ROOT_OF_3 = (float)sqrt(3.f);
static const float METERS_PER_DECIMETER  = 0.1f;
static const float METERS_PER_CENTIMETER = 0.01f;
static const float METERS_PER_MILLIMETER = 0.001f;
static const float MILLIMETERS_PER_METER = 1000.0f;
static const quint64 USECS_PER_MSEC = 1000;
static const quint64 MSECS_PER_SECOND = 1000;
static const quint64 USECS_PER_SECOND = USECS_PER_MSEC * MSECS_PER_SECOND;

const int BITS_IN_BYTE  = 8;

quint64 usecTimestampNow();
void usecTimestampNowForceClockSkew(int clockSkew);

float randFloat();
int randIntInRange (int min, int max);
float randFloatInRange (float min,float max);
float randomSign(); /// \return -1.0 or 1.0
unsigned char randomColorValue(int minimum = 0);
bool randomBoolean();

glm::quat safeMix(const glm::quat& q1, const glm::quat& q2, float alpha);

bool shouldDo(float desiredInterval, float deltaTime);

void outputBufferBits(const unsigned char* buffer, int length, QDebug* continuedDebug = NULL);
void outputBits(unsigned char byte, QDebug* continuedDebug = NULL);
void printVoxelCode(unsigned char* voxelCode);
int numberOfOnes(unsigned char byte);
bool oneAtBit(unsigned char byte, int bitIndex);
void setAtBit(unsigned char& byte, int bitIndex);
void clearAtBit(unsigned char& byte, int bitIndex);
int  getSemiNibbleAt(unsigned char byte, int bitIndex);
void setSemiNibbleAt(unsigned char& byte, int bitIndex, int value);

int getNthBit(unsigned char byte, int ordinal); /// determines the bit placement 0-7 of the ordinal set bit

bool isInEnvironment(const char* environment);

const char* getCmdOption(int argc, const char * argv[],const char* option);
bool cmdOptionExists(int argc, const char * argv[],const char* option);

void sharedMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message);

unsigned char* pointToVoxel(float x, float y, float z, float s, unsigned char r = 0, unsigned char g = 0, unsigned char b = 0);
unsigned char* pointToOctalCode(float x, float y, float z, float s);

#ifdef _WIN32
void usleep(int waitTime);
#endif

int insertIntoSortedArrays(void* value, float key, int originalIndex,
                           void** valueArray, float* keyArray, int* originalIndexArray,
                           int currentCount, int maxCount);

int removeFromSortedArrays(void* value, void** valueArray, float* keyArray, int* originalIndexArray,
                           int currentCount, int maxCount);



// Helper Class for debugging
class debug {
public:
    static const char* valueOf(bool checkValue) { return checkValue ? "yes" : "no"; }
    static void setDeadBeef(void* memoryVoid, int size);
    static void checkDeadBeef(void* memoryVoid, int size);
private:
    static unsigned char DEADBEEF[];
    static int DEADBEEF_SIZE;
};

bool isBetween(int64_t value, int64_t max, int64_t min);


// These pack/unpack functions are designed to start specific known types in as efficient a manner
// as possible. Taking advantage of the known characteristics of the semantic types.

// Angles are known to be between 0 and 360 degrees, this allows us to encode in 16bits with great accuracy
int packFloatAngleToTwoByte(unsigned char* buffer, float degrees);
int unpackFloatAngleFromTwoByte(const uint16_t* byteAnglePointer, float* destinationPointer);

// Orientation Quats are known to have 4 normalized components be between -1.0 and 1.0
// this allows us to encode each component in 16bits with great accuracy
int packOrientationQuatToBytes(unsigned char* buffer, const glm::quat& quatInput);
int unpackOrientationQuatFromBytes(const unsigned char* buffer, glm::quat& quatOutput);

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

glm::vec3 extractTranslation(const glm::mat4& matrix);

void setTranslation(glm::mat4& matrix, const glm::vec3& translation);

glm::quat extractRotation(const glm::mat4& matrix, bool assumeOrthogonal = false);

glm::vec3 extractScale(const glm::mat4& matrix);

float extractUniformScale(const glm::mat4& matrix);

float extractUniformScale(const glm::vec3& scale);

/// \return bool are two orientations similar to each other
const float ORIENTATION_SIMILAR_ENOUGH = 5.0f; // 10 degrees in any direction
bool isSimilarOrientation(const glm::quat& orientionA, const glm::quat& orientionB, 
                        float similarEnough = ORIENTATION_SIMILAR_ENOUGH);
const float POSITION_SIMILAR_ENOUGH = 0.1f; // 0.1 meter
bool isSimilarPosition(const glm::vec3& positionA, const glm::vec3& positionB, float similarEnough = POSITION_SIMILAR_ENOUGH);

/// \return bool is the float NaN                        
bool isNaN(float value);

QByteArray createByteArray(const glm::vec3& vector);

QString formatUsecTime(float usecs, int prec = 3);

#endif // hifi_SharedUtil_h
