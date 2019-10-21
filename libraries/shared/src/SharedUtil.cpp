//
//  SharedUtil.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SharedUtil.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <time.h>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <chrono>

#include <QtCore/QOperatingSystemVersion>
#include <glm/glm.hpp>

#ifdef Q_OS_WIN
#include <windows.h>
#include "CPUIdent.h"
#include <Psapi.h>

#if _MSC_VER >= 1900
#pragma comment(lib, "legacy_stdio_definitions.lib")
FILE _iob[] = {*stdin, *stdout, *stderr};
extern "C" FILE * __cdecl __iob_func(void) {
    return _iob;
}
#endif

#endif


#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif


#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <signal.h>
#include <cerrno>
#endif

#include <QtCore/QDebug>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTimer>
#include <QProcess>
#include <QSysInfo>
#include <QThread>

#include <BuildInfo.h>

#include "LogHandler.h"
#include "NumericalConstants.h"
#include "OctalCode.h"
#include "SharedLogging.h"

//     Global instances are stored inside the QApplication properties
// to provide a single instance across DLL boundaries.
// This is something we cannot do here since several DLLs
// and our main binaries statically link this "shared" library
// resulting in multiple static memory blocks in different constexts
//     But we need to be able to use global instances before the QApplication
// is setup, so to accomplish that we stage the global instances in a local
// map and setup a pre routine (commitGlobalInstances) that will run in the
// QApplication constructor and commit all the staged instances to the
// QApplication properties.
//     Note: One of the side effects of this, is that no DLL loaded before
// the QApplication is constructed, can expect to access the existing staged
// global instanced. For this reason, we advise all DLLs be loaded after
// the QApplication is instanced.
static std::mutex stagedGlobalInstancesMutex;
static std::unordered_map<std::string, QVariant> stagedGlobalInstances;

std::mutex& globalInstancesMutex() {
    return stagedGlobalInstancesMutex;
}

static void commitGlobalInstances() {
    std::unique_lock<std::mutex> lock(globalInstancesMutex());
    for (const auto& it : stagedGlobalInstances) {
        qApp->setProperty(it.first.c_str(), it.second);
    }
    stagedGlobalInstances.clear();
}

// This call is necessary for global instances to work across DLL boundaries
// Ideally, this founction would be called at the top of the main function.
// See description at the top of the file.
void setupGlobalInstances() {
    qAddPreRoutine(commitGlobalInstances);
}

QVariant getGlobalInstance(const char* propertyName) {
    if (qApp) {
        return qApp->property(propertyName);
    } else {
        auto it = stagedGlobalInstances.find(propertyName);
        if (it != stagedGlobalInstances.end()) {
            return it->second;
        }
    }
    return QVariant();
}

void setGlobalInstance(const char* propertyName, const QVariant& variant) {
    if (qApp) {
        qApp->setProperty(propertyName, variant);
    } else {
        stagedGlobalInstances[propertyName] = variant;
    }
}

static qint64 usecTimestampNowAdjust = 0; // in usec
void usecTimestampNowForceClockSkew(qint64 clockSkew) {
    ::usecTimestampNowAdjust = clockSkew;
}

quint64 usecTimestampNow(bool wantDebug) {
    using namespace std::chrono;
    static const auto unixEpoch = system_clock::from_time_t(0);
    return duration_cast<microseconds>(system_clock::now() - unixEpoch).count() + usecTimestampNowAdjust;
}

float secTimestampNow() {
    static const auto START_TIME = usecTimestampNow();
    const auto nowUsecs = usecTimestampNow() - START_TIME;
    const auto nowMsecs = nowUsecs / USECS_PER_MSEC;
    return (float)nowMsecs / MSECS_PER_SECOND;
}

float randFloat() {
    return (rand() % 10000)/10000.0f;
}

int randIntInRange (int min, int max) {
    return min + (rand() % ((max + 1) - min));
}

float randFloatInRange (float min,float max) {
    return min + ((rand() % 10000)/10000.0f * (max-min));
}

float randomSign() {
    return randomBoolean() ? -1.0 : 1.0;
}

unsigned char randomColorValue(int miniumum) {
    return miniumum + (rand() % (256 - miniumum));
}

bool randomBoolean() {
    return rand() % 2;
}

bool shouldDo(float desiredInterval, float deltaTime) {
    return randFloat() < deltaTime / desiredInterval;
}

void outputBufferBits(const unsigned char* buffer, int length, QDebug* continuedDebug) {
    for (int i = 0; i < length; i++) {
        outputBits(buffer[i], continuedDebug);
    }
}

void outputBits(unsigned char byte, QDebug* continuedDebug) {
    QDebug debug = qDebug().nospace();

    if (continuedDebug) {
        debug = *continuedDebug;
        debug.nospace();
    }

    QString resultString;

    if (isalnum(byte)) {
        resultString.sprintf("[ %d (%c): ", byte, byte);
    } else {
        resultString.sprintf("[ %d (0x%x): ", byte, byte);
    }
    debug << qPrintable(resultString);
    
    for (int i = 0; i < 8; i++) {
        resultString.sprintf("%d", byte >> (7 - i) & 1);
        debug << qPrintable(resultString);
    }
    debug << " ]";
}

