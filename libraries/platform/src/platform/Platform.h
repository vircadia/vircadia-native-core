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

int getNumCPUs();
json getCPU(int index);
int getMasterCPU();

int getNumGPUs();
json getGPU(int index);
int getMasterGPU();

int getNumDisplays();
json getDisplay(int index);
int getMasterDisplay();

json getMemory();

json getComputer();

json getAll();

}  // namespace platform

#endif  // hifi_platform_h
