//  VoxelServer.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __voxel_server__VoxelServer__
#define __voxel_server__VoxelServer__

#include <EnvironmentData.h>

#include "NodeWatcher.h"
#include "VoxelPersistThread.h"
#include "VoxelSendThread.h"
#include "VoxelServerConsts.h"
#include "VoxelServerPacketProcessor.h"

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
    
    bool wantsDebugVoxelSending() const { return _debugVoxelSending; }
    bool wantsDebugVoxelReceiving() const { return _debugVoxelReceiving; }
    bool wantShowAnimationDebug() const { return _shouldShowAnimationDebug; }
    bool wantSendEnvironments() const { return _sendEnvironments; }
    bool wantDumpVoxelsOnMove() const { return _dumpVoxelsOnMove; }
    bool wantDisplayVoxelStats() const { return _displayVoxelStats; }

    VoxelTree& getServerTree() { return _serverTree; }
    JurisdictionMap* getJurisdiction() { return _jurisdiction; }
    
    void lockTree() {  pthread_mutex_lock(&_treeLock); }
    void unlockTree() {  pthread_mutex_unlock(&_treeLock); }
    
    int getPacketsPerClientPerInterval() const { return _PACKETS_PER_CLIENT_PER_INTERVAL; }
    bool getSendMinimalEnvironment() const { return _sendMinimalEnvironment; }
    EnvironmentData* getEnvironmentData(int i) { return &_environmentData[i]; }
    int getEnvironmentDataCount() const { return sizeof(_environmentData)/sizeof(EnvironmentData); }
    
private:
    int _argc;
    const char** _argv;
    bool _dontKillOnMissingDomain;

    char _voxelPersistFilename[MAX_FILENAME_LENGTH];
    int _PACKETS_PER_CLIENT_PER_INTERVAL;
    VoxelTree _serverTree; // this IS a reaveraging tree 
    bool _wantVoxelPersist;
    bool _wantLocalDomain;
    bool _debugVoxelSending;
    bool _shouldShowAnimationDebug;
    bool _displayVoxelStats;
    bool _debugVoxelReceiving;
    bool _sendEnvironments;
    bool _sendMinimalEnvironment;
    bool _dumpVoxelsOnMove;
    JurisdictionMap* _jurisdiction;
    JurisdictionSender* _jurisdictionSender;
    VoxelServerPacketProcessor* _voxelServerPacketProcessor;
    VoxelPersistThread* _voxelPersistThread;
    pthread_mutex_t _treeLock;
    EnvironmentData _environmentData[3];
    
    NodeWatcher _nodeWatcher; // used to cleanup AGENT data when agents are killed
    
};



#endif // __voxel_server__VoxelServer__
