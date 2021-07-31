//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "../Platform.h"
#include "../PlatformKeys.h"

/*@jsdoc
 * Information on the computer platform as a whole.
 * @typedef {object} PlatformInfo.PlatformDescription
 * @property {PlatformInfo.ComputerDescription} computer - Information on the computer.
 * @property {PlatformInfo.CPUDescription[]} cpus - Information on the computer's CPUs.
 * @property {PlatformInfo.DisplayDescription[]} displays - Information on the computer's displays.
 * @property {PlatformInfo.GPUDescription[]} gpus - Information on the computer's GPUs.
 * @property {PlatformInfo.GraphicsAPIDescription[]} graphicsAPIs - Information on the computer's graphics APIs.
 * @property {PlatformInfo.MemoryDescription} memory - Information on the computer's memory.
 * @property {PlatformInfo.NICDescription} nics - Information on the computer's network cards.
 */
namespace platform { namespace keys {
    const char*  UNKNOWN = "UNKNOWN";

    /*@jsdoc
     * Information on a CPU.
     * @typedef {object} PlatformInfo.CPUDescription
     * @property {string} vendor - The CPU vendor (e.g., <code>"Intel"</code> or <code>"AMD"</code>).
     * @property {string} model - The CPU model.
     * @property {number} numCores - The number of logical cores.
     * @property {boolean} isMaster - <code>true</code> if the CPU is the "master" or primary CPU, <code>false</code> or 
     *     <code>undefined</code> if it isn't.
     */
    namespace cpu {
        const char*  vendor = "vendor";
        const char*  vendor_Intel = "Intel";
        const char*  vendor_AMD = "AMD";

        const char*  model = "model";
        const char*  clockSpeed = "clockSpeed";  // FIXME: Not used.
        const char*  numCores = "numCores";
        const char*  isMaster = "isMaster";
        }

    /*@jsdoc
     * Information on a GPU.
     * @typedef {object} PlatformInfo.GPUDescription
     * @property {string} vendor - The GPU vendor (e.g., <code>"NVIDIA"</code>, <code>"AMD"</code>, or <code>"Intel"</code>).
     * @property {string} model - The GPU model.
     * @property {string} driver - The GPU driver version.
     * @property {number} videoMemory - The size of the GPU's video memory, in MB.
     * @property {number[]} displays - The index numbers of the displays currently being driven by the GPU. An empty array if 
     *     the GPU is currently not driving any displays.
     * @property {boolean} isMaster - <code>true</code> if the GPU is the "master" or primary GPU, <code>false</code> or
     *     <code>undefined</code> if it isn't.
     */
    namespace gpu {
        const char*  vendor = "vendor";
        const char*  vendor_NVIDIA = "NVIDIA";
        const char*  vendor_AMD = "AMD";
        const char*  vendor_Intel = "Intel";

        const char*  model = "model";
        const char*  videoMemory = "videoMemory";
        const char*  driver = "driver";
        const char*  displays = "displays";
        const char*  isMaster = "isMaster";
    }
    
    /*@jsdoc
     * Information on a graphics API.
     * @typedef {object} PlatformInfo.GraphicsAPIDescription
     * @property {string} name - The name of the graphics API.
     * @property {string} version - The version of the graphics API.
     *
     * @property {string} [renderer] - If an OpenGL API, then the graphics card that performs the rendering.
     * @property {string} [vendor] - If an OpenGL API, then the OpenGL vendor.
     * @property {string} [shadingLanguageVersion] - If an OpenGL API, then the shading language version.
     * @property {string[]} [extensions] - If an OpenGL API, then the list of OpenGL extensions supported.
     *
     * @property {PlatformInfo.VulkanAPIDescription[]} [devices] - If a Vulkan API, then the devices provided in the API.
     */
    /*@jsdoc
     * Information on a Vulkan graphics API.
     * @typedef {object} PlatformInfo.VulkanAPIDescription
     * @property {string} 
     * @property {string} driverVersion - The driver version.
     * @property {string} apiVersion - The API version.
     * @property {string} deviceType - The device type.
     * @property {string} vendor - The device vendor.
     * @property {string} name - The device name.
     * @property {string[]} extensions - The list of Vulkan extensions supported.
     * @property {PlatformInfo.VulkanQueueDescription[]} queues - The Vulkan queues available.
     * @property {PlatformInfo.VulkanHeapDescription[]} heaps - The Vulkan heaps available.
     */
    /*@jsdoc
     * Information on a Vulkan queue.
     * @typedef {object} PlatformInfo.VulkanQueueDescription
     * @property {string} flags - The Vulkan queue flags.
     * @property {number} count - The queue count.
     */
    /*@jsdoc
     * Information on a Vulkan heap.
     * @typedef {object} PlatformInfo.VulkanHeapDescription
     * @property {string} flags - The Vulkan heap flags.
     * @property {number} size - The heap size.
     */
    namespace graphicsAPI {
        const char* name = "name";
        const char* version = "version";