int numberOfOnes(unsigned char byte) {

    static const int nbits[256] = {
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,
        4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,
        4,5,3,4,4,5,4,5,5,6,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,
        3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,
        4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,1,2,2,3,2,3,3,
        4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,
        3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,
        6,6,7,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,
        4,5,5,6,5,6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,
        6,5,6,6,7,5,6,6,7,6,7,7,8
    };

    return nbits[(unsigned char) byte];

}

bool oneAtBit(unsigned char byte, int bitIndex) {
    return (byte >> (7 - bitIndex) & 1);
}

void setAtBit(unsigned char& byte, int bitIndex) {
    byte |= (1 << (7 - bitIndex));
}

bool oneAtBit16(unsigned short word, int bitIndex) {
    return (word >> (15 - bitIndex) & 1);
}

void setAtBit16(unsigned short& word, int bitIndex) {
    word |= (1 << (15 - bitIndex));
}


void clearAtBit(unsigned char& byte, int bitIndex) {
    if (oneAtBit(byte, bitIndex)) {
        byte -= (1 << (7 - bitIndex));
    }
}

int  getSemiNibbleAt(unsigned short word, int bitIndex) {
    return (word >> (14 - bitIndex) & 3); // semi-nibbles store 00, 01, 10, or 11
}

int getNthBit(unsigned char byte, int ordinal) {
    const int ERROR_RESULT = -1;
    const int MIN_ORDINAL = 1;
    const int MAX_ORDINAL = 8;
    if (ordinal < MIN_ORDINAL || ordinal > MAX_ORDINAL) {
        return ERROR_RESULT;
    }
    int bitsSet = 0;
    for (int bitIndex = 0; bitIndex < MAX_ORDINAL; bitIndex++) {
        if (oneAtBit(byte, bitIndex)) {
            bitsSet++;
        }
        if (bitsSet == ordinal) {
            return bitIndex;
        }
    }
    return ERROR_RESULT;
}

void setSemiNibbleAt(unsigned short& word, int bitIndex, int value) {
    //assert(value <= 3 && value >= 0);
    word |= ((value & 3) << (14 - bitIndex)); // semi-nibbles store 00, 01, 10, or 11
}

