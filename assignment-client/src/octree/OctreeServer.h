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

#include <HTTPManager.h>

#include <ThreadedAssignment.h>
#include <EnvironmentData.h>

#include "OctreePersistThread.h"
#include "OctreeSendThread.h"
#include "OctreeServerConsts.h"
#include "OctreeInboundPacketProcessor.h"

const int DEFAULT_PACKETS_PER_INTERVAL = 2000; // some 120,000 packets per second total

/// Handles assignments of type OctreeServer - sending octrees to various clients.
class OctreeServer : public ThreadedAssignment, public HTTPRequestHandler {
    Q_OBJECT
public:
    OctreeServer(const QByteArray& packet);
    ~OctreeServer();

    /// allows setting of run arguments
    void setArguments(int argc, char** argv);

    bool wantsDebugSending() const { return _debugSending; }
    bool wantsDebugReceiving() const { return _debugReceiving; }
    bool wantsVerboseDebug() const { return _verboseDebug; }

    Octree* getOctree() { return _tree; }
    JurisdictionMap* getJurisdiction() { return _jurisdiction; }

    int getPacketsPerClientPerInterval() const { return _packetsPerClientPerInterval; }
    int getCurrentClientCount() const { return _clientCount; }
    void clientConnected() { _clientCount++; }
    void clientDisconnected() { _clientCount--; }

    bool isInitialLoadComplete() const { return (_persistThread) ? _persistThread->isInitialLoadComplete() : true; }
    bool isPersistEnabled() const { return (_persistThread) ? true : false; }
    quint64 getLoadElapsedTime() const { return (_persistThread) ? _persistThread->getLoadElapsedTime() : 0; }

    // Subclasses must implement these methods
    virtual OctreeQueryNode* createOctreeQueryNode() = 0;
    virtual Octree* createTree() = 0;
    virtual unsigned char getMyNodeType() const = 0;
    virtual PacketType getMyQueryMessageType() const = 0;
    virtual const char* getMyServerName() const = 0;
    virtual const char* getMyLoggingServerTargetName() const = 0;
    virtual const char* getMyDefaultPersistFilename() const = 0;

    // subclass may implement these method
    virtual void beforeRun() { };
    virtual bool hasSpecialPacketToSend(const SharedNodePointer& node) { return false; }
    virtual int sendSpecialPacket(const SharedNodePointer& node) { return 0; }

    static void attachQueryNodeToNode(Node* newNode);

    bool handleHTTPRequest(HTTPConnection* connection, const QString& path);
public slots:
    /// runs the voxel server assignment
    void run();
    void readPendingDatagrams();

protected:
    void parsePayload();
    void initHTTPManager(int port);

    int _argc;
    const char** _argv;
    char** _parsedArgV;

    HTTPManager* _httpManager;

    char _persistFilename[MAX_FILENAME_LENGTH];
    int _packetsPerClientPerInterval;
    int _packetsTotalPerInterval;
    Octree* _tree; // this IS a reaveraging tree
    bool _wantPersist;
    bool _debugSending;
    bool _debugReceiving;
    bool _verboseDebug;
    JurisdictionMap* _jurisdiction;
    JurisdictionSender* _jurisdictionSender;
    OctreeInboundPacketProcessor* _octreeInboundPacketProcessor;
    OctreePersistThread* _persistThread;

    static OctreeServer* _instance;

    time_t _started;
    quint64 _startedUSecs;
    
    int _clientCount;
};

#endif // __octree_server__OctreeServer__
