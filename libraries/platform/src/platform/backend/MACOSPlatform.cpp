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
#include <QString>
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
    // ennumerate the displays first
    std::vector<GLuint> displayMasks;
    {
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
            const auto MM_TO_IN = 0.0393701f;
            auto displaySizeWidthInches = displaySize.width * MM_TO_IN;
            auto displaySizeHeightInches = displaySize.height * MM_TO_IN;
            
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
            
            auto ppiH = displayModeWidth / displaySizeWidthInches;
            auto ppiV = displayModeHeight / displaySizeHeightInches;
            
            auto ppiHScaled = displayBounds.size.width / displaySizeWidthInches;
            auto ppiVScaled = displayBounds.size.height / displaySizeHeightInches;
            
            auto glmask = CGDisplayIDToOpenGLDisplayMask(displayID);
            // Metal device ID is the recommended new way but requires objective c
            // auto displayDevice = CGDirectDisplayCopyCurrentMetalDevice(displayID);
            
            CGDisplayModeRelease(displayMode);
            
            json display = {};
            
            // Rect region of the desktop in desktop units
            display[keys::display::boundsLeft] = displayBounds.origin.x;
            display[keys::display::boundsRight] = displayBounds.origin.x + displayBounds.size.width;
            display[keys::display::boundsTop] = displayBounds.origin.y;
            display[keys::display::boundsBottom] = displayBounds.origin.y + displayBounds.size.height;
            
            // PPI & resolution
            display[keys::display::physicalWidth] = displaySizeWidthInches;
            display[keys::display::physicalHeight] = displaySizeHeightInches;
            display[keys::display::modeWidth] = displayModeWidth;
            display[keys::display::modeHeight] = displayModeHeight;
            
            //Average the ppiH and V for the simple ppi
            display[keys::display::ppi] = std::round(0.5f * (ppiH + ppiV));
            display[keys::display::ppiDesktop] = std::round(0.5f * (ppiHScaled + ppiVScaled));
            
            // refreshrate
            display[keys::display::modeRefreshrate] = displayRefreshrate;
            
            // Master display ?
            display[keys::display::isMaster] = (displayMaster ? true : false);
            
            // Macos specific
            display["macos_unit"] = displayUnit;
            display["macos_vendor"] = displayVendor;
            display["macos_model"] = displayModel;
            display["macos_serial"] = displaySerial;
            display["macos_glmask"] = glmask;
            displayMasks.push_back(glmask);
            
            _displays.push_back(display);
        }
    }
    
    // Collect Renderer info as exposed by the CGL layers
    GLuint cglDisplayMask = -1; // Iterate over all of them.
    CGLRendererInfoObj rendererInfo;
    GLint rendererInfoCount;
    CGLError error = CGLQueryRendererInfo(cglDisplayMask, &rendererInfo, &rendererInfoCount);
    if (rendererInfoCount <= 0 || 0 != error) {
        return;
    }
    
    // Iterate over all of the renderers and use the figure for the one with the most VRAM,
    // on the assumption that this is the one that will actually be used.
    for (GLint i = 0; i < rendererInfoCount; i++) {
        struct CGLRendererDesc {
            int rendererID{0};
            int deviceVRAM{0};
            int accelerated{0};
            int displayMask{0};
        } desc;
        
        CGLDescribeRenderer(rendererInfo, i, kCGLRPRendererID, &desc.rendererID);
        CGLDescribeRenderer(rendererInfo, i, kCGLRPVideoMemoryMegabytes, &desc.deviceVRAM);
        CGLDescribeRenderer(rendererInfo, i, kCGLRPDisplayMask, &desc.displayMask);
        CGLDescribeRenderer(rendererInfo, i, kCGLRPAccelerated, &desc.accelerated);
        
        // If this renderer is not hw accelerated then just skip it in the enumeration
        if (!desc.accelerated) {
            continue;
        }
        
        // From the rendererID identify the vendorID
        // https://github.com/apitrace/apitrace/blob/master/retrace/glws_cocoa.mm
        GLint vendorID = desc.rendererID & kCGLRendererIDMatchingMask & ~0xfff;
        const GLint VENDOR_ID_SOFTWARE { 0x00020000 };
        const GLint VENDOR_ID_AMD { 0x00021000 };
        const GLint VENDOR_ID_NVIDIA { 0x00022000 };
        const GLint VENDOR_ID_INTEL { 0x00024000 };
        
        const char* vendorName;
        switch (vendorID) {
            case VENDOR_ID_SOFTWARE:
                // Software renderer then skip it (should already have been caught by hwaccelerated test abve
                continue;
                break;
            case VENDOR_ID_AMD:
                vendorName = keys::gpu::vendor_AMD;
                break;
            case VENDOR_ID_NVIDIA:
                vendorName = keys::gpu::vendor_NVIDIA;
                break;
            case VENDOR_ID_INTEL:
                vendorName = keys::gpu::vendor_Intel;
                break;
            default:
                vendorName = keys::UNKNOWN;
                break;
        }
        
        //once we reach this point, the renderer is legit
        // Start defining the gpu json
        json gpu = {};
        gpu[keys::gpu::vendor] = vendorName;
        gpu[keys::gpu::videoMemory] = desc.deviceVRAM;
        gpu["macos_rendererID"] = desc.rendererID;
        gpu["macos_displayMask"] = desc.displayMask;
        
        std::vector<int> displayIndices;
        for (int d = 0; d < (int) displayMasks.size(); d++) {
            if (desc.displayMask & displayMasks[d]) {
                displayIndices.push_back(d);
                _displays[d][keys::display::gpu] = _gpus.size();
            }
        }
        gpu[keys::gpu::displays] = displayIndices;
        
        _gpus.push_back(gpu);
    }
    
    CGLDestroyRendererInfo(rendererInfo);
    
    
    {   //get gpu information from the system profiler that we don't know how to retreive otherwise
        struct ChipsetModelDesc {
            std::string model;
            std::string vendor;
            int deviceID{0};
            int videoMemory{0};
        };
        std::vector<ChipsetModelDesc> chipsetDescs;
        FILE* stream = popen("system_profiler SPDisplaysDataType | grep -e Chipset -e VRAM -e Vendor -e \"Device ID\"", "r");
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
            ChipsetModelDesc desc;

            desc.model = found.str(2);
            
            // Find the end of the gpu block
            gpuArrayDesc = found.suffix();
            std::string gpuDesc = gpuArrayDesc;
            const std::regex endGpuToken("Chipset Model: ");
            if (std::regex_search(gpuArrayDesc, found, endGpuToken)) {
                gpuDesc = found.prefix();
            }
            
            // Find the vendor
            desc.vendor = findGPUVendorInDescription(desc.model);
            
            // Find the memory amount in MB
            const std::regex memoryToken("(VRAM .*: )(.*)");
            if (std::regex_search(gpuDesc, found, memoryToken)) {
                auto memAmountUnit = found.str(2);
                std::smatch amount;
                const std::regex memAmountGBToken("(\\d*) GB");
                const std::regex memAmountMBToken("(\\d*) MB");
                const int GB_TO_MB { 1024 };
                if (std::regex_search(memAmountUnit, amount, memAmountGBToken)) {
                    desc.videoMemory = std::stoi(amount.str(1)) * GB_TO_MB;
                } else if (std::regex_search(memAmountUnit, amount, memAmountMBToken)) {
                    desc.videoMemory = std::stoi(amount.str(1));
                } else {
                    desc.videoMemory = std::stoi(memAmountUnit);
                }
            }
            
            // Find the Device ID
            const std::regex deviceIDToken("(Device ID: )(.*)");
            if (std::regex_search(gpuDesc, found, deviceIDToken)) {
                desc.deviceID = std::stoi(found.str(2), 0, 16);
            }
            
            chipsetDescs.push_back(desc);
        }
        
        // GO through the detected gpus in order and complete missing information from ChipsetModelDescs
        // assuming the order is the same and checking that the vendor and memory amount match as a simple check
        auto numDescs = (int) std::min(chipsetDescs.size(),_gpus.size());
        for (int i = 0; i < numDescs; ++i) {
            const auto& chipset = chipsetDescs[i];
            auto& gpu = _gpus[i];
            
            if (   (chipset.vendor.find( gpu[keys::gpu::vendor]) != std::string::npos)
                && (chipset.videoMemory == gpu[keys::gpu::videoMemory]) ) {
                gpu[keys::gpu::model] = chipset.model;
                gpu["macos_deviceID"] = chipset.deviceID;
            }
        }
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

void MACOSInstance::enumerateGraphicsApis() {
    Instance::enumerateGraphicsApis();

    // TODO imeplement Metal query
}
