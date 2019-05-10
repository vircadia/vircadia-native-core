//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MACOSPlatform.h"

#include <thread>
#include <GPUIdent.h>
#include <string>
#include <unistd.h>

using namespace platform;

bool MACOSInstance::enumeratePlatform() {
    enumerateCpu();
    enumerateGpu();
    enumerateRam();
    return true;
}

void MACOSInstance::enumerateCpu() {
    json *cpu= new json();
   
  //  (*cpu)["brand"] =  ident->getName();
   // (*cpu)["model"] = CPUModelString;
   // (*cpu)["clockSpeed"] = CPUClockString;
   // (*cpu)["numCores"] = getNumLogicalCores();

    _cpu.push_back(cpu);
}

unsigned int MACOSInstance::getNumLogicalCores() {
    return std::thread::hardware_concurrency();
}

void MACOSInstance::enumerateGpu() {

	GPUIdent* ident = GPUIdent::getInstance();
   
    json *gpu = new json();
    (*gpu)["name"] = ident->getName().toUtf8().constData();
    (*gpu)["memory"] = ident->getMemory();
    (*gpu)["driver"] = ident->getDriver().toUtf8().constData();

    _gpu.push_back(gpu);
    _display = ident->getOutput();
}

void MACOSInstance::enumerateRam() {
    json* ram = new json();
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
   
    (*ram)["totalMem"] =  pages * page_size;;

    _memory.push_back(ram);
}
