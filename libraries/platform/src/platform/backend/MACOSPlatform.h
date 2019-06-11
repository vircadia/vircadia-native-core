//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MACOSPlatform_h
#define hifi_MACOSPlatform_h

#include "PlatformInstance.h"

namespace platform {
    class MACOSInstance : public Instance {

    public:
        void enumerateCpu() override;
        void enumerateMemory() override;
        void enumerateGpu() override;
        void enumerateDisplays() override;
        void enumerateComputer () override;
    };

}  // namespace platform

#endif //hifi_macosplatform_h
