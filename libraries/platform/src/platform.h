//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Platform_h
#define hifi_Platform_h


#include <nlohmann/json.hpp>

namespace platform {
    using json = nlohmann::json;
    
void create();
void destroy();
bool enumeratePlatform();

int getNumCPU();
json getCPU(int index);

int getNumGPU();
json getGPU(int index);

int getNumDisplay();
json getDisplay(int index);
    
int getNumMemory();
json getMemory(int index);

json getComputer();
    
}  // namespace platform

#endif  // hifi_platform_h