bool isInEnvironment(const char* environment) {
    char* environmentString = getenv("HIFI_ENVIRONMENT");
    return (environmentString && strcmp(environmentString, environment) == 0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    getCmdOption()
// Description: Handy little function to tell you if a command line flag and option was
//              included while launching the application, and to get the option value
//              immediately following the flag. For example if you ran:
//                      ./app -i filename.txt
//              then you're using the "-i" flag to set the input file name.
// Usage:       char * inputFilename = getCmdOption(argc, argv, "-i");
// Complaints:  Brad :)
const char* getCmdOption(int argc, const char * argv[],const char* option) {
    // check each arg
    for (int i=0; i < argc; i++) {
        // if the arg matches the desired option
        if (strcmp(option,argv[i])==0 && i+1 < argc) {
            // then return the next option
            return argv[i+1];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    getCmdOption()
// Description: Handy little function to tell you if a command line option flag was
//              included while launching the application. Returns bool true/false
// Usage:       bool wantDump   = cmdOptionExists(argc, argv, "-d");
// Complaints:  Brad :)

bool cmdOptionExists(int argc, const char * argv[],const char* option) {
    // check each arg
    for (int i=0; i < argc; i++) {
        // if the arg matches the desired option
        if (strcmp(option,argv[i])==0) {
            // then return the next option
            return true;
        }
    }
    return false;
}

void sharedMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message) {
    fprintf(stdout, "%s", message.toLocal8Bit().constData());
}

unsigned char* pointToOctalCode(float x, float y, float z, float s) {
    return pointToVoxel(x, y, z, s);
}

/// Given a universal point with location x,y,z this will return the voxel
/// voxel code corresponding to the closest voxel which encloses a cube with
/// lower corners at x,y,z, having side of length S.
/// The input values x,y,z range 0.0 <= v < 1.0
/// IMPORTANT: The voxel is returned to you a buffer which you MUST delete when you are
/// done with it.
unsigned char* pointToVoxel(float x, float y, float z, float s, unsigned char r, unsigned char g, unsigned char b ) {

    // special case for size 1, the root node
    if (s >= 1.0f) {
        unsigned char* voxelOut = new unsigned char;
        *voxelOut = 0;
        return voxelOut;
    }

    float xTest, yTest, zTest, sTest;
    xTest = yTest = zTest = sTest = 0.5f;

    // First determine the voxelSize that will properly encode a
    // voxel of size S.
    unsigned int voxelSizeInOctets = 1;
    while (sTest > s) {
        sTest /= 2.0f;
        voxelSizeInOctets++;
    }

    auto voxelSizeInBytes = bytesRequiredForCodeLength(voxelSizeInOctets); // (voxelSizeInBits/8)+1;
    auto voxelBufferSize = voxelSizeInBytes + sizeof(glm::u8vec3); // 3 for color

    // allocate our resulting buffer
    unsigned char* voxelOut = new unsigned char[voxelBufferSize];

    // first byte of buffer is always our size in octets
    voxelOut[0]=voxelSizeInOctets;

    sTest = 0.5f; // reset sTest so we can do this again.

    unsigned char byte = 0; // we will be adding coding bits here
    int bitInByteNDX = 0; // keep track of where we are in byte as we go
    int byteNDX = 1; // keep track of where we are in buffer of bytes as we go
    unsigned int octetsDone = 0;

    // Now we actually fill out the voxel code
    while (octetsDone < voxelSizeInOctets) {
        if (x >= xTest) {
            //<write 1 bit>
            byte = (byte << 1) | true;
            xTest += sTest/2.0f;
        } else {
            //<write 0 bit;>
            byte = (byte << 1) | false;
            xTest -= sTest/2.0f;
        }
        bitInByteNDX++;
        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if (bitInByteNDX == 8) {
            voxelOut[byteNDX]=byte;
            byteNDX++;
            bitInByteNDX=0;
            byte=0;
        }

        if (y >= yTest) {
            //<write 1 bit>
            byte = (byte << 1) | true;
            yTest += sTest/2.0f;
        } else {
            //<write 0 bit;>
            byte = (byte << 1) | false;
            yTest -= sTest/2.0f;
        }
        bitInByteNDX++;
        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if (bitInByteNDX == 8) {
            voxelOut[byteNDX]=byte;
            byteNDX++;
            bitInByteNDX=0;
            byte=0;
        }

        if (z >= zTest) {
            //<write 1 bit>
            byte = (byte << 1) | true;
            zTest += sTest/2.0f;
        } else {
            //<write 0 bit;>
            byte = (byte << 1) | false;
            zTest -= sTest/2.0f;
        }
        bitInByteNDX++;
        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if (bitInByteNDX == 8) {
            voxelOut[byteNDX]=byte;
            byteNDX++;
            bitInByteNDX=0;
            byte=0;
        }

        octetsDone++;
        sTest /= 2.0f;
    }

    // If we've got here, and we didn't fill the last byte, we need to zero pad this
    // byte before we copy it into our buffer.
    if (bitInByteNDX > 0 && bitInByteNDX < 8) {
        // Pad the last byte
        while (bitInByteNDX < 8) {
            byte = (byte << 1) | false;
            bitInByteNDX++;
        }

        // Copy it into our output buffer
        voxelOut[byteNDX]=byte;
        byteNDX++;
    }
    // copy color data
    voxelOut[byteNDX]=r;
    voxelOut[byteNDX+1]=g;
    voxelOut[byteNDX+2]=b;

    return voxelOut;
}

void printVoxelCode(unsigned char* voxelCode) {
    unsigned char octets = voxelCode[0];
    unsigned int voxelSizeInBits = octets*3;
    unsigned int voxelSizeInBytes = (voxelSizeInBits/8)+1;
    unsigned int voxelSizeInOctets = (voxelSizeInBits/3);
    unsigned int voxelBufferSize = voxelSizeInBytes+1+3; // 1 for size, 3 for color

    qCDebug(shared, "octets=%d",octets);
    qCDebug(shared, "voxelSizeInBits=%d",voxelSizeInBits);
    qCDebug(shared, "voxelSizeInBytes=%d",voxelSizeInBytes);
    qCDebug(shared, "voxelSizeInOctets=%d",voxelSizeInOctets);
    qCDebug(shared, "voxelBufferSize=%d",voxelBufferSize);

    for(unsigned int i=0; i < voxelBufferSize; i++) {
        QDebug voxelBufferDebug = qDebug();
        voxelBufferDebug << "i =" << i;
        outputBits(voxelCode[i], &voxelBufferDebug);
    }
}

#ifdef _WIN32
void usleep(int waitTime) {
    // Use QueryPerformanceCounter for least overhead
    LARGE_INTEGER now; // ticks
    QueryPerformanceCounter(&now);

    static int64_t ticksPerSec = 0;
    if (ticksPerSec == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        ticksPerSec = frequency.QuadPart;
    }

    // order ops to avoid loss in precision
    int64_t waitTicks = (ticksPerSec * waitTime) / USECS_PER_SECOND;
    int64_t sleepTicks = now.QuadPart + waitTicks;

    // Busy wait with sleep/yield where possible
    while (true) {
        QueryPerformanceCounter(&now);
        if (now.QuadPart >= sleepTicks) {
            break;
        }

        // Sleep if we have at least 1ms to spare
        const int64_t MIN_SLEEP_USECS = 1000;
        // msleep is allowed to overshoot, so give it a 100us berth
        const int64_t MIN_SLEEP_USECS_BERTH = 100;
        // order ops to avoid loss in precision
        int64_t sleepFor = ((sleepTicks - now.QuadPart) * USECS_PER_SECOND) / ticksPerSec - MIN_SLEEP_USECS_BERTH;
        if (sleepFor > MIN_SLEEP_USECS) {
            Sleep((DWORD)(sleepFor / USECS_PER_MSEC));
        // Yield otherwise
        } else {
            // Use Qt to delegate, as SwitchToThread is only supported starting with XP
            QThread::yieldCurrentThread();
        }
    }
}
#endif

// Inserts the value and key into three arrays sorted by the key array, the first array is the value,
// the second array is a sorted key for the value, the third array is the index for the value in it original
// non-sorted array
// returns -1 if size exceeded
// originalIndexArray is optional
int insertIntoSortedArrays(void* value, float key, int originalIndex,
                           void** valueArray, float* keyArray, int* originalIndexArray,
                           int currentCount, int maxCount) {

    if (currentCount < maxCount) {
        int i = 0;
        if (currentCount > 0) {
            while (i < currentCount && key > keyArray[i]) {
                i++;
            }
            // i is our desired location
            // shift array elements to the right
            if (i < currentCount && i+1 < maxCount) {
                memmove(&valueArray[i + 1], &valueArray[i], sizeof(void*) * (currentCount - i));
                memmove(&keyArray[i + 1], &keyArray[i], sizeof(float) * (currentCount - i));
                if (originalIndexArray) {
                    memmove(&originalIndexArray[i + 1], &originalIndexArray[i], sizeof(int) * (currentCount - i));
                }
            }
        }
        // place new element at i
        valueArray[i] = value;
        keyArray[i] = key;
        if (originalIndexArray) {
            originalIndexArray[i] = originalIndex;
        }
        return currentCount + 1;
    }
    return -1; // error case
}

int removeFromSortedArrays(void* value, void** valueArray, float* keyArray, int* originalIndexArray,
                           int currentCount, int maxCount) {

    int i = 0;
    if (currentCount > 0) {
        while (i < currentCount && value != valueArray[i]) {
            i++;
        }

        if (value == valueArray[i] && i < currentCount) {
            // i is the location of the item we were looking for
            // shift array elements to the left
            memmove(&valueArray[i], &valueArray[i + 1], sizeof(void*) * ((currentCount-1) - i));
            memmove(&keyArray[i], &keyArray[i + 1], sizeof(float) * ((currentCount-1) - i));
            if (originalIndexArray) {
                memmove(&originalIndexArray[i], &originalIndexArray[i + 1], sizeof(int) * ((currentCount-1) - i));
            }
            return currentCount-1;
        }
    }
    return -1; // error case
}

float SMALL_LIMIT = 10.0f;
float LARGE_LIMIT = 1000.0f;

int packFloatRatioToTwoByte(unsigned char* buffer, float ratio) {
    // if the ratio is less than 10, then encode it as a positive number scaled from 0 to int16::max()
    int16_t ratioHolder;

    if (ratio < SMALL_LIMIT) {
        const float SMALL_RATIO_CONVERSION_RATIO = (std::numeric_limits<int16_t>::max() / SMALL_LIMIT);
        ratioHolder = floorf(ratio * SMALL_RATIO_CONVERSION_RATIO);
    } else {
        const float LARGE_RATIO_CONVERSION_RATIO = std::numeric_limits<int16_t>::min() / LARGE_LIMIT;
        ratioHolder = floorf((std::min(ratio,LARGE_LIMIT) - SMALL_LIMIT) * LARGE_RATIO_CONVERSION_RATIO);
    }
    memcpy(buffer, &ratioHolder, sizeof(ratioHolder));
    return sizeof(ratioHolder);
}

int unpackFloatRatioFromTwoByte(const unsigned char* buffer, float& ratio) {
    int16_t ratioHolder;
    memcpy(&ratioHolder, buffer, sizeof(ratioHolder));

    // If it's positive, than the original ratio was less than SMALL_LIMIT
    if (ratioHolder > 0) {
        ratio = (ratioHolder / (float) std::numeric_limits<int16_t>::max()) * SMALL_LIMIT;
    } else {
        // If it's negative, than the original ratio was between SMALL_LIMIT and LARGE_LIMIT
        ratio = ((ratioHolder / (float) std::numeric_limits<int16_t>::min()) * LARGE_LIMIT) + SMALL_LIMIT;
    }
    return sizeof(ratioHolder);
}

int packClipValueToTwoByte(unsigned char* buffer, float clipValue) {
    // Clip values must be less than max signed 16bit integers
    assert(clipValue < std::numeric_limits<int16_t>::max());
    int16_t holder;

    // if the clip is less than 10, then encode it as a positive number scaled from 0 to int16::max()
    if (clipValue < SMALL_LIMIT) {
        const float SMALL_RATIO_CONVERSION_RATIO = (std::numeric_limits<int16_t>::max() / SMALL_LIMIT);
        holder = floorf(clipValue * SMALL_RATIO_CONVERSION_RATIO);
    } else {
        // otherwise we store it as a negative integer
        holder = -1 * floorf(clipValue);
    }
    memcpy(buffer, &holder, sizeof(holder));
    return sizeof(holder);
}

int unpackClipValueFromTwoByte(const unsigned char* buffer, float& clipValue) {
    int16_t holder;
    memcpy(&holder, buffer, sizeof(holder));

    // If it's positive, than the original clipValue was less than SMALL_LIMIT
    if (holder > 0) {
        clipValue = (holder / (float) std::numeric_limits<int16_t>::max()) * SMALL_LIMIT;
    } else {
        // If it's negative, than the original holder can be found as the opposite sign of holder
        clipValue = -1.0f * holder;
    }
    return sizeof(holder);
}

int packFloatToByte(unsigned char* buffer, float value, float scaleBy) {
    quint8 holder;
    const float CONVERSION_RATIO = (255 / scaleBy);
    holder = floorf(value * CONVERSION_RATIO);
    memcpy(buffer, &holder, sizeof(holder));
    return sizeof(holder);
}

int unpackFloatFromByte(const unsigned char* buffer, float& value, float scaleBy) {
    quint8 holder;
    memcpy(&holder, buffer, sizeof(holder));
    value = ((float)holder / (float) 255) * scaleBy;
    return sizeof(holder);
}

unsigned char debug::DEADBEEF[] = { 0xDE, 0xAD, 0xBE, 0xEF };
int debug::DEADBEEF_SIZE = sizeof(DEADBEEF);
void debug::setDeadBeef(void* memoryVoid, int size) {
    unsigned char* memoryAt = (unsigned char*)memoryVoid;
    int deadBeefSet = 0;
    int chunks = size / DEADBEEF_SIZE;
    for (int i = 0; i < chunks; i++) {
        memcpy(memoryAt + (i * DEADBEEF_SIZE), DEADBEEF, DEADBEEF_SIZE);
        deadBeefSet += DEADBEEF_SIZE;
    }
    memcpy(memoryAt + deadBeefSet, DEADBEEF, size - deadBeefSet);
}

void debug::checkDeadBeef(void* memoryVoid, int size) {
    assert(memcmp((unsigned char*)memoryVoid, DEADBEEF, std::min(size, DEADBEEF_SIZE)) != 0);
}


// glm::abs() works for signed or unsigned types
template <typename T>
QString formatUsecTime(T usecs) {
    static const int PRECISION = 3;
    static const int FRACTION_MASK = pow(10, PRECISION);

    static const T USECS_PER_MSEC = 1000;
    static const T USECS_PER_SECOND = 1000 * USECS_PER_MSEC;
    static const T USECS_PER_MINUTE = USECS_PER_SECOND * 60;
    static const T USECS_PER_HOUR = USECS_PER_MINUTE * 60;

    QString result;
    if (glm::abs(usecs) > USECS_PER_HOUR) {
        if (std::is_integral<T>::value) {
            result = QString::number(usecs / USECS_PER_HOUR);
            result += "." + QString::number(((int)(usecs * FRACTION_MASK / USECS_PER_HOUR)) % FRACTION_MASK);
        } else {
            result = QString::number(usecs / USECS_PER_HOUR, 'f', PRECISION);
        }
        result += " hrs";
    } else if (glm::abs(usecs) > USECS_PER_MINUTE) {
        if (std::is_integral<T>::value) {
            result = QString::number(usecs / USECS_PER_MINUTE);
            result += "." + QString::number(((int)(usecs * FRACTION_MASK / USECS_PER_MINUTE)) % FRACTION_MASK);
        } else {
            result = QString::number(usecs / USECS_PER_MINUTE, 'f', PRECISION);
        }
        result += " mins";
    } else if (glm::abs(usecs) > USECS_PER_SECOND) {
        if (std::is_integral<T>::value) {
            result = QString::number(usecs / USECS_PER_SECOND);
            result += "." + QString::number(((int)(usecs * FRACTION_MASK / USECS_PER_SECOND)) % FRACTION_MASK);
        } else {
            result = QString::number(usecs / USECS_PER_SECOND, 'f', PRECISION);
        }
        result += " secs";
    } else if (glm::abs(usecs) > USECS_PER_MSEC) {
        if (std::is_integral<T>::value) {
            result = QString::number(usecs / USECS_PER_MSEC);
            result += "." + QString::number(((int)(usecs * FRACTION_MASK / USECS_PER_MSEC)) % FRACTION_MASK);
        } else {
            result = QString::number(usecs / USECS_PER_MSEC, 'f', PRECISION);
        }
        result += " msecs";
    } else {
        result = QString::number(usecs) + " usecs";
    }
    return result;
}


QString formatUsecTime(quint64 usecs) {
    return formatUsecTime<quint64>(usecs);
}

QString formatUsecTime(qint64 usecs) {
    return formatUsecTime<qint64>(usecs);
}

QString formatUsecTime(float usecs) {
    return formatUsecTime<float>(usecs);
}

QString formatUsecTime(double usecs) {
    return formatUsecTime<double>(usecs);
}

QString formatSecTime(qint64 secs) {
    return formatUsecTime(secs * 1000000);
}


QString formatSecondsElapsed(float seconds) {
    QString result;

    const float SECONDS_IN_DAY = 60.0f * 60.0f * 24.0f;        
    if (seconds > SECONDS_IN_DAY) {
        float days = floor(seconds / SECONDS_IN_DAY);
        float rest = seconds - (days * SECONDS_IN_DAY);
        result = QString::number((int)days);
        if (days > 1.0f) {
            result += " days ";
        } else {
            result += " day ";
        }
        result += QDateTime::fromTime_t(rest).toUTC().toString("h 'hours' m 'minutes' s 'seconds'");
    } else {
        result = QDateTime::fromTime_t(seconds).toUTC().toString("h 'hours' m 'minutes' s 'seconds'");
    }
    return result;
}

bool similarStrings(const QString& stringA, const QString& stringB) {
    QStringList aWords = stringA.split(" ");
    QStringList bWords = stringB.split(" ");
    float aWordsInB = 0.0f;
    foreach(QString aWord, aWords) {
        if (bWords.contains(aWord)) {
            aWordsInB += 1.0f;
        }
    }
    float bWordsInA = 0.0f;
    foreach(QString bWord, bWords) {
        if (aWords.contains(bWord)) {
            bWordsInA += 1.0f;
        }
    }
    float similarity = 0.5f * (aWordsInB / (float)bWords.size()) + 0.5f * (bWordsInA / (float)aWords.size());
    const float SIMILAR_ENOUGH = 0.5f; // half the words the same is similar enough for us
    return similarity >= SIMILAR_ENOUGH;
}

void disableQtBearerPoll() {
    // To disable the Qt constant wireless scanning, set the env for polling interval to -1
    // The constant polling causes ping spikes on windows every 10 seconds or so that affect the audio
    const QByteArray DISABLE_BEARER_POLL_TIMEOUT = QString::number(-1).toLocal8Bit();
    qputenv("QT_BEARER_POLL_TIMEOUT", DISABLE_BEARER_POLL_TIMEOUT);
}

void printSystemInformation() {
    // Write system information to log
    qCDebug(shared) << "Build Information";
    qCDebug(shared).noquote() << "\tBuild ABI: " << QSysInfo::buildAbi();
    qCDebug(shared).noquote() << "\tBuild CPU Architecture: " << QSysInfo::buildCpuArchitecture();

    qCDebug(shared).noquote() << "System Information";
    qCDebug(shared).noquote() << "\tProduct Name: " << QSysInfo::prettyProductName();
    qCDebug(shared).noquote() << "\tCPU Architecture: " << QSysInfo::currentCpuArchitecture();
    qCDebug(shared).noquote() << "\tKernel Type: " << QSysInfo::kernelType();
    qCDebug(shared).noquote() << "\tKernel Version: " << QSysInfo::kernelVersion();

    qCDebug(shared) << "\tOS Version: " << QOperatingSystemVersion::current();

#ifdef Q_OS_WIN
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);

    qCDebug(shared) << "SYSTEM_INFO";
    qCDebug(shared).noquote() << "\tOEM ID: " << si.dwOemId;
    qCDebug(shared).noquote() << "\tProcessor Architecture: " << si.wProcessorArchitecture;
    qCDebug(shared).noquote() << "\tProcessor Type: " << si.dwProcessorType;
    qCDebug(shared).noquote() << "\tProcessor Level: " << si.wProcessorLevel;
    qCDebug(shared).noquote() << "\tProcessor Revision: "
                       << QString("0x%1").arg(si.wProcessorRevision, 4, 16, QChar('0'));
    qCDebug(shared).noquote() << "\tNumber of Processors: " << si.dwNumberOfProcessors;
    qCDebug(shared).noquote() << "\tPage size: " << si.dwPageSize << " Bytes";
    qCDebug(shared).noquote() << "\tMin Application Address: "
                       << QString("0x%1").arg(qulonglong(si.lpMinimumApplicationAddress), 16, 16, QChar('0'));
    qCDebug(shared).noquote() << "\tMax Application Address: "
                       << QString("0x%1").arg(qulonglong(si.lpMaximumApplicationAddress), 16, 16, QChar('0'));

    const double BYTES_TO_MEGABYTE = 1.0 / (1024 * 1024);

    qCDebug(shared) << "MEMORYSTATUSEX";
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        qCDebug(shared).noquote()
            << QString("\tCurrent System Memory Usage: %1%").arg(ms.dwMemoryLoad);
        qCDebug(shared).noquote()
            << QString("\tAvail Physical Memory: %1 MB").arg(ms.ullAvailPhys * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(shared).noquote()
            << QString("\tTotal Physical Memory: %1 MB").arg(ms.ullTotalPhys * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(shared).noquote()
            << QString("\tAvail in Page File:    %1 MB").arg(ms.ullAvailPageFile * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(shared).noquote()
            << QString("\tTotal in Page File:    %1 MB").arg(ms.ullTotalPageFile * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(shared).noquote()
            << QString("\tAvail Virtual Memory:  %1 MB").arg(ms.ullAvailVirtual * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(shared).noquote()
            << QString("\tTotal Virtual Memory:  %1 MB").arg(ms.ullTotalVirtual * BYTES_TO_MEGABYTE, 20, 'f', 2);
    } else {
        qCDebug(shared) << "\tFailed to retrieve memory status: " << GetLastError();
    }

    qCDebug(shared) << "CPUID";

    qCDebug(shared) << "\tCPU Vendor: " << CPUIdent::Vendor().c_str();
    qCDebug(shared) << "\tCPU Brand:  " << CPUIdent::Brand().c_str();

    for (auto& feature : CPUIdent::getAllFeatures()) {
        qCDebug(shared).nospace().noquote() << "\t[" << (feature.supported ? "x" : " ") << "] " << feature.name.c_str();
    }
#endif

    qCDebug(shared) << "Environment Variables";
    // List of env variables to include in the log. For privacy reasons we don't send all env variables.
    const QStringList envWhitelist = {
        "QTWEBENGINE_REMOTE_DEBUGGING"
    };
    auto envVariables = QProcessEnvironment::systemEnvironment();
    for (auto& env : envWhitelist)
    {
        qCDebug(shared).noquote().nospace() << "\t" <<
            (envVariables.contains(env) ? " = " + envVariables.value(env) : " NOT FOUND");
    }
}

bool getMemoryInfo(MemoryInfo& info) {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) {
        return false;
    }

    info.totalMemoryBytes = ms.ullTotalPhys;
    info.availMemoryBytes = ms.ullAvailPhys;
    info.usedMemoryBytes = ms.ullTotalPhys - ms.ullAvailPhys;


    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        return false;
    }
    info.processUsedMemoryBytes = pmc.PrivateUsage;
    info.processPeakUsedMemoryBytes = pmc.PeakPagefileUsage;

    return true;
#endif

    return false;
}

// Largely taken from: https://msdn.microsoft.com/en-us/library/windows/desktop/ms683194(v=vs.85).aspx

#ifdef Q_OS_WIN
using LPFN_GLPI = BOOL(WINAPI*)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
    PDWORD);

DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
    DWORD i;

    for (i = 0; i <= LSHIFT; ++i) {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}
#endif

bool getProcessorInfo(ProcessorInfo& info) {

#ifdef Q_OS_WIN
    LPFN_GLPI glpi;
    bool done = false;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD returnLength = 0;
    DWORD logicalProcessorCount = 0;
    DWORD numaNodeCount = 0;
    DWORD processorCoreCount = 0;
    DWORD processorL1CacheCount = 0;
    DWORD processorL2CacheCount = 0;
    DWORD processorL3CacheCount = 0;
    DWORD processorPackageCount = 0;
    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR Cache;

    glpi = (LPFN_GLPI)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),
        "GetLogicalProcessorInformation");
    if (nullptr == glpi) {
        qCDebug(shared) << "GetLogicalProcessorInformation is not supported.";
        return false;
    }

    while (!done) {
        DWORD rc = glpi(buffer, &returnLength);

        if (FALSE == rc) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if (buffer) {
                    free(buffer);
                }

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                    returnLength);

                if (NULL == buffer) {
                    qCDebug(shared) << "Error: Allocation failure";
                    return false;
                }
            } else {
                qCDebug(shared) << "Error " << GetLastError();
                return false;
            }
        } else {
            done = true;
        }
    }

    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) {
        switch (ptr->Relationship) {
        case RelationNumaNode:
            // Non-NUMA systems report a single record of this type.
            numaNodeCount++;
            break;

        case RelationProcessorCore:
            processorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;

        case RelationCache:
            // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
            Cache = &ptr->Cache;
            if (Cache->Level == 1) {
                processorL1CacheCount++;
            } else if (Cache->Level == 2) {
                processorL2CacheCount++;
            } else if (Cache->Level == 3) {
                processorL3CacheCount++;
            }
            break;

        case RelationProcessorPackage:
            // Logical processors share a physical package.
            processorPackageCount++;
            break;

        default:
            qCDebug(shared) << "\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n";
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    qCDebug(shared) << "GetLogicalProcessorInformation results:";
    qCDebug(shared) << "Number of NUMA nodes:" << numaNodeCount;
    qCDebug(shared) << "Number of physical processor packages:" << processorPackageCount;
    qCDebug(shared) << "Number of processor cores:" << processorCoreCount;
    qCDebug(shared) << "Number of logical processors:" << logicalProcessorCount;
    qCDebug(shared) << "Number of processor L1/L2/L3 caches:"
        << processorL1CacheCount
        << "/" << processorL2CacheCount
        << "/" << processorL3CacheCount;

    info.numPhysicalProcessorPackages = processorPackageCount;
    info.numProcessorCores = processorCoreCount;
    info.numLogicalProcessors = logicalProcessorCount;
    info.numProcessorCachesL1 = processorL1CacheCount;
    info.numProcessorCachesL2 = processorL2CacheCount;
    info.numProcessorCachesL3 = processorL3CacheCount;

    free(buffer);

    return true;
#endif

    return false;
}


const QString& getInterfaceSharedMemoryName() {
    static const QString applicationName = "High Fidelity Interface - " + qgetenv("USERNAME");
    return applicationName;
}

const std::vector<uint8_t>& getAvailableCores() {
    static std::vector<uint8_t> availableCores;
#ifdef Q_OS_WIN
    static std::once_flag once;
    std::call_once(once, [&] {
        DWORD_PTR defaultProcessAffinity = 0, defaultSystemAffinity = 0;
        HANDLE process = GetCurrentProcess();
        GetProcessAffinityMask(process, &defaultProcessAffinity, &defaultSystemAffinity);
        for (uint64_t i = 0; i < sizeof(DWORD_PTR) * BITS_IN_BYTE; ++i) {
            DWORD_PTR coreMask = 1;
            coreMask <<= i;
            if (0 != (defaultSystemAffinity & coreMask)) {
                availableCores.push_back(i);
            }
        }
    });
#endif
    return availableCores;
}

void setMaxCores(uint8_t maxCores) {
#ifdef Q_OS_WIN
    HANDLE process = GetCurrentProcess();
    auto availableCores = getAvailableCores();
    if (availableCores.size() <= maxCores) {
        DWORD_PTR currentProcessAffinity = 0, currentSystemAffinity = 0;
        GetProcessAffinityMask(process, &currentProcessAffinity, &currentSystemAffinity);
        SetProcessAffinityMask(GetCurrentProcess(), currentSystemAffinity);
        return;
    }

    DWORD_PTR newProcessAffinity = 0;
    while (maxCores) {
        int index = randIntInRange(0, (int)availableCores.size() - 1);
        DWORD_PTR coreMask = 1;
        coreMask <<= availableCores[index];
        newProcessAffinity |= coreMask;
        availableCores.erase(availableCores.begin() + index);
        maxCores--;
    }
    SetProcessAffinityMask(process, newProcessAffinity);
#endif
}

bool processIsRunning(int64_t pid) {
#ifdef Q_OS_WIN
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process) {
        DWORD exitCode;
        if (GetExitCodeProcess(process, &exitCode) != 0) {
            return exitCode == STILL_ACTIVE;
        }
    }
    return false;
#else
    if (kill(pid, 0) == -1) {
        return errno != ESRCH;
    }
    return true;
#endif
}

