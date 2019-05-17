//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "platformInstance.h"
#include <QtGlobal>

using namespace platform;

bool Instance::enumeratePlatform() {
    enumerateComputer();
    enumerateCpu();
    enumerateGpu();
    enumerateMemory();
    return true;
}

json Instance::getCPU(int index) {
    assert(index <(int) _cpu.size());
    if (index >= (int)_cpu.size())
        return json();

    return _cpu.at(index);
}

//These are ripe for template.. will work on that next
json Instance::getMemory(int index) {
    assert(index <(int) _memory.size());
    if(index >= (int)_memory.size())
        return json();

    return _memory.at(index);
}

json Instance::getGPU(int index) {
    assert(index <(int) _gpu.size());

    if (index >=(int) _gpu.size())
        return json();
    
    return _gpu.at(index);
}

json Instance::getDisplay(int index) {
    assert(index <(int) _display.size());
    
    if (index >=(int) _display.size())
        return json();

    return _display.at(index);
}

Instance::~Instance() {
    if (_cpu.size() > 0) {
        _cpu.clear();
    }

    if (_memory.size() > 0) {
        _memory.clear();
    }

    if (_gpu.size() > 0) {
        _gpu.clear();
    }

    if (_display.size() > 0) {
        _display.clear();
    }
}
