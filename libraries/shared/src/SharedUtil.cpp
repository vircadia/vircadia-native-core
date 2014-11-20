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

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <QtCore/QDebug>
#include <QDateTime>
#include <QElapsedTimer>
#include <QThread>

#include "OctalCode.h"
#include "SharedUtil.h"

static int usecTimestampNowAdjust = 0; // in usec
void usecTimestampNowForceClockSkew(int clockSkew) {
    ::usecTimestampNowAdjust = clockSkew;
}

quint64 usecTimestampNow(bool wantDebug) {
    static bool usecTimestampNowIsInitialized = false;
    static qint64 TIME_REFERENCE = 0; // in usec
    static QElapsedTimer timestampTimer;
    
    if (!usecTimestampNowIsInitialized) {
        TIME_REFERENCE = QDateTime::currentMSecsSinceEpoch() * 1000; // ms to usec
        timestampTimer.start();
        usecTimestampNowIsInitialized = true;
    } else {
        // We've seen the QElapsedTimer begin to introduce dramatic errors if it's been
        // continuously running for a long time. So we will periodically reset it.
        const quint64 SECS_TO_NSECS = 1000000000;
        const quint64 RESET_AFTER_ELAPSED_SECS = 60 * 60; // 1 hour: 60 minutes * 60 seconds
        const quint64 RESET_AFTER_ELAPSED_NSECS = RESET_AFTER_ELAPSED_SECS * SECS_TO_NSECS; 
        const quint64 nsecsElapsed = timestampTimer.nsecsElapsed();
        if (nsecsElapsed > RESET_AFTER_ELAPSED_NSECS) {
            quint64 msecsElapsed = timestampTimer.restart();
            quint64 usecsElapsed = nsecsElapsed / 1000;  // nsec to usec
            TIME_REFERENCE += usecsElapsed;
            if (wantDebug) {
                qDebug() << "usecTimestampNow() - resetting QElapsedTimer. ";
                qDebug() << "    RESET_AFTER_ELAPSED_NSECS:" << RESET_AFTER_ELAPSED_NSECS;
                qDebug() << "    nsecsElapsed:" << nsecsElapsed;
                qDebug() << "    msecsElapsed:" << msecsElapsed;
                qDebug() << "    usecsElapsed:" << usecsElapsed;
            }
        }
    }
    
    quint64 nsecsElapsed = timestampTimer.nsecsElapsed();
    quint64 usecsElapsed = nsecsElapsed / 1000;  // nsec to usec
    quint64 now = TIME_REFERENCE + usecsElapsed + ::usecTimestampNowAdjust;
    
    if (wantDebug) {
        QDateTime currentLocalTime = QDateTime::currentDateTime();

        quint64 msecsNow = now / 1000; // usecs to msecs
        QDateTime nowAsString;
        nowAsString.setMSecsSinceEpoch(msecsNow);

        quint64 msecsTimeReference = TIME_REFERENCE / 1000; // usecs to msecs
        QDateTime timeReferenceAsString;
        timeReferenceAsString.setMSecsSinceEpoch(msecsTimeReference);

        qDebug() << "usecTimestampNow() - details... ";
        qDebug() << "           TIME_REFERENCE:" << TIME_REFERENCE;
        qDebug() << "    timeReferenceAsString:" << timeReferenceAsString.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "   usecTimestampNowAdjust:" << usecTimestampNowAdjust;
        qDebug() << "             nsecsElapsed:" << nsecsElapsed;
        qDebug() << "             usecsElapsed:" << usecsElapsed;
        qDebug() << "                      now:" << now;
        qDebug() << "                 msecsNow:" << msecsNow;
        qDebug() << "              nowAsString:" << nowAsString.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "         currentLocalTime:" << currentLocalTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    }
    
    return now;
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

void clearAtBit(unsigned char& byte, int bitIndex) {
    if (oneAtBit(byte, bitIndex)) {
        byte -= (1 << (7 - bitIndex));
    }
}

int  getSemiNibbleAt(unsigned char byte, int bitIndex) {
    return (byte >> (6 - bitIndex) & 3); // semi-nibbles store 00, 01, 10, or 11
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

bool isBetween(int64_t value, int64_t max, int64_t min) {
    return ((value <= max) && (value >= min));
}



void setSemiNibbleAt(unsigned char& byte, int bitIndex, int value) {
    //assert(value <= 3 && value >= 0);
    byte |= ((value & 3) << (6 - bitIndex)); // semi-nibbles store 00, 01, 10, or 11
}

bool isInEnvironment(const char* environment) {
    char* environmentString = getenv("HIFI_ENVIRONMENT");

    if (environmentString && strcmp(environmentString, environment) == 0) {
        return true;
    } else {
        return false;
    }
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

    unsigned int voxelSizeInBytes = bytesRequiredForCodeLength(voxelSizeInOctets); // (voxelSizeInBits/8)+1;
    unsigned int voxelBufferSize = voxelSizeInBytes + sizeof(rgbColor); // 3 for color

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

    qDebug("octets=%d",octets);
    qDebug("voxelSizeInBits=%d",voxelSizeInBits);
    qDebug("voxelSizeInBytes=%d",voxelSizeInBytes);
    qDebug("voxelSizeInOctets=%d",voxelSizeInOctets);
    qDebug("voxelBufferSize=%d",voxelBufferSize);

    for(unsigned int i=0; i < voxelBufferSize; i++) {
        QDebug voxelBufferDebug = qDebug();
        voxelBufferDebug << "i =" << i;
        outputBits(voxelCode[i], &voxelBufferDebug);
    }
}

#ifdef _WIN32
    void usleep(int waitTime) {
        const quint64 BUSY_LOOP_USECS = 2000;
        quint64 compTime = waitTime + usecTimestampNow();
        quint64 compTimeSleep = compTime - BUSY_LOOP_USECS;
        while (true) {
            if (usecTimestampNow() < compTimeSleep) {
                QThread::msleep(1);
            }
            if (usecTimestampNow() >= compTime) {
                break;
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

bool isNaN(float value) { 
    return value != value; 
}

QString formatUsecTime(float usecs, int prec) {
    static const quint64 SECONDS_PER_MINUTE = 60;
    static const quint64 USECS_PER_MINUTE = USECS_PER_SECOND * SECONDS_PER_MINUTE;

    QString result;
    if (usecs > USECS_PER_MINUTE) {
        result = QString::number(usecs / USECS_PER_MINUTE, 'f', prec) + "min";
    } else if (usecs > USECS_PER_SECOND) {
        result = QString::number(usecs / USECS_PER_SECOND, 'f', prec) + 's';
    } else if (usecs > USECS_PER_MSEC) {
        result = QString::number(usecs / USECS_PER_MSEC, 'f', prec) + "ms";
    } else {
        result = QString::number(usecs, 'f', prec) + "us";
    }
    return result;
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