void quitWithParentProcess() {
    if (qApp) {
        qDebug() << "Parent process died, quitting";
        exit(0);
    }
}

#ifdef Q_OS_WIN
VOID CALLBACK parentDiedCallback(PVOID lpParameter, BOOLEAN timerOrWaitFired) {
    if (!timerOrWaitFired) {
        quitWithParentProcess();
    }
}

void watchParentProcess(int parentPID) {
    DWORD processID = parentPID;
    HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);

    HANDLE newHandle;
    RegisterWaitForSingleObject(&newHandle, procHandle, parentDiedCallback, NULL, INFINITE, WT_EXECUTEONLYONCE);
}
#elif defined(Q_OS_MAC) || defined(Q_OS_LINUX)
void watchParentProcess(int parentPID) {
    auto timer = new QTimer(qApp);
    timer->setInterval(MSECS_PER_SECOND);
    QObject::connect(timer, &QTimer::timeout, qApp, [parentPID]() {
        auto ppid = getppid();
        if (parentPID != ppid) {
            // If the PPID changed, then that means our parent process died.
            quitWithParentProcess();
        }
    });
    timer->start();
}
#endif

void setupHifiApplication(QString applicationName) {
    disableQtBearerPoll(); // Fixes wifi ping spikes

    // Those calls are necessary to format the log correctly
    // and to direct the application to the correct location
    // for read/writes into AppData and other platform equivalents.
    QCoreApplication::setApplicationName(applicationName);
    QCoreApplication::setOrganizationName(BuildInfo::MODIFIED_ORGANIZATION);
    QCoreApplication::setOrganizationDomain(BuildInfo::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationVersion(BuildInfo::VERSION);

    // This ensures the global instances mechanism is correctly setup.
    // You can find more details as to why this is important in the SharedUtil.h/cpp files
    setupGlobalInstances();

#ifndef WIN32
    // Windows tends to hold onto log lines until it has a sizeable buffer
    // This makes the log feel unresponsive and trap useful log data in the log buffer
    // when a crash occurs.
    //Force windows to flush the buffer on each new line character to avoid this.
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif

    // Install the standard hifi message handler so we get consistant log formatting
    qInstallMessageHandler(LogHandler::verboseMessageHandler);
}

#ifdef Q_OS_WIN
QString getLastErrorAsString() {
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return QString();
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

    auto message = QString::fromLocal8Bit(messageBuffer, (int)size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

// All processes in the group will shut down with the process creating the group
void* createProcessGroup() {
    HANDLE jobObject = CreateJobObject(nullptr, nullptr);
    if (jobObject == nullptr) {
        qWarning() << "Could NOT create job object:" << getLastErrorAsString();
        return nullptr;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION JELI;
    if (!QueryInformationJobObject(jobObject, JobObjectExtendedLimitInformation, &JELI, sizeof(JELI), nullptr)) {
        qWarning() << "Could NOT query job object information" << getLastErrorAsString();
        return nullptr;
    }
    JELI.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(jobObject, JobObjectExtendedLimitInformation, &JELI, sizeof(JELI))) {
        qWarning() << "Could NOT set job object information" << getLastErrorAsString();
        return nullptr;
    }

    return jobObject;
}

void addProcessToGroup(void* processGroup, qint64 processId) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == nullptr) {
        qCritical() << "Could NOT open process" << getLastErrorAsString();
    }
    if (!AssignProcessToJobObject(processGroup, hProcess)) {
        qCritical() << "Could NOT assign process to job object" << getLastErrorAsString();
    }
}

#endif
