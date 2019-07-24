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
    //clear all knowledge
    _computer.clear();
    _memory.clear();
    _cpus.clear();
    _gpus.clear();
    _displays.clear();
    _nics.clear();

    // enumerate platform components
    enumerateComputer();
    enumerateMemory();
    enumerateCpus();
    enumerateGpusAndDisplays();
    enumerateNics();
    
    // eval the master index for each platform scopes
    updateMasterIndices();

    // And profile the platform and put the tier in "computer"
    _computer[keys::computer::profileTier] = Profiler::TierNames[Profiler::profilePlatform()];

    return true;
}

void Instance::updateMasterIndices() {
    // We assume a single CPU at the moment:
    {
        if (!_cpus.empty()) {
            _masterCPU = 0;
            _cpus[0][keys::cpu::isMaster] = true; 
        } else {
            _masterCPU = NOT_FOUND;
        }
    }

    // Go through the displays list
    {
        _masterDisplay = NOT_FOUND;
        for (int i = 0; i < (int) _displays.size(); ++i) {
            const auto& display = _displays[i];
            if (display.count(keys::display::isMaster)) {
                if (display[keys::display::isMaster].get<bool>()) {
                    _masterDisplay = i;
                    break;
                }
            }
        }
        // NO master display found, return the first one or NOT_FOUND if no display
        if (_masterDisplay == NOT_FOUND) {
            if (!_displays.empty()) {
                _masterDisplay = 0;
                _displays[0][keys::display::isMaster] = true;
            }
        }
    }

    // From the master display decide the master gpu
    {
        _masterGPU = NOT_FOUND;
        if (_masterDisplay != NOT_FOUND) {
            const auto& display = _displays[_masterDisplay];
            // FInd the GPU index of the master display
            if (display.count(keys::display::gpu)) {
                _masterGPU = display[keys::display::gpu];
                _gpus[_masterGPU][keys::gpu::isMaster] = true; 
            }
        }
        // NO master GPU found from master display, bummer, return the first one or NOT_FOUND if no display
        if (_masterGPU == NOT_FOUND) {
            if (!_gpus.empty()) {
                _masterGPU = 0;
                _gpus[0][keys::gpu::isMaster] = true;
            }
        }        
    }
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

    if (index < 0 || (int) _cpus.size() <= index)
        return json();

    return _cpus.at(index);
}

json Instance::getGPU(int index) {
    assert(index <(int) _gpus.size());

    if (index < 0 || (int) _gpus.size() <= index)
        return json();
    
    return _gpus.at(index);
}


json Instance::getDisplay(int index) {
    assert(index <(int) _displays.size());
    
    if (index < 0 || (int) _displays.size() <= index)
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
        keys::gpu::displays,

        keys::display::boundsLeft,
        keys::display::boundsRight,
        keys::display::boundsTop,
        keys::display::boundsBottom,
        keys::display::gpu,

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
