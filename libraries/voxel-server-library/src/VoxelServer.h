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

#include <OctreeServer.h>
#include "civetweb.h"


#include "VoxelServerConsts.h"

/// Handles assignments of type VoxelServer - sending voxels to various clients.
class VoxelServer : public OctreeServer {
public:                
    VoxelServer(const unsigned char* dataBuffer, int numBytes);
    ~VoxelServer();

    bool wantSendEnvironments() const { return _sendEnvironments; }
    bool getSendMinimalEnvironment() const { return _sendMinimalEnvironment; }
    EnvironmentData* getEnvironmentData(int i) { return &_environmentData[i]; }
    int getEnvironmentDataCount() const { return sizeof(_environmentData)/sizeof(EnvironmentData); }

    // Subclasses must implement these methods    
    virtual OctreeQueryNode* createOctreeQueryNode(Node* newNode);
    virtual Octree* createTree();
    virtual unsigned char getMyNodeType();
    virtual const char* getMyLoggingServerTargetName();
    virtual const char* getMyDefaultPersistFilename();
    virtual bool hasSpecialPacketToSend();
    virtual int sendSpecialPacket(Node* node);

    // subclass may implement these method
    virtual void beforeRun();

private:
    bool _sendEnvironments;
    bool _sendMinimalEnvironment;
    EnvironmentData _environmentData[3];
    unsigned char _tempOutputBuffer[MAX_PACKET_SIZE];
};

#endif // __voxel_server__VoxelServer__
