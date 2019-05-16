//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AndroidPlatform.h"
#include "platformJsonKeys.h"
#include <GPUIdent.h>

using namespace platform;

void AndroidInstance::enumerateCpu() {
     json cpu;
     cpu["cpuBrand"] = "";
     cpu["cpuModel"] = "";
     cpu["cpuClockSpeed"] = "";
     cpu["cpuNumCores"] = "";
    _cpu.push_back(cpu);
}

void AndroidInstance::enumerateGpu() {
    GPUIdent* ident = GPUIdent::getInstance();
    json gpu = {};
    gpu["gpuName"] = ident->getName().toUtf8().constData();
    gpu["gpuMemory"] = ident->getMemory();
    gpu["gpuDriver"] = ident->getDriver().toUtf8().constData();
    
    _gpu.push_back(gpu);
    _display = ident->getOutput();
}

void AndroidInstance::enumerateMemory() {
    json ram = {};
    ram["totalMemory"]="";
    _memory.push_back(ram);
}

void AndroidInstance::enumerateComputer(){
    //no implememntation at this time
}

