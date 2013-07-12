//
//  AudioInjectionManager.h
//  hifi
//
//  Created by Stephen Birarda on 5/16/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjectionManager__
#define __hifi__AudioInjectionManager__

#include <iostream>

#include "UDPSocket.h"
#include "AudioInjector.h"

const int MAX_CONCURRENT_INJECTORS = 50;

class AudioInjectionManager {
public:
    static AudioInjector* injectorWithCapacity(int capacity);
    static AudioInjector* injectorWithSamplesFromFile(const char* filename);
    
    static void threadInjector(AudioInjector* injector);
    
    static void setInjectorSocket(UDPSocket* injectorSocket) { _injectorSocket = injectorSocket;}
    static void setDestinationSocket(sockaddr& destinationSocket);
private:
    static void* injectAudioViaThread(void* args);
    
    static UDPSocket* _injectorSocket;
    static sockaddr _destinationSocket;
    static bool _isDestinationSocketExplicit;
    static AudioInjector* _injectors[MAX_CONCURRENT_INJECTORS];
};

#endif /* defined(__hifi__AudioInjectionManager__) */
