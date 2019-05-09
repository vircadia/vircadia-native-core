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
#include "MACPlatform.h"
#endif

#ifdef Q_OS_LINUX
#endif

using namespace platform;
using namespace nlohmann;

Instance* _instance;

void platform::create() {

    #ifdef Q_OS_WIN
        _instance =new WINInstance();
    #elif Q_OS_MAC
    #endif
}

void platform::destroy() {
	delete _instance;
}

json Instance::getCPU(int index) {
    assert(index < _processor.size());

    json result;
    if (index >= _cpu.size())
        return result;

    return _cpu.at(index);
}


//These are ripe for template.. will work on that next
json Instance::getMemory(int index) { 
    
    assert(index < _memory.size());

    json result;
    if(index>= _memory.size())
        return result;

    return _memory.at(index);
}

json Instance::getGPU(int index) {
    assert(index < _gpu.size());

    json result;
    if (index >= _gpu.size())
        return result;

    return _gpu.at(index);
}

json Instance::getDisplay(int index) {
	assert(index < _display.size());

	json result;
	if (index >= _display.size())
		return result;

	return _display.at(index);
}

Instance::~Instance() {
    if (_cpu.size() > 0) {
    
       for (std::vector<json*>::iterator it = _cpu.begin(); it != _cpu.end(); ++it)   {
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

json platform::getProcessor(int index) {
    return _instance->getCPU(index);
}

int platform::getNumGraphics() {
	return _instance->getNumGPU();
}

nlohmann::json platform::getGraphics(int index) {
	return _instance->getGPU(index);
}

int platform::getNumDisplay() {
	return _instance->getNumDisplay();
}

nlohmann::json platform::getDisplay(int index) {
	return _instance->getDisplay(index);
}

json platform::getMemory(int index) {
    return _instance->getMemory(index);
}

int platform::getNumMemory() {
    return _instance->getNumMemory();
}

