//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Platform_h
#define hifi_Platform_h

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
namespace platform {

class Instance {
public:
    bool virtual enumerateProcessors() = 0;

    int getNumProcessor() { return _processors.size(); }
    nlohmann::json getProcessor(int index);

    int getNumMemory() { return _memory.size(); }
    nlohmann::json getSystemRam(int index);

    int getNumGPU() { return _gpu.size(); }
    nlohmann::json getGPU(int index);

    ~Instance();

protected:
    std::vector<nlohmann::json> _processors;
    std::vector<nlohmann::json> _memory;
    std::vector<nlohmann::json*> _gpu;
};

//Platform level functions
void create();

bool enumerateProcessors();
int getNumProcessor();
nlohmann::json getProcessor(int index);
int getNumMemory();
nlohmann::json getSystemRam(int index);

}  // namespace platform

#endif  // hifi_platform_h