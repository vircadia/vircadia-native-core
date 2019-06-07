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
#endif

using namespace platform;

void MACOSInstance::enumerateCpu() {
    json cpu = {};

    cpu[keys::cpu::vendor] = CPUIdent::Vendor();
    cpu[keys::cpu::model] = CPUIdent::Brand();
    cpu[keys::cpu::numCores] = std::thread::hardware_concurrency();

    _cpu.push_back(cpu);
}

void MACOSInstance::enumerateGpu() {
#ifdef Q_OS_MAC
/*    uint32_t cglDisplayMask = -1; // Iterate over all of them.
    CGLRendererInfoObj rendererInfo;
    GLint rendererInfoCount;
    CGLError err = CGLQueryRendererInfo(cglDisplayMask, &rendererInfo, &rendererInfoCount);
    GLint j, numRenderers = 0, deviceVRAM, bestVRAM = 0;
    err = CGLQueryRendererInfo(cglDisplayMask, &rendererInfo, &numRenderers);
    if (0 == err) {
        // Iterate over all of them and use the figure for the one with the most VRAM,
        // on the assumption that this is the one that will actually be used.
        CGLDescribeRenderer(rendererInfo, 0, kCGLRPRendererCount, &numRenderers);
        for (j = 0; j < numRenderers; j++) {
            CGLDescribeRenderer(rendererInfo, j, kCGLRPVideoMemoryMegabytes, &deviceVRAM);
            if (deviceVRAM > bestVRAM) {
                bestVRAM = deviceVRAM;
                isValid = true;
            }
        }
    }
    
    //get gpu name
    FILE* stream = popen("system_profiler SPDisplaysDataType | grep Chipset", "r");
    
    std::ostringstream hostStream;
    while (!feof(stream) && !ferror(stream)) {
        char buf[128];
        int bytesRead = fread(buf, 1, 128, stream);
        hostStream.write(buf, bytesRead);
    }
    
    QString result = QString::fromStdString(hostStream.str());
    QStringList parts = result.split('\n');
    std::string name;
    
    for (int i = 0; i < parts.size(); ++i) {
        if (parts[i].toLower().contains("radeon") || parts[i].toLower().contains("nvidia")) {
            _name=parts[i];
        }
    }
    
    _dedicatedMemoryMB = bestVRAM;
    CGLDestroyRendererInfo(rendererInfo);
*/
    
   // auto displayID = CGMainDisplayID();
   // auto displayGLMask = CGDisplayIDToOpenGLDisplayMask(displayID);
  //  auto metalDevice = CGDirectDisplayCopyCurrentMetalDevice(displayID);
    
    GPUIdent* ident = GPUIdent::getInstance();
    json gpu = {};

    gpu[keys::gpu::vendor] = ident->getName().toUtf8().constData();
    gpu[keys::gpu::model] = ident->getName().toUtf8().constData();
    gpu[keys::gpu::videoMemory] = ident->getMemory();
    gpu[keys::gpu::driver] = ident->getDriver().toUtf8().constData();

    _gpu.push_back(gpu);
    
#endif

}

void MACOSInstance::enumerateDisplays() {
#ifdef Q_OS_MAC
    auto displayID = CGMainDisplayID();
    auto displaySize = CGDisplayScreenSize(displayID);
    auto displaySizeWidthInches = displaySize.width * 0.0393701;
    auto displaySizeHeightInches = displaySize.height * 0.0393701;
    auto displaySizeDiagonalInches = sqrt(displaySizeWidthInches * displaySizeWidthInches + displaySizeHeightInches * displaySizeHeightInches);
    
    auto displayPixelsWidth= CGDisplayPixelsWide(displayID);
    auto displayPixelsHeight= CGDisplayPixelsHigh(displayID);
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
    
   // display["physicalWidth"] = displaySizeWidthInches;
   // display["physicalHeight"] = displaySizeHeightInches;
    display["physicalDiagonal"] = displaySizeDiagonalInches;
    
  //  display["ppiH"] = displayModeWidth / displaySizeWidthInches;
  //  display["ppiV"] = displayModeHeight / displaySizeHeightInches;
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
    
    
    _display.push_back(display);
#endif
}

void MACOSInstance::enumerateMemory() {
    json ram = {};

#ifdef Q_OS_MAC
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    ram[keys::memTotal] = pages * page_size;
#endif
    _memory.push_back(ram);
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
}

