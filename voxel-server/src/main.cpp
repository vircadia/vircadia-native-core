//
//  main.cpp
//  Voxel Server
//
//  Created by Stephen Birarda on 03/06/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <SharedUtil.h>
#include <VoxelServer.h>
const int VOXEL_LISTEN_PORT = 40106;


int main(int argc, const char * argv[]) {

    // Handle Local Domain testing with the --local command line
    const char* local = "--local";
    bool wantLocalDomain = cmdOptionExists(argc, argv, local);
    const char* domainIP = getCmdOption(argc, argv, "--domain");

    int listenPort = VOXEL_LISTEN_PORT;
    // Check to see if the user passed in a command line option for setting listen port
    const char* PORT_PARAMETER = "--port";
    const char* portParameter = getCmdOption(argc, argv, PORT_PARAMETER);
    if (portParameter) {
        listenPort = atoi(portParameter);
        if (listenPort < 1) {
            listenPort = VOXEL_LISTEN_PORT;
        }
        printf("portParameter=%s listenPort=%d\n", portParameter, listenPort);
    }

    if (wantLocalDomain) {
        VoxelServer::setupDomainAndPort(local, listenPort);
    } else {
        if (domainIP) {
            VoxelServer::setupDomainAndPort(domainIP, listenPort);
        }
    }

    VoxelServer::setArguments(argc, const_cast<char**>(argv));
    
    VoxelServer dummyAssignedVoxelServer(Assignment::CreateCommand);
    dummyAssignedVoxelServer.run();
}


