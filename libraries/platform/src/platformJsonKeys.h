//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_PlatformJsonKeys_h
#define hifi_PlatformJsonKeys_h

namespace platform {
    namespace jsonKeys{
        const char*  cpuBrand { "cpuBrand"};
        const char*  cpuModel {"cpuModel"};
        const char*  cpuClockSpeed {"clockSpeed"};
        const char*  cpuNumCores { "numCores"};
        const char*  gpuName  {"GpuName"};
        const char*  gpuMemory {"gpuMemory"};
        const char*  gpuDriver  {"gpuDriver"};
        const char*  totalMemory  {"totalMem"};
        const char*  displayDescription { "description"};
        const char*  displayName  {"deviceName"};
        const char*  displayCoordsLeft {"coordinatesleft"};
        const char*  displayCoordsRight { "coordinatesright"};
        const char*  displayCoordsTop { "coordinatestop"};
        const char*  displayCoordsBottom { "coordinatesbottom"};
    }

} // namespace platform

#endif
