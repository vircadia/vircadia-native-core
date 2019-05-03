//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WINPlatform.h"
#include <intrin.h>
#include <Windows.h>
#include <thread>

using namespace platform;
using namespace nlohmann;

bool WINInstance::enumerateProcessors() {
    
    cpu cpu;
    
    getCpuDetails(cpu);
    
    cpu.numberOfCores = getNumLogicalCores();

    _processors.push_back(cpu);

    _memory.totalMb = getTotalSystemRamMb();

    return true;
}

void WINInstance::getCpuDetails(cpu &cpu) {
    int CPUInfo[4] = { -1 };
    unsigned nExIds;
    unsigned int i = 0;
    char CPUBrandString[16];
    char CPUModelString[16];
    char CPUClockString[16];
   
    // Get the information associated with each extended ID.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];

    for (i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(CPUInfo, i);
        // Interpret CPU brand string
        if (i == 0x80000002) {
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000003) {
            memcpy(CPUModelString, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000004) {
            memcpy(CPUClockString, CPUInfo, sizeof(CPUInfo));
        }
    }
    
    cpu.brand = CPUBrandString;
    cpu.model = CPUModelString;
    cpu.clockSpeed = CPUClockString;
}

unsigned int WINInstance::getNumLogicalCores() {
    return std::thread::hardware_concurrency();
}

int WINInstance::getTotalSystemRamMb() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullTotalPhys / 1024 / 1024;
}

std::string WINInstance::getProcessor(int index) {
   
    std::string result;
    if (index >= _processors.size())
        return result;
    
    json j;
    to_Json(j, _processors.at(index));

    //serialize this
    return j.dump();
}

void WINInstance::to_Json(json& result, const cpu& cpu) {
    
    result["cpuBrand"] = cpu.brand;
    result["cpuModel"] = cpu.model;
    result["cpuClockSpeed"] = cpu.clockSpeed;
    result["cpuNumberOfCores"] = cpu.numberOfCores;
}