        const char* apiOpenGL = "OpenGL";
        const char* apiVulkan = "Vulkan";
        const char* apiDirect3D11 = "D3D11";
        const char* apiDirect3D12 = "D3D12";
        const char* apiMetal = "Metal";

        namespace gl {
            const char* shadingLanguageVersion = "shadingLanguageVersion";
            const char* vendor = "vendor";
            const char* renderer = "renderer";
            const char* extensions = "extensions";
        }
        namespace vk {
            const char* devices = "devices";
            namespace device {
                const char* apiVersion = "apiVersion";
                const char* driverVersion = "driverVersion";
                const char* deviceType = "deviceType";
                const char* vendor = "vendor";
                const char* name = "name";
                const char* formats = "formats";
                const char* extensions = "extensions";
                const char* heaps = "heaps";
                namespace heap {
                    const char* flags = "flags";
                    const char* size = "size";
                }
                const char* queues = "queues";
                namespace queue {
                    const char* flags = "flags";
                    const char* count = "count";
                }
            }
        }
    }

    /*@jsdoc
     * Information on a network card.
     * @typedef {object} PlatformInfo.NICDescription
     * @property {string} name - The name of the network card.
     * @property {string} mac - The MAC address of the network card.
     */
    namespace nic {
        const char* mac = "mac";
        const char* name = "name";
    }

    /*@jsdoc
     * Information on a display.
     * @typedef {object} PlatformInfo.DisplayDescription
     * @property {string} description - The display's description.
     * @property {string} deviceName - The display's device name.
     * @property {number} boundsLeft - The pixel coordinate of the left edge of the display (e.g., <code>0</code>).
     * @property {number} boundsRight - The pixel coordinate of the right edge of the display (e.g., <code>1920</code>).
     * @property {number} boundsTop - The pixel coordinate of the top edge of the display (e.g., <code>0</code>).
     * @property {number} boundsBottom - The pixel coordinate of the bottom edge of the display (e.g., <code>1080</code>).
     * @property {number} gpu - The index number of the GPU that's driving the display.
     * @property {number} ppi - The physical dots per inch of the display.
     * @property {number} ppiDesktop - The logical dots per inch of the desktop as used by the operating system.
     * @property {number} physicalWidth - The physical width of the display, in inches.
     * @property {number} physicalHeight - The physical height of the display, in inches.
     * @property {number} modeRefreshrate - The refresh rate of the current display mode, in Hz.
     * @property {number} modeWidth - The width of the current display mode, in pixels.
     * @property {number} modeHeight - The height of the current display mode, in pixels.
     * @property {boolean} isMaster - <code>true</code> if the GPU is the "master" or primary display, <code>false</code> or
     *     <code>undefined</code> if it isn't.
     */
    namespace display {
        const char*  description = "description";
        const char*  name = "deviceName";
        const char*  boundsLeft = "boundsLeft";
        const char*  boundsRight = "boundsRight";
        const char*  boundsTop = "boundsTop";
        const char*  boundsBottom = "boundsBottom";
        const char*  gpu = "gpu";
        const char*  ppi = "ppi";
        const char*  ppiDesktop = "ppiDesktop";
        const char*  physicalWidth = "physicalWidth";
        const char*  physicalHeight = "physicalHeight";
        const char*  modeRefreshrate = "modeRefreshrate";
        const char*  modeWidth = "modeWidth";
        const char*  modeHeight = "modeHeight";
        const char*  isMaster = "isMaster";
    }

