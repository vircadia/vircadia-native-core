//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AndroidPlatform_h
#define hifi_AndroidPlatform_h

#include "PlatformInstance.h"

namespace platform {
    class AndroidInstance : public Instance {

    public:
        void enumerateCpus() override;
        void enumerateGpusAndDisplays() override;
        void enumerateMemory() override;
        void enumerateComputer() override;
    };

}  // namespace platform

#endif //hifi_androidplatform_h
