//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "../Platform.h"
#include "../PlatformKeys.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include "../../CPUIdent.h"
#include <Psapi.h>

#endif

#include <QtCore/QDebug>
#include <QtCore/QOperatingSystemVersion>
#include <QSysInfo>
#include <QProcessEnvironment>
#include <QStringList>

Q_LOGGING_CATEGORY(platform_log, "platform")

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
    if (_instance) {
        return;
    }

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

json platform::getDescription() {
    auto all = _instance->getAll();
    json desc{};
    desc[platform::keys::COMPUTER] = all[platform::keys::COMPUTER];
    desc[platform::keys::MEMORY] = all[platform::keys::MEMORY];
    desc[platform::keys::CPUS] = all[platform::keys::CPUS];
    desc[platform::keys::GPUS] = all[platform::keys::GPUS];
    desc[platform::keys::DISPLAYS] = all[platform::keys::DISPLAYS];
    desc[platform::keys::NICS] = all[platform::keys::NICS];
    return desc;
}

void platform::printSystemInformation() {
    // Write system information to log
    qCDebug(platform_log) << "Build Information";
    qCDebug(platform_log).noquote() << "\tBuild ABI: " << QSysInfo::buildAbi();
    qCDebug(platform_log).noquote() << "\tBuild CPU Architecture: " << QSysInfo::buildCpuArchitecture();

    qCDebug(platform_log).noquote() << "System Information";
    qCDebug(platform_log).noquote() << "\tProduct Name: " << QSysInfo::prettyProductName();
    qCDebug(platform_log).noquote() << "\tCPU Architecture: " << QSysInfo::currentCpuArchitecture();
    qCDebug(platform_log).noquote() << "\tKernel Type: " << QSysInfo::kernelType();
    qCDebug(platform_log).noquote() << "\tKernel Version: " << QSysInfo::kernelVersion();

    qCDebug(platform_log) << "\tOS Version: " << QOperatingSystemVersion::current();

#ifdef Q_OS_WIN
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);

    qCDebug(platform_log) << "SYSTEM_INFO";
    qCDebug(platform_log).noquote() << "\tOEM ID: " << si.dwOemId;
    qCDebug(platform_log).noquote() << "\tProcessor Architecture: " << si.wProcessorArchitecture;
    qCDebug(platform_log).noquote() << "\tProcessor Type: " << si.dwProcessorType;
    qCDebug(platform_log).noquote() << "\tProcessor Level: " << si.wProcessorLevel;
    qCDebug(platform_log).noquote() << "\tProcessor Revision: "
                       << QString("0x%1").arg(si.wProcessorRevision, 4, 16, QChar('0'));
    qCDebug(platform_log).noquote() << "\tNumber of Processors: " << si.dwNumberOfProcessors;
    qCDebug(platform_log).noquote() << "\tPage size: " << si.dwPageSize << " Bytes";
    qCDebug(platform_log).noquote() << "\tMin Application Address: "
                       << QString("0x%1").arg(qulonglong(si.lpMinimumApplicationAddress), 16, 16, QChar('0'));
    qCDebug(platform_log).noquote() << "\tMax Application Address: "
                       << QString("0x%1").arg(qulonglong(si.lpMaximumApplicationAddress), 16, 16, QChar('0'));

    const double BYTES_TO_MEGABYTE = 1.0 / (1024 * 1024);

    qCDebug(platform_log) << "MEMORYSTATUSEX";
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        qCDebug(platform_log).noquote()
            << QString("\tCurrent System Memory Usage: %1%").arg(ms.dwMemoryLoad);
        qCDebug(platform_log).noquote()
            << QString("\tAvail Physical Memory: %1 MB").arg(ms.ullAvailPhys * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(platform_log).noquote()
            << QString("\tTotal Physical Memory: %1 MB").arg(ms.ullTotalPhys * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(platform_log).noquote()
            << QString("\tAvail in Page File:    %1 MB").arg(ms.ullAvailPageFile * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(platform_log).noquote()
            << QString("\tTotal in Page File:    %1 MB").arg(ms.ullTotalPageFile * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(platform_log).noquote()
            << QString("\tAvail Virtual Memory:  %1 MB").arg(ms.ullAvailVirtual * BYTES_TO_MEGABYTE, 20, 'f', 2);
        qCDebug(platform_log).noquote()
            << QString("\tTotal Virtual Memory:  %1 MB").arg(ms.ullTotalVirtual * BYTES_TO_MEGABYTE, 20, 'f', 2);
    } else {
        qCDebug(platform_log) << "\tFailed to retrieve memory status: " << GetLastError();
    }

    qCDebug(platform_log) << "CPUID";

    qCDebug(platform_log) << "\tCPU Vendor: " << CPUIdent::Vendor().c_str();
    qCDebug(platform_log) << "\tCPU Brand:  " << CPUIdent::Brand().c_str();

    for (auto& feature : CPUIdent::getAllFeatures()) {
        qCDebug(platform_log).nospace().noquote() << "\t[" << (feature.supported ? "x" : " ") << "] " << feature.name.c_str();
    }
#endif

    qCDebug(platform_log) << "Environment Variables";
    // List of env variables to include in the log. For privacy reasons we don't send all env variables.
    const QStringList envWhitelist = {
        "QTWEBENGINE_REMOTE_DEBUGGING"
    };
    auto envVariables = QProcessEnvironment::systemEnvironment();
    for (auto& env : envWhitelist)
    {
        qCDebug(platform_log).noquote().nospace() << "\t" <<
            (envVariables.contains(env) ? " = " + envVariables.value(env) : " NOT FOUND");
    }
}

bool platform::getMemoryInfo(MemoryInfo& info) {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) {
        return false;
    }

    info.totalMemoryBytes = ms.ullTotalPhys;
    info.availMemoryBytes = ms.ullAvailPhys;
    info.usedMemoryBytes = ms.ullTotalPhys - ms.ullAvailPhys;


    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        return false;
    }
    info.processUsedMemoryBytes = pmc.PrivateUsage;
    info.processPeakUsedMemoryBytes = pmc.PeakPagefileUsage;

    return true;
#endif

    return false;
}

