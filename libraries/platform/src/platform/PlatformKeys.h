//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_platform_PlatformKeys_h
#define hifi_platform_PlatformKeys_h

namespace platform { namespace keys{
    // "UNKNOWN"
    extern const char*  UNKNOWN;

    namespace cpu {
        extern const char*  vendor;
        extern const char*  vendor_Intel;
        extern const char*  vendor_AMD;

        extern const char*  model;
        extern const char*  clockSpeed;
        extern const char*  numCores;
        extern const char*  isMaster;
    }
    namespace gpu {
        extern const char*  vendor;
        extern const char*  vendor_NVIDIA;
        extern const char*  vendor_AMD;
        extern const char*  vendor_Intel;

        extern const char*  model;
        extern const char*  videoMemory;
        extern const char*  driver;
        extern const char*  displays;
        extern const char*  isMaster;
    }
    namespace graphicsAPI {
        extern const char* name;
        extern const char* version;
        extern const char* apiOpenGL;
        extern const char* apiVulkan;
        extern const char* apiDirect3D11;
        extern const char* apiDirect3D12;
        extern const char* apiMetal;
        namespace gl {
            extern const char* shadingLanguageVersion;
            extern const char* vendor;
            extern const char* renderer;
            extern const char* extensions;
        }
        namespace vk {
            extern const char* devices;
            namespace device {
                extern const char* apiVersion;
                extern const char* driverVersion;
                extern const char* deviceType;
                extern const char* vendor;
                extern const char* name;
                extern const char* formats;
                extern const char* extensions;
                extern const char* queues;
                extern const char* heaps;
                namespace heap {
                    extern const char* flags;
                    extern const char* size;
                }
                namespace queue {
                    extern const char* flags;
                    extern const char* count;
                }
            }
        }
    }
    namespace nic {
        extern const char* mac;
        extern const char* name;
    }
    namespace display {
        extern const char*  description;
        extern const char*  name;
        extern const char*  boundsLeft;
        extern const char*  boundsRight;
        extern const char*  boundsTop;
        extern const char*  boundsBottom;
        extern const char*  gpu;
        extern const char*  ppi;
        extern const char*  ppiDesktop;
        extern const char*  physicalWidth;
        extern const char*  physicalHeight;
        extern const char*  modeRefreshrate;
        extern const char*  modeWidth;
        extern const char*  modeHeight;
        extern const char*  isMaster;
    }
    namespace memory {
        extern const char*  memTotal;
    }
    namespace computer {
        extern const char*  OS;
        extern const char*  OS_WINDOWS;
        extern const char*  OS_MACOS;
        extern const char*  OS_LINUX;
        extern const char*  OS_ANDROID;

        extern const char*  OSVersion;

        extern const char*  vendor;
        extern const char*  vendor_Apple;
        
        extern const char*  model;

        extern const char*  profileTier;
    }

    // Keys for categories used in json returned by getAll()
    extern const char*  CPUS;
    extern const char*  GPUS;
    extern const char*  GRAPHICS_APIS;
    extern const char*  DISPLAYS;
    extern const char*  NICS;
    extern const char*  MEMORY;
    extern const char*  COMPUTER;

} } // namespace plaform::keys

#endif
