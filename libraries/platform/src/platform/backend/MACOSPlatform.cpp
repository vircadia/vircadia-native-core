//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MACOSPlatform.h"
#include "../PlatformKeys.h"

#include <thread>
#include <string>
#include <CPUIdent.h>
#include <GPUIdent.h>

#ifdef Q_OS_MAC
#include <unistd.h>
#include <cpuid.h>
#include <sys/sysctl.h>

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <QSysInfo>
#endif

using namespace platform;

void MACOSInstance::enumerateCpus() {
    json cpu = {};

    cpu[keys::cpu::vendor] = CPUIdent::Vendor();
    cpu[keys::cpu::model] = CPUIdent::Brand();
    cpu[keys::cpu::numCores] = std::thread::hardware_concurrency();

    _cpus.push_back(cpu);
}

void MACOSInstance::enumerateGpus() {
#ifdef Q_OS_MAC

    GPUIdent* ident = GPUIdent::getInstance();
    json gpu = {};

    gpu[keys::gpu::model] = ident->getName().toUtf8().constData();
    gpu[keys::gpu::vendor] = findGPUVendorInDescription(gpu[keys::gpu::model].get<std::string>());
    gpu[keys::gpu::videoMemory] = ident->getMemory();
    gpu[keys::gpu::driver] = ident->getDriver().toUtf8().constData();

    _gpus.push_back(gpu);
    
#endif

}

void MACOSInstance::enumerateDisplays() {
#ifdef Q_OS_MAC
    auto displayID = CGMainDisplayID();
    auto displaySize = CGDisplayScreenSize(displayID);

    const auto MM_TO_IN = 0.0393701;
    auto displaySizeWidthInches = displaySize.width * MM_TO_IN;
    auto displaySizeHeightInches = displaySize.height * MM_TO_IN;
    auto displaySizeDiagonalInches = sqrt(displaySizeWidthInches * displaySizeWidthInches + displaySizeHeightInches * displaySizeHeightInches);
    
    auto displayBounds = CGDisplayBounds(displayID);
    auto displayMaster =CGDisplayIsMain(displayID);
    
    auto displayUnit =CGDisplayUnitNumber(displayID);
    auto displayModel =CGDisplayModelNumber(displayID);
    auto displayVendor = CGDisplayVendorNumber(displayID);
    auto displaySerial = CGDisplaySerialNumber(displayID);

    auto displayMode = CGDisplayCopyDisplayMode(displayID);
    auto displayModeWidth = CGDisplayModeGetPixelWidth(displayMode);
    auto displayModeHeight = CGDisplayModeGetPixelHeight(displayMode);
    auto displayRefreshrate = CGDisplayModeGetRefreshRate(displayMode);

    CGDisplayModeRelease(displayMode);
    
    json display = {};
    
    display["physicalWidth"] = displaySizeWidthInches;
    display["physicalHeight"] = displaySizeHeightInches;
    display["physicalDiagonal"] = displaySizeDiagonalInches;
    
    display["ppi"] = sqrt(displayModeHeight * displayModeHeight + displayModeWidth * displayModeWidth) / displaySizeDiagonalInches;
    
    display["coordLeft"] = displayBounds.origin.x;
    display["coordRight"] = displayBounds.origin.x + displayBounds.size.width;
    display["coordTop"] = displayBounds.origin.y;
    display["coordBottom"] = displayBounds.origin.y + displayBounds.size.height;
    
    display["isMaster"] = displayMaster;

    display["unit"] = displayUnit;
    display["vendor"] = displayVendor;
    display["model"] = displayModel;
    display["serial"] = displaySerial;
    
    display["refreshrate"] =displayRefreshrate;
    display["modeWidth"] = displayModeWidth;
    display["modeHeight"] = displayModeHeight;

    _displays.push_back(display);
#endif
}

void MACOSInstance::enumerateMemory() {
    json ram = {};

#ifdef Q_OS_MAC
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    ram[keys::memory::memTotal] = pages * page_size;
#endif
    _memory = ram;
}

void MACOSInstance::enumerateComputer(){
    _computer[keys::computer::OS] = keys::computer::OS_MACOS;
    _computer[keys::computer::vendor] = keys::computer::vendor_Apple;

#ifdef Q_OS_MAC

    //get system name
    size_t len=0;
    sysctlbyname("hw.model",NULL, &len, NULL, 0);
    char* model = (char *) malloc(sizeof(char)*len+1);
    sysctlbyname("hw.model", model, &len, NULL,0);
    
    _computer[keys::computer::model]=std::string(model);

    free(model);
    
#endif

    auto sysInfo = QSysInfo();

    _computer[keys::computer::OSVersion] = sysInfo.kernelVersion().toStdString();
}

