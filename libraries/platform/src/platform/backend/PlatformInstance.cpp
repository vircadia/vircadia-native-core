//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "PlatformInstance.h"
#include <QNetworkInterface>

#include "../PlatformKeys.h"
#include "../Profiler.h"

using namespace platform;

bool Instance::enumeratePlatform() {
    enumerateComputer();
    enumerateMemory();
    enumerateCpus();
    enumerateGpus();
    enumerateDisplays();
    enumerateNics();

    // And profile the platform and put the tier in "computer"
    _computer[keys::computer::profileTier] = Profiler::TierNames[Profiler::profilePlatform()];

    return true;
}
void Instance::enumerateNics() {
    QNetworkInterface interface;
    foreach(interface, interface.allInterfaces()) {
        if (interface.flags().testFlag(QNetworkInterface::IsRunning) && !interface.hardwareAddress().isEmpty()) {
            json nic = {};
            nic[keys::nic::mac] = interface.hardwareAddress().toUtf8().constData();
            nic[keys::nic::name] = interface.humanReadableName().toUtf8().constData();
            _nics.push_back(nic);
        }
    }
}

json Instance::getCPU(int index) {
    assert(index <(int) _cpus.size());
    if (index >= (int)_cpus.size())
        return json();

    return _cpus.at(index);
}

json Instance::getGPU(int index) {
    assert(index <(int) _gpus.size());

    if (index >=(int) _gpus.size())
        return json();
    
    return _gpus.at(index);
}

json Instance::getDisplay(int index) {
    assert(index <(int) _displays.size());
    
    if (index >=(int) _displays.size())
        return json();

    return _displays.at(index);
}

Instance::~Instance() {
    if (_cpus.size() > 0) {
        _cpus.clear();
    }

    if (_gpus.size() > 0) {
        _gpus.clear();
    }

    if (_displays.size() > 0) {
        _displays.clear();
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

        keys::memory::memTotal,

        keys::computer::OS,
        keys::computer::OS_WINDOWS,
        keys::computer::OS_MACOS,
        keys::computer::OS_LINUX,
        keys::computer::OS_ANDROID,
        keys::computer::OSVersion,
        keys::computer::vendor,
        keys::computer::vendor_Apple,
        keys::computer::model,
        keys::computer::profileTier,

        keys::CPUS,
        keys::GPUS,
        keys::DISPLAYS,
        keys::MEMORY,
        keys::COMPUTER,
    }});
    return allKeys;
}

const char* Instance::findGPUVendorInDescription(const std::string& description) {
    // intel integrated graphics
    if (description.find(keys::gpu::vendor_Intel) != std::string::npos) {
        return keys::gpu::vendor_Intel;
    }
    // AMD gpu
    else if ((description.find(keys::gpu::vendor_AMD) != std::string::npos) || (description.find("Radeon") != std::string::npos)) {
        return keys::gpu::vendor_AMD;
    }
    // NVIDIA gpu
    else if (description.find(keys::gpu::vendor_NVIDIA) != std::string::npos) {
        return keys::gpu::vendor_NVIDIA;
    } else {
        return keys::UNKNOWN;
    }
}

json Instance::getAll() {
    json all = {};

    all[keys::COMPUTER] = _computer;
    all[keys::MEMORY] = _memory;
    all[keys::CPUS] = _cpus;
    all[keys::GPUS] = _gpus;
    all[keys::DISPLAYS] = _displays;
    all[keys::NICS] = _nics;

    return all;
}
