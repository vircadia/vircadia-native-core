//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LinuxPlatform.h"
#include "../PlatformKeys.h"

#include <thread>
#include <string>
#include <CPUIdent.h>
#include <GPUIdent.h>

using namespace platform;

void LinuxInstance::enumerateCpu() {
    json cpu = {};

    cpu[keys::cpu::vendor] = CPUIdent::Vendor();
    cpu[keys::cpu::model] = CPUIdent::Brand();
    cpu[keys::cpu::numCores] = std::thread::hardware_concurrency();

    _cpu.push_back(cpu);
}
/*
void LinuxInstance::enumerateCpu() {
    json cpu = {};

    uint32_t cpuInfo[4]={0,0,0,0};
    char CPUBrandString[16];
    char CPUModelString[16];
    char CPUClockString[16];
    uint32_t nExIds;
    getLCpuId(cpuInfo, 0x80000000);
    nExIds = cpuInfo[0];
    
    for (uint32_t i = 0x80000000; i <= nExIds; ++i) {
        getLCpuId(cpuInfo, i);
        // Interpret CPU brand string
        if (i == 0x80000002) {
            memcpy(CPUBrandString, cpuInfo, sizeof(cpuInfo));
        } else if (i == 0x80000003) {
            memcpy(CPUModelString, cpuInfo, sizeof(cpuInfo));
        } else if (i == 0x80000004) {
            memcpy(CPUClockString, cpuInfo, sizeof(cpuInfo));
        }
    }

    cpu[keys::cpu::vendor] = CPUBrandString;
    cpu[keys::cpu::model] = CPUModelString;
    cpu[keys::cpu::clockSpeed] = CPUClockString;
    cpu[keys::cpu::numCores] = std::thread::hardware_concurrency();

    _cpu.push_back(cpu);
}
*/

void LinuxInstance::enumerateGpu() {
    GPUIdent* ident = GPUIdent::getInstance();
    json gpu = {};
    gpu[keys::gpu::vendor] = ident->getName().toUtf8().constData();
    gpu[keys::gpu::model] = ident->getName().toUtf8().constData();
    gpu[keys::gpu::videoMemory] = ident->getMemory();
    gpu[keys::gpu::driver] = ident->getDriver().toUtf8().constData();

    _gpu.push_back(gpu);
    _display = ident->getOutput();
}

void LinuxInstance::enumerateMemory() {
    json ram = {};
    ram[keys::memTotal]=0;

    _memory.push_back(ram);
}

void LinuxInstance::enumerateComputer(){
    
    _computer[keys::computer::OS] = keys::computer::OS_LINUX;
    _computer[keys::computer::vendor] = "";
    _computer[keys::computer::model] = "";
    //no implememntation at this time
}

