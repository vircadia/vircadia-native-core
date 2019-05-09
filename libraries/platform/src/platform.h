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
    bool virtual enumeratePlatform() = 0;

    int getNumCPU() { return _cpu.size(); }
    nlohmann::json getCPU(int index);

    int getNumGPU() { return _gpu.size(); }
    nlohmann::json getGPU(int index);

    int getNumMemory() { return _memory.size(); }
    nlohmann::json getMemory(int index);

    int getNumDisplay() { return _display.size(); }
    nlohmann::json getDisplay(int index);

    ~Instance();

protected:
    std::vector<nlohmann::json*>  _cpu;
    std::vector<nlohmann::json*>  _memory;
    std::vector<nlohmann::json*> _gpu;
    std::vector<nlohmann::json*> _display;

};

//Platform level functions
void create();
void destroy();

bool enumeratePlatform();

int getNumProcessor();
nlohmann::json getProcessor(int index);

int getNumGraphics();
nlohmann::json getGraphics(int index);

int getNumDisplay();
nlohmann::json getDisplay(int index);

int getNumMemory();
nlohmann::json getMemory(int index);

}  // namespace platform

#endif  // hifi_platform_h