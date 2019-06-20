//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WINPlatform.h"
#include "../PlatformKeys.h"

#include <thread>
#include <string>
#include <CPUIdent.h>
#include <GPUIdent.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <QSysInfo>
#endif

using namespace platform;

void WINInstance::enumerateCpus() {
    json cpu = {};
    
    cpu[keys::cpu::vendor] = CPUIdent::Vendor();
    cpu[keys::cpu::model] = CPUIdent::Brand();
    cpu[keys::cpu::numCores] = std::thread::hardware_concurrency();

    _cpus.push_back(cpu);
}

void WINInstance::enumerateGpus() {

    GPUIdent* ident = GPUIdent::getInstance();
   
    json gpu = {};
    gpu[keys::gpu::model] = ident->getName().toUtf8().constData();
    gpu[keys::gpu::vendor] = findGPUVendorInDescription(gpu[keys::gpu::model].get<std::string>());
    gpu[keys::gpu::videoMemory] = ident->getMemory();
    gpu[keys::gpu::driver] = ident->getDriver().toUtf8().constData();

    _gpus.push_back(gpu);
    _displays = ident->getOutput();
}

void WINInstance::enumerateMemory() {
    json ram = {};
    
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    int totalRam = statex.ullTotalPhys / 1024 / 1024;
    ram[platform::keys::memory::memTotal] = totalRam;
#endif
    _memory = ram;
}

void WINInstance::enumerateComputer(){
    _computer[keys::computer::OS] = keys::computer::OS_WINDOWS;
    _computer[keys::computer::vendor] = "";
    _computer[keys::computer::model] = "";
    
    auto sysInfo = QSysInfo();

    _computer[keys::computer::OSVersion] = sysInfo.kernelVersion().toStdString();
}

