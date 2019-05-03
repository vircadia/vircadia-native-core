//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <string>
#include <vector>

namespace platform {

struct cpu {
    std::string brand;
    std::string model;
    int numberOfCores;
    std::string clockSpeed;
};

struct memory {
    int totalMb;
};

class Instance {
public:
    std::string virtual getProcessor(int index) = 0;
    bool virtual enumerateProcessors() = 0;
    int virtual getTotalSystemRamMb() = 0;
    int getProcessorCount() {return _processors.size(); }

protected:
    std::vector<cpu> _processors;
    struct memory _memory;
};

static Instance* _instance;

//Platform level functions
void create();
std::string getProcessor(int index);
int getProcessorCount();
bool enumerateProcessors();
int getTotalSystemRamMb();

}  // namespace platform