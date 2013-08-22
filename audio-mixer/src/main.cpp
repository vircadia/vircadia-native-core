//
//  main.cpp
//  mixer
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <AudioMixer.h>

bool wantLocalDomain = false;

int main(int argc, const char* argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_AUDIO_MIXER, MIXER_LISTEN_PORT);
    
    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    ::wantLocalDomain = cmdOptionExists(argc, argv,local);
    if (::wantLocalDomain) {
        printf("Local Domain MODE!\n");
        nodeList->setDomainIPToLocalhost();
    }    

    const char* domainIP = getCmdOption(argc, argv, "--domain");
    if (domainIP) {
        NodeList::getInstance()->setDomainHostname(domainIP);
    }
    
    nodeList->startSilentNodeRemovalThread();
    
    return 0;
}
