//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WINPlatform.h"

#ifdef Q_OS_WINDOWS
#include <intrin.h>
#include <Windows.h>
#endif

#include <thread>
#include <GPUIdent.h>
#include <string>


using namespace platform;

void WINInstance::enumerateCpu() {
    json cpu = {};

#ifdef Q_OS_WINDOWS
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

   cpu["brand"] = CPUBrandString;
   cpu["model"] = CPUModelString;
   cpu["clockSpeed"] = CPUClockString;
   cpu["numCores"] = std::thread::hardware_concurrency();
#endif
   
    _cpu.push_back(cpu);
}

void WINInstance::enumerateGpu() {

    GPUIdent* ident = GPUIdent::getInstance();
   
    json gpu = {};
    gpu["name"] = ident->getName().toUtf8().constData();
    gpu["memory"] = ident->getMemory();
    gpu["driver"] = ident->getDriver().toUtf8().constData();

    _gpu.push_back(gpu);
    _display = ident->getOutput();
}

void WINInstance::enumerateMemory() {
    json ram = {};
    
#ifdef Q_OS_WINDOWS
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    int totalRam = statex.ullTotalPhys / 1024 / 1024;
    ram["totalMem"] = totalRam;
#endif
    _memory.push_back(ram);
}
