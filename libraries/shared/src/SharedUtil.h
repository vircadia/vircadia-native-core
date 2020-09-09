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
#include <mutex>
#include <math.h>
#include <stdint.h>

#ifndef _WIN32
#include <unistd.h> // not on windows, not needed for mac or windows
#endif

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QUuid>

#include "NumericalConstants.h"
// When writing out avatarEntities to a QByteArray, if the parentID is the ID of MyAvatar, use this ID instead.  This allows
// the value to be reset when the sessionID changes.
const QUuid AVATAR_SELF_ID = QUuid("{00000000-0000-0000-0000-000000000001}");

// Access to the global instance pointer to enable setting / unsetting
template <typename T>
std::unique_ptr<T>& globalInstancePointer() {
    static std::unique_ptr<T> instancePtr;
    return instancePtr;
}

// Sets up the global instances for use
// This NEEDS to be called on startup
// for any binary planing on using global instances
// More details in cpp file
void setupGlobalInstances();

std::mutex& globalInstancesMutex();
QVariant getGlobalInstance(const char* propertyName);
void setGlobalInstance(const char* propertyName, const QVariant& variant);

// Provides efficient access to a named global type.  By storing the value
// in the QApplication by name we can implement the singleton pattern and 
// have the single instance function across DLL boundaries.  
template <typename T, typename... Args>
T* globalInstance(const char* propertyName, Args&&... args) {
    static T* resultInstance { nullptr };
    if (!resultInstance) {
        std::unique_lock<std::mutex> lock(globalInstancesMutex());
        if (!resultInstance) {
            auto variant = getGlobalInstance(propertyName);
            if (variant.isNull()) {
                std::unique_ptr<T>& instancePtr = globalInstancePointer<T>();
                if (!instancePtr.get()) {
                    // Since we're building the object, store it in a shared_ptr so it's 
                    // destroyed by the destructor of the static instancePtr
                    instancePtr = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
                }
                void* voidInstance = &(*instancePtr);
                variant = QVariant::fromValue(voidInstance);
                setGlobalInstance(propertyName, variant);
            }
            void* returnedVoidInstance = variant.value<void*>();
            resultInstance = static_cast<T*>(returnedVoidInstance);
        }
    }
    return resultInstance;
}


const int BYTES_PER_COLOR = 3;
const int BYTES_PER_FLAGS = 1;
typedef unsigned char colorPart;
typedef unsigned char nodeColor[BYTES_PER_COLOR + BYTES_PER_FLAGS];

// Equivalent to time_t but in usecs instead of secs
quint64 usecTimestampNow(bool wantDebug = false);
void usecTimestampNowForceClockSkew(qint64 clockSkew);

inline bool afterUsecs(quint64& startUsecs, quint64 maxIntervalUecs) {
    auto now = usecTimestampNow();
    auto interval = now - startUsecs;
    if (interval > maxIntervalUecs) {
        startUsecs = now;
        return true;
    }
    return false;
}

inline bool afterSecs(quint64& startUsecs, quint64 maxIntervalSecs) {
    return afterUsecs(startUsecs, maxIntervalSecs * USECS_PER_SECOND);
}

template <typename F>
void doEvery(quint64& lastReportUsecs, quint64 secs, F lamdba) {
    if (afterSecs(lastReportUsecs, secs)) {
        lamdba();
    }
}

// Number of seconds expressed since the first call to this function, expressed as a float
// Maximum accuracy in msecs
float secTimestampNow();

// Custom deleter for QObjects that calls deleteLater
struct LaterDeleter {
    void operator()(QObject* ptr) {
        ptr->deleteLater();
    }
};

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
bool oneAtBit16(unsigned short word, int bitIndex);
void setAtBit16(unsigned short& word, int bitIndex);
void clearAtBit(unsigned char& byte, int bitIndex);
int  getSemiNibbleAt(unsigned short word, int bitIndex);
void setSemiNibbleAt(unsigned short& word, int bitIndex, int value);

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

/// \return true when value is between max and min
inline bool isBetween(int64_t value, int64_t max, int64_t min) { return ((value <= max) && (value >= min)); }

/// \return bool is the float NaN
inline bool isNaN(float value) { return value != value; }

QString formatUsecTime(float usecs);
QString formatUsecTime(double usecs);
QString formatUsecTime(quint64 usecs);
QString formatUsecTime(qint64 usecs);
QString formatSecTime(qint64 secs);

QString formatSecondsElapsed(float seconds);
bool similarStrings(const QString& stringA, const QString& stringB);

template <typename T>
uint qHash(const std::shared_ptr<T>& ptr, uint seed = 0)
{
    return qHash(ptr.get(), seed);
}

void disableQtBearerPoll();

void printSystemInformation();

struct MemoryInfo {
    uint64_t totalMemoryBytes;
    uint64_t availMemoryBytes;
    uint64_t usedMemoryBytes;
    uint64_t processUsedMemoryBytes;
    uint64_t processPeakUsedMemoryBytes;
};

bool getMemoryInfo(MemoryInfo& info);

struct ProcessorInfo {
    int32_t numPhysicalProcessorPackages;
    int32_t numProcessorCores;
    int32_t numLogicalProcessors;
    int32_t numProcessorCachesL1;
    int32_t numProcessorCachesL2;
    int32_t numProcessorCachesL3;
};

bool getProcessorInfo(ProcessorInfo& info);

const QString& getInterfaceSharedMemoryName();

void setMaxCores(uint8_t maxCores);

const QString PARENT_PID_OPTION = "parent-pid";
void watchParentProcess(int parentPID);

bool processIsRunning(int64_t pid);

void setupHifiApplication(QString applicationName);

#ifdef Q_OS_WIN
void* createProcessGroup();
void addProcessToGroup(void* processGroup, qint64 processId);
#endif

#endif // hifi_SharedUtil_h
