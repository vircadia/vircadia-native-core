//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WINPlatform.h"
#include <intrin.h>
#include <Windows.h>
#include <thread>



#ifdef Q_OS_WINDOWS
#include <dxgi.h>
#include <d3d11.h>
#endif

using namespace platform;
using namespace nlohmann;

bool WINInstance::enumerateProcessors() {
    enumerateCpu();
    enumerateGpu();
    enumerateRam();

    return true;
}

void WINInstance::enumerateCpu() {
    json cpu;
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
    cpu["numCores"] = getNumLogicalCores();

    _processors.push_back(cpu);
}

unsigned int WINInstance::getNumLogicalCores() {
    return std::thread::hardware_concurrency();
}

void WINInstance::enumerateGpu() {
#ifdef Q_OS_WINDOWS
    IDXGIAdapter* adapter;
    std::vector<IDXGIAdapter*> adapters;
    IDXGIFactory* factory = NULL;

    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    _gpu.clear();
    if (SUCCEEDED(hr)) {
        for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC* desc;

            if (SUCCEEDED(adapter->GetDesc(desc))) {
                json* gpu = new json();
            
                (*gpu)["BrandModel"] = desc->Description;
                (*gpu)["DedicatedRam"] = desc->DedicatedVideoMemory/1024/1024;
                (*gpu)["SharedRam"] = desc->SharedSystemMemory / 1024 / 1024;

                UINT numModes = 0;
                DXGI_MODE_DESC* displayModes = NULL;
                DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                IDXGIOutput* output = NULL;

                if (SUCCEEDED(adapter->EnumOutputs(0, &output))) {
                   output->GetDisplayModeList(format, 0, &numModes, displayModes);

                   DXGI_OUTPUT_DESC* desc;

                   output->GetDesc(desc);

                   //auto a = desc->Monitor;
                   //auto b = desc->DeviceName;
                   //figure out monitor info here

                }
                
                _gpu.push_back(gpu);

            }
        }
    }

    if (adapter) {
        adapter->Release();
    }
    if (factory) {
        factory->Release();
    }


#endif
}

void WINInstance::enumerateRam() {
    json ram;
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    int totalRam = statex.ullTotalPhys / 1024 / 1024;
    ram["totalMem"] = totalRam;
    _memory.push_back(ram);
}
