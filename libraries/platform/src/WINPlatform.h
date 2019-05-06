//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WinPlatform_h
#define hifi_WinPlatform_h

#include "platform.h"
#include <nlohmann/json.hpp>

namespace platform {

    using namespace nlohmann;

    class WINInstance : public Instance {
    
    public:
        bool enumerateProcessors();

    private:
        unsigned int getNumLogicalCores();
        void getCpuDetails(nlohmann::json& cpu);
        int getTotalSystemRam();
};

}  // namespace platform

#endif //hifi_winplatform_h