// Largely taken from: https://msdn.microsoft.com/en-us/library/windows/desktop/ms683194(v=vs.85).aspx

#ifdef Q_OS_WIN
using LPFN_GLPI = BOOL(WINAPI*)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
    PDWORD);

DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
    DWORD i;

    for (i = 0; i <= LSHIFT; ++i) {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}
#endif

bool platform::getProcessorInfo(ProcessorInfo& info) {

#ifdef Q_OS_WIN
    LPFN_GLPI glpi;
    bool done = false;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD returnLength = 0;
    DWORD logicalProcessorCount = 0;
    DWORD numaNodeCount = 0;
    DWORD processorCoreCount = 0;
    DWORD processorL1CacheCount = 0;
    DWORD processorL2CacheCount = 0;
    DWORD processorL3CacheCount = 0;
    DWORD processorPackageCount = 0;
    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR Cache;

    glpi = (LPFN_GLPI)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),
        "GetLogicalProcessorInformation");
    if (nullptr == glpi) {
        qCDebug(platform_log) << "GetLogicalProcessorInformation is not supported.";
        return false;
    }

    while (!done) {
        DWORD rc = glpi(buffer, &returnLength);

        if (FALSE == rc) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if (buffer) {
                    free(buffer);
                }

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                    returnLength);

                if (NULL == buffer) {
                    qCDebug(platform_log) << "Error: Allocation failure";
                    return false;
                }
            } else {
                qCDebug(platform_log) << "Error " << GetLastError();
                return false;
            }
        } else {
            done = true;
        }
    }

    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) {
        switch (ptr->Relationship) {
        case RelationNumaNode:
            // Non-NUMA systems report a single record of this type.
            numaNodeCount++;
            break;

        case RelationProcessorCore:
            processorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;

        case RelationCache:
            // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache.
            Cache = &ptr->Cache;
            if (Cache->Level == 1) {
                processorL1CacheCount++;
            } else if (Cache->Level == 2) {
                processorL2CacheCount++;
            } else if (Cache->Level == 3) {
                processorL3CacheCount++;
            }
            break;

        case RelationProcessorPackage:
            // Logical processors share a physical package.
            processorPackageCount++;
            break;

        default:
            qCDebug(platform_log) << "\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n";
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    qCDebug(platform_log) << "GetLogicalProcessorInformation results:";
    qCDebug(platform_log) << "Number of NUMA nodes:" << numaNodeCount;
    qCDebug(platform_log) << "Number of physical processor packages:" << processorPackageCount;
    qCDebug(platform_log) << "Number of processor cores:" << processorCoreCount;
    qCDebug(platform_log) << "Number of logical processors:" << logicalProcessorCount;
    qCDebug(platform_log) << "Number of processor L1/L2/L3 caches:"
        << processorL1CacheCount
        << "/" << processorL2CacheCount
        << "/" << processorL3CacheCount;

    info.numPhysicalProcessorPackages = processorPackageCount;
    info.numProcessorCores = processorCoreCount;
    info.numLogicalProcessors = logicalProcessorCount;
    info.numProcessorCachesL1 = processorL1CacheCount;
    info.numProcessorCachesL2 = processorL2CacheCount;
    info.numProcessorCachesL3 = processorL3CacheCount;

    free(buffer);

    return true;
#endif

    return false;
}

