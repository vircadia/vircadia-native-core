//
//  ControlSender.cpp
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-27.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <chrono>
#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include "ControlSender.h"

using namespace udt;

void ControlSender::loop() {
    while (!_isStopped) {
        // grab the time now
        auto start = std::chrono::high_resolution_clock::now();
        
        // for each of the connections managed by the udt::Socket, we need to ask for the ACK value to send
       
        // since we're infinitely looping, give the thread a chance to process events
        QCoreApplication::processEvents();
        
        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double, std::micro> elapsed = end - start;
        int timeToSleep = _synInterval - (int) elapsed.count();
        
        // based on how much time it took us to process, let's figure out how much time we have need to sleep
        std::this_thread::sleep_for(std::chrono::microseconds(timeToSleep));
    }
}
