//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Platform_h
#define hifi_Platform_h

#include <vector>
#include <nlohmann/json.hpp>

namespace platform {
    using json = nlohmann::json;
    
class Instance {
public:
    bool virtual enumeratePlatform() = 0;

    int getNumCPU() { return _cpu.size(); }
    json* getCPU(int index);

    int getNumGPU() { return _gpu.size(); }
    json* getGPU(int index);

    int getNumMemory() { return _memory.size(); }
    json* getMemory(int index);

    int getNumDisplay() { return _display.size(); }
    json* getDisplay(int index);

    virtual ~Instance();

protected:
    std::vector<json*>  _cpu;
    std::vector<json*>  _memory;
    std::vector<json*> _gpu;
    std::vector<json*> _display;
};

//Platform level functions
void create();
void destroy();

bool enumeratePlatform();

int getNumProcessor();
const json* getProcessor(int index);

int getNumGraphics();
const json* getGraphics(int index);

int getNumDisplay();
const json* getDisplay(int index);
    

int getNumMemory();
const json* getMemory(int index);

}  // namespace platform

#endif  // hifi_platform_h
