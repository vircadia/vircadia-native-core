//
//  VoxelServer.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __voxel_server__VoxelServer__
#define __voxel_server__VoxelServer__

#include <QStringList>
#include <QDateTime>
#include <QtCore/QCoreApplication>

#include <ThreadedAssignment.h>
#include <EnvironmentData.h>

#include "civetweb.h"

#include "NodeWatcher.h"
#include "VoxelPersistThread.h"
#include "VoxelSendThread.h"
#include "VoxelServerConsts.h"
#include "VoxelServerPacketProcessor.h"

/// Handles assignments of type VoxelServer - sending voxels to various clients.
class VoxelServer : public ThreadedAssignment {
    Q_OBJECT
public:                
    VoxelServer(const unsigned char* dataBuffer, int numBytes);
    
    ~VoxelServer();
    
   
    
    /// allows setting of run arguments
    void setArguments(int argc, char** argv);

    bool wantsDebugVoxelSending() const { return _debugVoxelSending; }
    bool wantsDebugVoxelReceiving() const { return _debugVoxelReceiving; }
    bool wantsVerboseDebug() const { return _verboseDebug; }
    bool wantShowAnimationDebug() const { return _shouldShowAnimationDebug; }
    bool wantSendEnvironments() const { return _sendEnvironments; }
    bool wantDumpVoxelsOnMove() const { return _dumpVoxelsOnMove; }
    bool wantDisplayVoxelStats() const { return _displayVoxelStats; }

    VoxelTree& getServerTree() { return _serverTree; }
    JurisdictionMap* getJurisdiction() { return _jurisdiction; }
    
    int getPacketsPerClientPerInterval() const { return _packetsPerClientPerInterval; }
    bool getSendMinimalEnvironment() const { return _sendMinimalEnvironment; }
    EnvironmentData* getEnvironmentData(int i) { return &_environmentData[i]; }
    int getEnvironmentDataCount() const { return sizeof(_environmentData)/sizeof(EnvironmentData); }
    
    static VoxelServer* GetInstance() { return _theInstance; }
    
    bool isInitialLoadComplete() const { return (_voxelPersistThread) ? _voxelPersistThread->isInitialLoadComplete() : true; }
    time_t* getLoadCompleted() { return (_voxelPersistThread) ? _voxelPersistThread->getLoadCompleted() : NULL; }
    uint64_t getLoadElapsedTime() const { return (_voxelPersistThread) ? _voxelPersistThread->getLoadElapsedTime() : 0; }
public slots:
    /// runs the voxel server assignment
    void run();
    
    void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr);
private:
    int _argc;
    const char** _argv;
    char** _parsedArgV;

    char _voxelPersistFilename[MAX_FILENAME_LENGTH];
    int _packetsPerClientPerInterval;
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
    bool _verboseDebug;
    JurisdictionMap* _jurisdiction;
    JurisdictionSender* _jurisdictionSender;
    VoxelServerPacketProcessor* _voxelServerPacketProcessor;
    VoxelPersistThread* _voxelPersistThread;
    EnvironmentData _environmentData[3];
    
    NodeWatcher _nodeWatcher; // used to cleanup AGENT data when agents are killed
    
    void parsePayload();

    void initMongoose(int port);

    static int civetwebRequestHandler(struct mg_connection *connection);
    static VoxelServer* _theInstance;
    
    time_t _started;
    uint64_t _startedUSecs;
};

#endif // __voxel_server__VoxelServer__
