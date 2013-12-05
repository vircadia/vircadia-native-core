//
//  OctreeServer.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __octree_server__OctreeServer__
#define __octree_server__OctreeServer__

#include <QStringList>
#include <QDateTime>
#include <QtCore/QCoreApplication>

#include <ThreadedAssignment.h>
#include <EnvironmentData.h>

#include "OctreePersistThread.h"
#include "OctreeSendThread.h"
#include "OctreeServerConsts.h"
#include "OctreeInboundPacketProcessor.h"

/// Handles assignments of type OctreeServer - sending octrees to various clients.
class OctreeServer : public ThreadedAssignment, public NodeListHook {
public:                
    OctreeServer(const unsigned char* dataBuffer, int numBytes);
    ~OctreeServer();
    
    /// allows setting of run arguments
    void setArguments(int argc, char** argv);

    bool wantsDebugSending() const { return _debugSending; }
    bool wantsDebugReceiving() const { return _debugReceiving; }
    bool wantsVerboseDebug() const { return _verboseDebug; }

    Octree* getOctree() { return _tree; }
    JurisdictionMap* getJurisdiction() { return _jurisdiction; }
    
    int getPacketsPerClientPerInterval() const { return _packetsPerClientPerInterval; }
    static OctreeServer* GetInstance() { return _theInstance; }
    
    bool isInitialLoadComplete() const { return (_persistThread) ? _persistThread->isInitialLoadComplete() : true; }
    time_t* getLoadCompleted() { return (_persistThread) ? _persistThread->getLoadCompleted() : NULL; }
    uint64_t getLoadElapsedTime() const { return (_persistThread) ? _persistThread->getLoadElapsedTime() : 0; }

    // Subclasses must implement these methods    
    virtual OctreeQueryNode* createOctreeQueryNode(Node* newNode) = 0;
    virtual Octree* createTree() = 0;
    virtual unsigned char getMyNodeType() const = 0;
    virtual PACKET_TYPE getMyQueryMessageType() const = 0;
    virtual const char* getMyServerName() const = 0;
    virtual const char* getMyLoggingServerTargetName() const = 0;
    virtual const char* getMyDefaultPersistFilename() const = 0;
    
    // subclass may implement these method
    virtual void beforeRun() { };
    virtual bool hasSpecialPacketToSend() { return false; }
    virtual int sendSpecialPacket(Node* node) { return 0; }

    static void attachQueryNodeToNode(Node* newNode);

    // NodeListHook 
    virtual void nodeAdded(Node* node);
    virtual void nodeKilled(Node* node);

public slots:
    /// runs the voxel server assignment
    void run();
    void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr);

protected:
    int _argc;
    const char** _argv;
    char** _parsedArgV;

    char _persistFilename[MAX_FILENAME_LENGTH];
    int _packetsPerClientPerInterval;
    Octree* _tree; // this IS a reaveraging tree 
    bool _wantPersist;
    bool _debugSending;
    bool _debugReceiving;
    bool _verboseDebug;
    JurisdictionMap* _jurisdiction;
    JurisdictionSender* _jurisdictionSender;
    OctreeInboundPacketProcessor* _octreeInboundPacketProcessor;
    OctreePersistThread* _persistThread;

    void parsePayload();
    void initMongoose(int port);
    static int civetwebRequestHandler(struct mg_connection *connection);
    static OctreeServer* _theInstance;
    time_t _started;
    uint64_t _startedUSecs;
};

#endif // __octree_server__OctreeServer__
