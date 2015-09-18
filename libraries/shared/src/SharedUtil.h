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

#include <memory>
#include <math.h>
#include <stdint.h>

#ifndef _WIN32
#include <unistd.h> // not on windows, not needed for mac or windows
#endif

#include <QDebug>

const int BYTES_PER_COLOR = 3;
const int BYTES_PER_FLAGS = 1;
typedef unsigned char colorPart;
typedef unsigned char nodeColor[BYTES_PER_COLOR + BYTES_PER_FLAGS];
typedef unsigned char rgbColor[BYTES_PER_COLOR];

inline QDebug& operator<<(QDebug& dbg, const rgbColor& c) {
    dbg.nospace() << "{type='rgbColor'"
        ", red=" << c[0] <<
        ", green=" << c[1] <<
        ", blue=" << c[2] <<
        "}";
    return dbg;
}

struct xColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

inline QDebug& operator<<(QDebug& dbg, const xColor& c) {
    dbg.nospace() << "{type='xColor'"
        ", red=" << c.red <<
        ", green=" << c.green <<
        ", blue=" << c.blue <<
        "}";
    return dbg;
}

inline bool operator==(const xColor& lhs, const xColor& rhs)
{
    return (lhs.red == rhs.red) && (lhs.green == rhs.green) && (lhs.blue == rhs.blue);
}

inline bool operator!=(const xColor& lhs, const xColor& rhs)
{
    return (lhs.red != rhs.red) || (lhs.green != rhs.green) || (lhs.blue != rhs.blue);
}

// Use a custom User-Agent to avoid ModSecurity filtering, e.g. by hosting providers.
const QByteArray HIGH_FIDELITY_USER_AGENT = "Mozilla/5.0 (HighFidelityInterface)";

quint64 usecTimestampNow(bool wantDebug = false);
void usecTimestampNowForceClockSkew(int clockSkew);

float randFloat();
int randIntInRange (int min, int max);
float randFloatInRange (float min,float max);
float randomSign(); /// \return -1.0 or 1.0
unsigned char randomColorValue(int minimum = 0);
bool randomBoolean();

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


/// \return bool is the float NaN
bool isNaN(float value);

QString formatUsecTime(float usecs, int prec = 3);
QString formatSecondsElapsed(float seconds);
bool similarStrings(const QString& stringA, const QString& stringB);

template <typename T>
uint qHash(const std::shared_ptr<T>& ptr, uint seed = 0)
{
    return qHash(ptr.get(), seed);
}

#endif // hifi_SharedUtil_h
