//
//  Profiler.cpp
//  libraries/platform/src/platform
//
//  Created by Sam Gateau on 5/22/2019.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Profiler.h"

#include "Platform.h"
#include "PlatformKeys.h"

using namespace platform;

const std::array<const char*, Profiler::Tier::NumTiers> Profiler::TierNames = {{ "UNKNOWN", "LOW", "MID", "HIGH" }};


bool filterOnComputer(const platform::json& computer, Profiler::Tier& tier);
bool filterOnComputerMACOS(const platform::json& computer, Profiler::Tier& tier);
bool filterOnProcessors(const platform::json& computer, const platform::json& cpu, const platform::json& gpu, Profiler::Tier& tier);

Profiler::Tier Profiler::profilePlatform() {
    auto platformTier = Profiler::Tier::UNKNOWN; // unknown tier yet

    // first filter on the computer info and catch known models
    auto computerInfo = platform::getComputer();
    if (filterOnComputer(computerInfo, platformTier)) {
        return platformTier;
    }

    // Not filtered yet, let s try to make sense of the cpu and gpu info
    auto cpuInfo = platform::getCPU(0);
    auto gpuInfo = platform::getGPU(0);
    if (filterOnProcessors(computerInfo, cpuInfo, gpuInfo, platformTier)) {
        return platformTier;
    }

    // Default answer is 
    return Profiler::Tier::LOW;
}

// tier filter on computer info
bool filterOnComputer(const platform::json& computer, Profiler::Tier& tier) {
    if (computer.count(keys::computer::OS)) {
        const auto os = computer[keys::computer::OS].get<std::string>();
        if (os.compare(keys::computer::OS_MACOS) == 0) {
            return filterOnComputerMACOS(computer, tier);
        }
    }
    return false;
}


// tier filter on computer MACOS
bool filterOnComputerMACOS(const platform::json& computer, Profiler::Tier& tier) {

    // it s a macos computer, probably can tell from the model name:
    if (computer.count(keys::computer::model)) {
        const auto model = computer[keys::computer::model].get<std::string>();
        if (model.find("MacBookAir") != std::string::npos) {
            tier = Profiler::Tier::LOW;
            return true;
        } else if (model.find("MacBookPro") != std::string::npos) {
            tier = Profiler::Tier::MID;
            return true;
        } else if (model.find("MacBook") != std::string::npos) {
            tier = Profiler::Tier::LOW;
            return true;
        }
    }
    return false;
}

bool filterOnProcessors(const platform::json& computer, const platform::json& cpu, const platform::json& gpu, Profiler::Tier& tier) {
    // first filter on the number of cores, <= 4 means LOW
    int numCores = 0;
    std::string cpuVendor;
    if (cpu.count(keys::cpu::numCores)) {
        numCores = cpu[keys::cpu::numCores].get<int>();
        if (numCores <= 4) {
            tier = Profiler::Tier::LOW;
            return true;
        }

        cpuVendor = cpu[keys::cpu::vendor].get<std::string>();
    }

    // enough cores to be mid or high
    // let s look at the gpu
    if (gpu.count(keys::gpu::vendor)) {
        std::string gpuVendor = gpu[keys::gpu::vendor].get<std::string>();
        std::string gpuModel = gpu[keys::gpu::model].get<std::string>();

        // intel integrated graphics
        if (gpuVendor.find(keys::gpu::vendor_Intel) != std::string::npos) {
            // go mid because GPU
            tier = Profiler::Tier::MID;
            return true;
        }
        // AMD gpu
        else if (gpuVendor.find(keys::gpu::vendor_AMD) != std::string::npos) {
            // TODO: Filter base on the model of AMD
            // that is their  integrated graphics, go low!
            if (gpuModel.find("Vega 3") != std::string::npos) {
                tier = Profiler::Tier::LOW;
                return true;
            }
            
            // go high because GPU
            tier = Profiler::Tier::HIGH;
            return true;
        }
        // NVIDIA gpu
        else if (gpuVendor.find(keys::gpu::vendor_NVIDIA) != std::string::npos) {
            // TODO: Filter base on the model of NV
            // go high because GPU
            tier = Profiler::Tier::HIGH;
            return true;
        }
    }

    // Not able to profile
    return false;
}

// Ugly very adhoc capability check to know if a particular hw can REnder with Deferred method or not
// YES for PC  windows and linux
// NO for android
// YES on macos EXCEPT for macbookair with gpu intel iris or intel HD 6000
bool Profiler::isRenderMethodDeferredCapable() {
#if defined(Q_OS_MAC)
    auto computerInfo = platform::getComputer();
    if (computer.count(keys::computer::model)) {
        const auto model = computer[keys::computer::model].get<std::string>();
        if (model.find("MacBookAir") != std::string::npos) {
            return false;
        }
    }


/*    auto gpuInfo = platform::getGPU(0);
    if (gpuInfo.count(keys::gpu::model)) {
        const auto model = computer[keys::gpu::model].get<std::string>();
        if (model.find("MacBookAir") != std::string::npos) {
        }
    }
*/
    return true;
#elif defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}