    /*@jsdoc
     * Information on the computer's memory.
     * @typedef {object} PlatformInfo.MemoryDescription
     * @property {number} memTotal - The total amount of usable physical memory, in MB.
     */
    namespace memory {
        const char*  memTotal = "memTotal";
    }

    /*@jsdoc
     * Information on the computer.
     * @typedef {object} PlatformInfo.ComputerDescription
     * @property {PlatformInfo.ComputerOS} OS - The operating system.
     * @property {string} OSversion - The operating system version.
     * @property {string} vendor - The computer vendor.
     * @property {string} model - The computer model.
     * @property {PlatformInfo.PlatformTier} profileTier - The platform tier of the computer, profiled at Interface start-up.
     */
    /*@jsdoc
     * <p>The computer operating system.</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>"WINDOWS"</code></td><td>Windows.</td></tr>
     *     <tr><td><code>"MACOS"</code></td><td>Mac OS.</td></tr>
     *     <tr><td><code>"LINUX"</code></td><td>Linux.</td></tr>
     *     <tr><td><code>"ANDROID"</code></td><td>Android.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {string} PlatformInfo.ComputerOS
     */
    namespace computer {
        const char*  OS = "OS";
        const char*  OS_WINDOWS = "WINDOWS";
        const char*  OS_MACOS = "MACOS";
        const char*  OS_LINUX = "LINUX";
        const char*  OS_ANDROID = "ANDROID";

        const char*  OSVersion = "OSVersion";

        const char*  vendor = "vendor";
        const char*  vendor_Apple = "Apple";

        const char*  model = "model";

        const char*  profileTier = "profileTier";
    }

    const char*  CPUS = "cpus";
    const char*  GPUS = "gpus";
    const char*  GRAPHICS_APIS = "graphicsAPIs";
    const char*  DISPLAYS = "displays";
    const char*  NICS = "nics";
    const char*  MEMORY = "memory";
    const char*  COMPUTER = "computer";
}}

#include <qglobal.h>

#if defined(Q_OS_WIN)
#include "WINPlatform.h"
#elif defined(Q_OS_MAC)
#include "MACOSPlatform.h"
#elif defined(Q_OS_ANDROID)
#include "AndroidPlatform.h"
#elif defined(Q_OS_LINUX)
#include "LinuxPlatform.h"
#endif

using namespace platform;

Instance *_instance;

void platform::create() {
#if defined(Q_OS_WIN)
    _instance =new WINInstance();
#elif defined(Q_OS_MAC)
    _instance = new MACOSInstance();
#elif defined(Q_OS_ANDROID)
    _instance= new AndroidInstance();
#elif defined(Q_OS_LINUX)
    _instance= new LinuxInstance();
#endif
}

void platform::destroy() {
    if(_instance)
        delete _instance;
}

bool platform::enumeratePlatform() {
    return _instance->enumeratePlatform();
}

int platform::getNumCPUs() {
    return _instance->getNumCPUs();
}

json platform::getCPU(int index) {
    return _instance->getCPU(index);
}

int platform::getMasterCPU() {
    return _instance->getMasterCPU();
}

int platform::getNumGPUs() {
    return _instance->getNumGPUs();
}

json platform::getGPU(int index) {
    return _instance->getGPU(index);
}

int platform::getMasterGPU() {
    return _instance->getMasterGPU();
}

int platform::getNumDisplays() {
    return _instance->getNumDisplays();
}

json platform::getDisplay(int index) {
    return _instance->getDisplay(index);
}

int platform::getMasterDisplay() {
    return _instance->getMasterDisplay();
}

json platform::getMemory() {
    return _instance->getMemory();
}

json platform::getComputer() {
    return _instance->getComputer();
}

json platform::getAll() {
    return _instance->getAll();
}
