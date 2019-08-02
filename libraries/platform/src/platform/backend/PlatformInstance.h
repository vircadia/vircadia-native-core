//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PlatformInstance_h
#define hifi_PlatformInstance_h

#include <vector>
#include <nlohmann/json.hpp>

namespace platform {
    using json = nlohmann::json;
    
class Instance {
public:
    const int NOT_FOUND { -1 };

    bool virtual enumeratePlatform();

    int getNumCPUs() { return (int)_cpus.size(); }
    json getCPU(int index);
    int getMasterCPU() const { return _masterCPU; }

    int getNumGPUs() { return (int)_gpus.size(); }
    json getGPU(int index);
    int getMasterGPU() const { return _masterGPU; }

    int getNumDisplays() { return (int)_displays.size(); }
    json getDisplay(int index);
    int getMasterDisplay() const { return _masterDisplay; }

    json getMemory() { return _memory; }

    json getComputer() { return _computer; }
    
    json getAll();

    void virtual enumerateCpus()=0;
    void virtual enumerateGpusAndDisplays()=0;
    void virtual enumerateNics();
    void virtual enumerateMemory() = 0;
    void virtual enumerateComputer()=0;
    virtual void enumerateGraphicsApis();
    
    virtual ~Instance();

    static json listAllKeys();

    // Helper function to filter the vendor name out of the description of a GPU
    static const char* findGPUVendorInDescription(const std::string& description);

protected:
    std::vector<json>  _cpus;
    std::vector<json>  _gpus;
    std::vector<json>  _displays;
    std::vector<json>  _nics;
    json  _graphicsApis;
    json  _memory;
    json  _computer;

    int _masterCPU{ -1 };
    int _masterGPU{ -1 };
    int _masterDisplay{ -1 };

    // Traverse the cpus, gpus and displays to update the "master" index in each domain
    void updateMasterIndices();

};

}  // namespace platform

#endif  // hifi_platformInstance_h
