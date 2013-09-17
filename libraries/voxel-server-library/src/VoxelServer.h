//  VoxelServer.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __voxel_server__VoxelServer__
#define __voxel_server__VoxelServer__

/// Handles assignments of type VoxelServer - sending voxels to various clients.
class VoxelServer {
public:
    VoxelServer();
    
    /// runs the voxel server assignment
    void run(const char* configuration = NULL);
    
    /// allows setting of run arguments
    void setArguments(int argc, char** argv);

    /// when VoxelServer class is used by voxel-server stand alone executable it calls this to specify the domain
    /// and port it is handling. When called by assignment-client, this is not needed because assignment-client
    /// handles ports and domains automatically.
    /// \param const char* domain domain name, IP address, or local to specify the domain the voxel server is serving
    /// \param int port port the voxel server will listen on
    void setupStandAlone(const char* domain, int port);
    
private:
    int _argc;
    const char** _argv;
    bool _dontKillOnMissingDomain;
};



#endif // __voxel_server__VoxelServer__
