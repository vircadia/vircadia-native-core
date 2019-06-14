//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "PlatformInstance.h"

#include "../PlatformKeys.h"
#include "../Profiler.h"

using namespace platform;

bool Instance::enumeratePlatform() {
    enumerateComputer();
    enumerateCpu();
    enumerateGpu();
    enumerateDisplays();
    enumerateMemory();

    // And profile the platform and put the tier in "computer"
    _computer[keys::computer::profileTier] = Profiler::TierNames[Profiler::profilePlatform()];

    return true;
}

json Instance::getCPU(int index) {
    assert(index <(int) _cpu.size());
    if (index >= (int)_cpu.size())
        return json();

    return _cpu.at(index);
}

//These are ripe for template.. will work on that next
json Instance::getMemory(int index) {
    assert(index <(int) _memory.size());
    if(index >= (int)_memory.size())
        return json();

    return _memory.at(index);
}

json Instance::getGPU(int index) {
    assert(index <(int) _gpu.size());

    if (index >=(int) _gpu.size())
        return json();
    
    return _gpu.at(index);
}

json Instance::getDisplay(int index) {
    assert(index <(int) _display.size());
    
    if (index >=(int) _display.size())
        return json();

    return _display.at(index);
}

Instance::~Instance() {
    if (_cpu.size() > 0) {
        _cpu.clear();
    }

    if (_memory.size() > 0) {
        _memory.clear();
    }

    if (_gpu.size() > 0) {
        _gpu.clear();
    }

    if (_display.size() > 0) {
        _display.clear();
    }
}


json Instance::listAllKeys() {
    json allKeys;
    allKeys.array({{
        keys::cpu::vendor,
        keys::cpu::vendor_Intel,
        keys::cpu::vendor_AMD,
        keys::cpu::model,
        keys::cpu::clockSpeed,
        keys::cpu::numCores,

        keys::gpu::vendor,
        keys::gpu::vendor_NVIDIA,
        keys::gpu::vendor_AMD,
        keys::gpu::vendor_Intel,
        keys::gpu::model,
        keys::gpu::videoMemory,
        keys::gpu::driver,

        keys::display::description,
        keys::display::name,
        keys::display::coordsLeft,
        keys::display::coordsRight,
        keys::display::coordsTop,
        keys::display::coordsBottom,

        keys::memTotal,

        keys::computer::OS,
        keys::computer::OS_WINDOWS,
        keys::computer::OS_MACOS,
        keys::computer::OS_LINUX,
        keys::computer::OS_ANDROID,
        keys::computer::vendor,
        keys::computer::vendor_Apple,
        keys::computer::model,
        keys::computer::profileTier
    }});
    return allKeys;
}
