//
//  main.cpp
//  tools/container-profile/src
//
//  Created by Stephen Birarda on 8/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <algorithm>
#include <chrono>
#include <random>
#include <vector>
#include <unordered_map>

#include <HifiSockAddr.h>

using namespace std::chrono;

int main(int argc, char* argv[]) {
    static const int NUM_ELEMENTS = 50;
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> ipDistribution(1, UINT32_MAX);
    std::uniform_int_distribution<> portDistribution(1, UINT16_MAX);
    
    // from 1 to NUM_ELEMENTS, create a vector and unordered map with that many elements and compare how long it takes to pull the last value
    for (int i = 1; i <= NUM_ELEMENTS; ++i) {
        // create our vector with i elements
        std::vector<std::pair<HifiSockAddr, int>> vectorElements(i);
        
        // create our unordered map with i elements
        std::unordered_map<HifiSockAddr, int> hashElements;
        
        HifiSockAddr lastAddress;
        
        // fill the structures with HifiSockAddr
        for (int j = 0; j < i; ++j) {
            // create a random IP address
            quint32 randomNumber = ipDistribution(generator);
            quint32 randomIP = (randomNumber >> 24 & 0xFF) << 24 |
            (randomNumber >> 16 & 0xFF) << 16 |
            (randomNumber >> 8 & 0xFF) << 8 |
            (randomNumber & 0xFF);
            
            HifiSockAddr randomAddress(QHostAddress(randomIP), portDistribution(generator));
            
            vectorElements.push_back({ randomAddress, 12 });
            hashElements.insert({ randomAddress, 12 });
            
            if (j == i - 1) {
                // this is the last element - store it for lookup purposes
                lastAddress = randomAddress;
            }
        }
        
        // time how long it takes to get the value for the key lastAddress, for each container
        auto before = high_resolution_clock::now();
        int vectorResult;
        
        for (auto& vi : vectorElements) {
            if (vi.first == lastAddress) {
                vectorResult = vi.second;;
                break;
            }
        }
        
        auto vectorDuration = duration_cast<nanoseconds>(high_resolution_clock::now() - before);
        
        Q_UNUSED(vectorResult);
        
        before = high_resolution_clock::now();
        auto mi = hashElements.find(lastAddress);
        int mapResult = mi->second;
        auto mapDuration = duration_cast<nanoseconds>(high_resolution_clock::now() - before);
        
        Q_UNUSED(mapResult);
        
        qDebug() << i << QString("v: %1ns").arg(vectorDuration.count()) << QString("m: %2ns").arg(mapDuration.count());
    }
}

