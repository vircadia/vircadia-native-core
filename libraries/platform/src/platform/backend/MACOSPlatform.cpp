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

#include <QtCore/QtGlobal>

#ifdef Q_OS_MAC
#include <unistd.h>
#include <cpuid.h>
#include <sys/sysctl.h>

#include <sstream>
#include <regex>

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <QSysInfo>

#include <OpenGL/OpenGL.h>
#endif

using namespace platform;

void MACOSInstance::enumerateCpus() {
    json cpu = {};

    cpu[keys::cpu::vendor] = CPUIdent::Vendor();
    cpu[keys::cpu::model] = CPUIdent::Brand();
    cpu[keys::cpu::numCores] = std::thread::hardware_concurrency();

    _cpus.push_back(cpu);
}


void MACOSInstance::enumerateGpusAndDisplays() {
#ifdef Q_OS_MAC
    // Collect Renderer info as exposed by the CGL layers
    GLuint cglDisplayMask = -1; // Iterate over all of them.
    CGLRendererInfoObj rendererInfo;
    GLint rendererInfoCount;
    CGLError error = CGLQueryRendererInfo(cglDisplayMask, &rendererInfo, &rendererInfoCount);
    if (rendererInfoCount <= 0 || 0 != error) {
        return;
    }
    
    struct CGLRendererDesc {
        int rendererID{0};
        int deviceVRAM{0};
        int majorGLVersion{0};
        int registryIDLow{0};
        int registryIDHigh{0};
        int displayMask{0};
    };
    std::vector<CGLRendererDesc> renderers(rendererInfoCount);
    
    // Iterate over all of the renderers and use the figure for the one with the most VRAM,
    // on the assumption that this is the one that will actually be used.
    for (GLint i = 0; i < rendererInfoCount; i++) {
        auto& desc = renderers[i];
        
        CGLDescribeRenderer(rendererInfo, i, kCGLRPRendererID, &desc.rendererID);
        CGLDescribeRenderer(rendererInfo, i, kCGLRPVideoMemoryMegabytes, &desc.deviceVRAM);
        CGLDescribeRenderer(rendererInfo, i, kCGLRPMajorGLVersion, &desc.majorGLVersion);
        CGLDescribeRenderer(rendererInfo, i, kCGLRPRegistryIDLow, &desc.registryIDLow);
        CGLDescribeRenderer(rendererInfo, i, kCGLRPRegistryIDHigh, &desc.registryIDHigh);
        CGLDescribeRenderer(rendererInfo, i, kCGLRPDisplayMask, &desc.displayMask);
        
        kCGLRPDisplayMask
        json gpu = {};
        gpu["A"] =desc.rendererID;
        gpu["B"] =desc.deviceVRAM;
        gpu["C"] =desc.majorGLVersion;
        
        gpu["D"] =desc.registryIDLow;
        gpu["E"] =desc.registryIDHigh;
        gpu["F"] =desc.displayMask;
        
        _gpus.push_back(gpu);
    }
    
    CGLDestroyRendererInfo(rendererInfo);
    
    //get gpu / display information from the system profiler
    
    FILE* stream = popen("system_profiler SPDisplaysDataType | grep -e Chipset -e VRAM -e Vendor -e \"Device ID\" -e Displays -e \"Display Type\" -e Resolution -e \"Main Display\"", "r");
    std::ostringstream hostStream;
    while (!feof(stream) && !ferror(stream)) {
        char buf[128];
        int bytesRead = fread(buf, 1, 128, stream);
        hostStream.write(buf, bytesRead);
    }
    std::string gpuArrayDesc = hostStream.str();
    
    // Find the Chipset model first
    const std::regex chipsetModelToken("(Chipset Model: )(.*)");
    std::smatch found;
    
    while (std::regex_search(gpuArrayDesc, found, chipsetModelToken)) {
        json gpu = {};
        gpu[keys::gpu::model] = found.str(2);
        
        // Find the end of the gpu block
        gpuArrayDesc = found.suffix();
        std::string gpuDesc = gpuArrayDesc;
        const std::regex endGpuToken("Chipset Model: ");
        if (std::regex_search(gpuArrayDesc, found, endGpuToken)) {
            gpuDesc = found.prefix();
        }
        
        // Find the vendor
        gpu[keys::gpu::vendor] = findGPUVendorInDescription(gpu[keys::gpu::model].get<std::string>());
        
        // Find the memory amount in MB
        const std::regex memoryToken("(VRAM .*: )(.*)");
        if (std::regex_search(gpuDesc, found, memoryToken)) {
            auto memAmountUnit = found.str(2);
            std::smatch amount;
            const std::regex memAmountGBToken("(\\d*) GB");
            const std::regex memAmountMBToken("(\\d*) MB");
            const int GB_TO_MB { 1024 };
            if (std::regex_search(memAmountUnit, amount, memAmountGBToken)) {
                gpu[keys::gpu::videoMemory] = std::stoi(amount.str(1)) * GB_TO_MB;
            } else if (std::regex_search(memAmountUnit, amount, memAmountMBToken)) {
                gpu[keys::gpu::videoMemory] = std::stoi(amount.str(1));
            } else {
                gpu[keys::gpu::videoMemory] = found.str(2);
            }
        }
        
        // Find the Device ID
        const std::regex deviceIDToken("(Device ID: )(.*)");
        if (std::regex_search(gpuDesc, found, deviceIDToken)) {
            gpu["deviceID"] = std::stoi(found.str(2));
        }
        
        // Enumerate the Displays
        const std::regex displaysToken("(Displays: )");
        if (std::regex_search(gpuDesc, found, displaysToken)) {
            std::string displayArrayDesc = found.suffix();
            std::vector<json> displays;
            
            int displayIndex = 0;
            const std::regex displayTypeToken("(Display Type: )(.*)");
            while (std::regex_search(displayArrayDesc, found, displayTypeToken)) {
                json display = {};
                display["display"] = found.str(2);
                
                // Find the end of the display block
                displayArrayDesc = found.suffix();
                std::string displayDesc = displayArrayDesc;
                const std::regex endDisplayToken("Display Type: ");
                if (std::regex_search(displayArrayDesc, found, endDisplayToken)) {
                    displayDesc = found.prefix();
                }
                
                // Find the resolution
                const std::regex resolutionToken("(Resolution: )(.*)");
                if (std::regex_search(displayDesc, found, deviceIDToken)) {
                    display["resolution"] = found.str(2);
                }
                
                // Find is main display
                const std::regex mainMonitorToken("(Main Display: )(.*)");
                if (std::regex_search(displayDesc, found, mainMonitorToken)) {
                    display["isMaster"] = found.str(2);
                } else {
                    display["isMaster"] = "false";
                }
                
                display["index"] = displayIndex;
                displayIndex++;
                
                displays.push_back(display);
            }
            if (!displays.empty()) {
                gpu["displays"] = displays;
            }
        }
            
        _gpus.push_back(gpu);
    }
#endif

#ifdef Q_OS_MAC
    uint32_t numDisplays = 0;
    CGError error = CGGetOnlineDisplayList(0, nullptr, &numDisplays);
    if (numDisplays <= 0 || error != kCGErrorSuccess) {
        return;
    }
    
    std::vector<CGDirectDisplayID> onlineDisplayIDs(numDisplays, 0);
    error = CGGetOnlineDisplayList(onlineDisplayIDs.size(), onlineDisplayIDs.data(), &numDisplays);
    if (error != kCGErrorSuccess) {
        return;
    }
    
    for (auto displayID : onlineDisplayIDs) {
        auto displaySize = CGDisplayScreenSize(displayID);

        auto glmask = CGDisplayIDToOpenGLDisplayMask(displayID);
        
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
        
        display[keys::display::boundsLeft] = displayBounds.origin.x;
        display[keys::display::boundsRight] = displayBounds.origin.x + displayBounds.size.width;
        display[keys::display::boundsTop] = displayBounds.origin.y;
        display[keys::display::boundsBottom] = displayBounds.origin.y + displayBounds.size.height;
        
        display["isMaster"] = displayMaster;

        display["unit"] = displayUnit;
        display["vendor"] = displayVendor;
        display["model"] = displayModel;
        display["serial"] = displaySerial;
        
        display["refreshrate"] =displayRefreshrate;
        display["modeWidth"] = displayModeWidth;
        display["modeHeight"] = displayModeHeight;
        
        display["glmask"] = glmask;

        _displays.push_back(display);
    }
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

    auto sysInfo = QSysInfo();

    _computer[keys::computer::OSVersion] = sysInfo.kernelVersion().toStdString();
#endif

}

