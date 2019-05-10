//
//  Created by Amer Cerkic 05/02/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "platform.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
#include "WINPlatform.h"
#endif

#ifdef Q_OS_MACOS
#include "MACOSPlatform.h"
#endif

#ifdef Q_OS_LINUX
#endif

using namespace platform;
using namespace nlohmann;

Instance* _instance;

void platform::create() {

    #ifdef Q_OS_WIN
        _instance =new WINInstance();
    #endif
    
    #ifdef Q_OS_MAC
    _instance = new MACOSInstance();
    #endif
}

void platform::destroy() {
	delete _instance;
}

 json* Instance::getCPU(int index) {
    assert(index <(int) _cpu.size());
    if (index >= (int)_cpu.size())
        return nullptr;

    return _cpu.at(index);
}


//These are ripe for template.. will work on that next
json* Instance::getMemory(int index) {
    assert(index <(int) _memory.size());
    if(index >= (int)_memory.size())
        return nullptr;

    return _memory.at(index);
}

json* Instance::getGPU(int index) {
    assert(index <(int) _gpu.size());
    if (index >=(int) _gpu.size())
        return nullptr;

    return _gpu.at(index);
}

json* Instance::getDisplay(int index) {
	assert(index <(int) _display.size());
    
	if (index >=(int) _display.size())
		return nullptr;

	return _display.at(index);
}

Instance::~Instance() {
	if (_cpu.size() > 0) {

		for (std::vector<json*>::iterator it = _cpu.begin(); it != _cpu.end(); ++it) {
			delete (*it);
		}
		_cpu.clear();
	}

	if (_memory.size() > 0) {
		for (std::vector<json*>::iterator it = _memory.begin(); it != _memory.end(); ++it) {
			delete (*it);
		}
		_memory.clear();
	}


	if (_gpu.size() > 0) {
		for (std::vector<json*>::iterator it = _gpu.begin(); it != _gpu.end(); ++it) {
			delete (*it);
		}
				_gpu.clear();
	}

	if (_display.size() > 0) {
		for (std::vector<json*>::iterator it = _display.begin(); it != _display.end(); ++it) {
			delete (*it);
		}

		_display.clear();
	}
}

bool platform::enumeratePlatform() {
    return _instance->enumeratePlatform();
}

int platform::getNumProcessor() {
	return _instance->getNumCPU();
}

const json* platform::getProcessor(int index) {
    return _instance->getCPU(index);
}

int platform::getNumGraphics() {
	return _instance->getNumGPU();
}

const json* platform::getGraphics(int index) {
	return _instance->getGPU(index);
}

int platform::getNumDisplay() {
	return _instance->getNumDisplay();
}

const json* platform::getDisplay(int index) {
	return _instance->getDisplay(index);
}

int platform::getNumMemory() {
    return _instance->getNumMemory();
}

const json* platform::getMemory(int index) {
    return _instance->getMemory(index);
}



