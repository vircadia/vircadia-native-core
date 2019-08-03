//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AndroidPlatform.h"
#include "../PlatformKeys.h"
#include <GPUIdent.h>
#include <QSysInfo>
#include <QtCore/QtGlobal>

using namespace platform;

void AndroidInstance::enumerateCpus() {
     json cpu;
     cpu[keys::cpu::vendor] = "";
     cpu[keys::cpu::model] = "";
     cpu[keys::cpu::clockSpeed] = "";
     cpu[keys::cpu::numCores] = 0;

    _cpus.push_back(cpu);
}

void AndroidInstance::enumerateGpusAndDisplays() {
    GPUIdent* ident = GPUIdent::getInstance();
    json gpu = {};
    gpu[keys::gpu::model] = ident->getName().toUtf8().constData();
    gpu[keys::gpu::vendor] = findGPUVendorInDescription(gpu[keys::gpu::model].get<std::string>());
    gpu[keys::gpu::videoMemory] = ident->getMemory();
    gpu[keys::gpu::driver] = ident->getDriver().toUtf8().constData();

    _gpus.push_back(gpu);
    _displays = ident->getOutput();
}

void AndroidInstance::enumerateMemory() {
    json ram = {};
    ram[keys::memory::memTotal]=0;
    _memory = ram;
}

void AndroidInstance::enumerateComputer(){
    _computer[keys::computer::OS] = keys::computer::OS_ANDROID;
    _computer[keys::computer::vendor] = "";
    _computer[keys::computer::model] = "";

    auto sysInfo = QSysInfo();

    _computer[keys::computer::OSVersion] = sysInfo.kernelVersion().toStdString();
}

