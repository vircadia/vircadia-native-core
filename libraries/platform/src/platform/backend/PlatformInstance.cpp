//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "PlatformInstance.h"
#include <QNetworkInterface>

#include <gl/GLHelpers.h>
#include "../PlatformKeys.h"
#include "../Profiler.h"

// For testing the vulkan dump
//#define HAVE_VULKAN 1
//#pragma comment(lib, "C:\\VulkanSDK\\1.1.101.0\\Lib\\vulkan-1.lib")

#ifdef HAVE_VULKAN
#include <vulkan/vulkan.hpp>
#endif

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
    enumerateGraphicsApis();
    
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

#if defined(HAVE_VULKAN)
static std::string vkVersionToString(uint32_t version) {
    return QString("%1.%2.%3").arg(VK_VERSION_MAJOR(version)).arg(VK_VERSION_MINOR(version)).arg(VK_VERSION_PATCH(version)).toStdString();
}
#endif


void Instance::enumerateGraphicsApis() {
    // OpenGL rendering API is supported on all platforms
    {
        auto& glContextInfo = gl::ContextInfo::get();
        json gl;
        gl[keys::graphicsAPI::name] = keys::graphicsAPI::apiOpenGL;
        gl[keys::graphicsAPI::version] = glContextInfo.version;
        gl[keys::graphicsAPI::gl::vendor] = glContextInfo.vendor;
        gl[keys::graphicsAPI::gl::renderer] = glContextInfo.renderer;
        gl[keys::graphicsAPI::gl::shadingLanguageVersion] = glContextInfo.shadingLanguageVersion;
        gl[keys::graphicsAPI::gl::extensions] = glContextInfo.extensions;
        _graphicsApis.push_back(gl);
    }

#if defined(HAVE_VULKAN)
    // Vulkan rendering API is supported on all platforms (sort of)
    {
        try {
            vk::ApplicationInfo appInfo{ "Interface", 1, "Luci", 1, VK_API_VERSION_1_1 };
            auto instancePtr = vk::createInstanceUnique({ {},  &appInfo });
            if (instancePtr) {
                json vkinfo;
                const auto& vkinstance = *instancePtr;
                vkinfo[keys::graphicsAPI::name] = keys::graphicsAPI::apiVulkan;
                vkinfo[keys::graphicsAPI::version] = vkVersionToString(VK_API_VERSION_1_1);
                for (const auto& physicalDevice : vkinstance.enumeratePhysicalDevices()) {
                    json vkdevice;
                    auto properties = physicalDevice.getProperties();
                    vkdevice[keys::graphicsAPI::vk::device::driverVersion] = vkVersionToString(properties.driverVersion);
                    vkdevice[keys::graphicsAPI::vk::device::apiVersion] = vkVersionToString(properties.apiVersion);
                    vkdevice[keys::graphicsAPI::vk::device::deviceType] = vk::to_string(properties.deviceType);
                    vkdevice[keys::graphicsAPI::vk::device::vendor] = properties.vendorID;
                    vkdevice[keys::graphicsAPI::vk::device::name] = properties.deviceName;
                    for (const auto& extensionProperties : physicalDevice.enumerateDeviceExtensionProperties()) {
                        vkdevice[keys::graphicsAPI::vk::device::extensions].push_back(extensionProperties.extensionName);
                    }

                    for (const auto& queueFamilyProperties : physicalDevice.getQueueFamilyProperties()) {
                        json vkqueuefamily;
                        vkqueuefamily[keys::graphicsAPI::vk::device::queue::flags] = vk::to_string(queueFamilyProperties.queueFlags);
                        vkqueuefamily[keys::graphicsAPI::vk::device::queue::count] = queueFamilyProperties.queueCount;
                        vkdevice[keys::graphicsAPI::vk::device::queues].push_back(vkqueuefamily);
                    }
                    auto memoryProperties = physicalDevice.getMemoryProperties();
                    for (uint32_t heapIndex = 0; heapIndex < memoryProperties.memoryHeapCount; ++heapIndex) {
                        json vkmemoryheap;
                        const auto& heap = memoryProperties.memoryHeaps[heapIndex];
                        vkmemoryheap[keys::graphicsAPI::vk::device::heap::flags] = vk::to_string(heap.flags);
                        vkmemoryheap[keys::graphicsAPI::vk::device::heap::size] = heap.size;
                        vkdevice[keys::graphicsAPI::vk::device::heaps].push_back(vkmemoryheap);
                    }
                    vkinfo[keys::graphicsAPI::vk::devices].push_back(vkdevice);
                }
                _graphicsApis.push_back(vkinfo);
            }
        } catch (const std::runtime_error&) {
        }
    }
#endif
}

json Instance::getCPU(int index) {
    assert(index < (int)_cpus.size());

    if (index < 0 || (int)_cpus.size() <= index)
        return json();

    return _cpus.at(index);
}

json Instance::getGPU(int index) {
    assert(index < (int)_gpus.size());

    if (index < 0 || (int)_gpus.size() <= index)
        return json();

    return _gpus.at(index);
}

json Instance::getDisplay(int index) {
    assert(index < (int)_displays.size());

    if (index < 0 || (int)_displays.size() <= index)
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

        keys::graphicsAPI::version,
        keys::graphicsAPI::name,

        keys::graphicsAPI::gl::shadingLanguageVersion,
        keys::graphicsAPI::gl::vendor,
        keys::graphicsAPI::gl::renderer,
        keys::graphicsAPI::gl::extensions,

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
        keys::GRAPHICS_APIS,
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
    all[keys::GRAPHICS_APIS] = _graphicsApis;
    all[keys::DISPLAYS] = _displays;
    all[keys::NICS] = _nics;

    return all;
}
