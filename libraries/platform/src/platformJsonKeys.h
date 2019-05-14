//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PlatformJsonKeys_h
#define hifi_PlatformJsonKeys_h


namespace platform {
    namespace jsonKeys{
#if !defined(Q_OS_LINUX) //hiding from linux at the moment due to unused variables warning
        static const char*  cpuBrand= "cpuBrand";
        static const char*  cpuModel = "cpuModel";
        static const char*  cpuClockSpeed = "clockSpeed";
        static const char*  cpuNumCores = "numCores";
        static const char*  gpuName = "GpuName";
        static const char*  gpuMemory = "gpuMemory";
        static const char*  gpuDriver = "gpuDriver";
        static const char*  totalMemory = "totalMem";
        static const char*  displayDescription = "description";
        static const char*  displayName = "deviceName";
        static const char*  displayCoordsLeft = "coordinatesleft";
        static const char*  displayCoordsRight = "coordinatesright";
        static const char*  displayCoordsTop = "coordinatestop";
        static const char*  displayCoordsBottom = "coordinatesbottom";
#endif
	}

} // namespace platform

#endif  
