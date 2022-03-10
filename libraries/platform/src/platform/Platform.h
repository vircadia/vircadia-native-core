//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Platform_h
#define hifi_Platform_h


#include <QLoggingCategory>
#include <nlohmann/json.hpp>

Q_DECLARE_LOGGING_CATEGORY(platform_log)

namespace platform {
    using json = nlohmann::json;

void create();
void destroy();
bool enumeratePlatform();

int getNumCPUs();
json getCPU(int index);
int getMasterCPU();

int getNumGPUs();
json getGPU(int index);
int getMasterGPU();

int getNumDisplays();
json getDisplay(int index);
int getMasterDisplay();

json getMemory();

json getComputer();

json getAll();

json getDescription();

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

}  // namespace platform

#endif  // hifi_platform_h
