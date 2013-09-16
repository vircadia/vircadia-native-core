//
//  main.cpp
//  Voxel Server
//
//  Created by Stephen Birarda on 03/06/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <VoxelServer.h>

int main(int argc, const char * argv[]) {
    VoxelServer::setArguments(argc, argv[]);
    VoxelServer::run();
}


