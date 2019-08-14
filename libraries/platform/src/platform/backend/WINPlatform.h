//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WinPlatform_h
#define hifi_WinPlatform_h

#include "PlatformInstance.h"

namespace platform {
    class WINInstance : public Instance {
    
    public:
        void enumerateCpus() override;
        void enumerateGpusAndDisplays() override;
        void enumerateMemory() override;
        void enumerateComputer () override;
        void enumerateNics() override;
        void enumerateGraphicsApis() override;
    };
}  // namespace platform

#endif //hifi_winplatform_h
