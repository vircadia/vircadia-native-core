//
//  DomainServer.h
//  hifi
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DomainServer__
#define __hifi__DomainServer__

#include <deque>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QMutex>

#include <Assignment.h>
#include <NodeList.h>

#include "civetweb.h"

const int MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS = 1000;

class DomainServer : public QCoreApplication, public NodeListHook {
    Q_OBJECT
public:
    DomainServer(int argc, char* argv[]);
    
    void exit(int retCode = 0);

    static void setDomainServerInstance(DomainServer* domainServer);
    
    /// Called by NodeList to inform us that a node has been added.
    void nodeAdded(Node* node);
    /// Called by NodeList to inform us that a node has been killed.
    void nodeKilled(Node* node);
private:    
    static int civetwebRequestHandler(struct mg_connection *connection);
    static void civetwebUploadHandler(struct mg_connection *connection, const char *path);
    
    static DomainServer* domainServerInstance;
    
    void prepopulateStaticAssignmentFile();
    Assignment* matchingStaticAssignmentForCheckIn(const QUuid& checkInUUID, NODE_TYPE nodeType);
    Assignment* deployableAssignmentForRequest(Assignment& requestAssignment);
    void removeAssignmentFromQueue(Assignment* removableAssignment);
    bool checkInWithUUIDMatchesExistingNode(const HifiSockAddr& nodePublicSocket,
                                            const HifiSockAddr& nodeLocalSocket,
                                            const QUuid& checkInUUI);
    void addReleasedAssignmentBackToQueue(Assignment* releasedAssignment);
    
    unsigned char* addNodeToBroadcastPacket(unsigned char* currentPosition, Node* nodeToAdd);
    
    QMutex _assignmentQueueMutex;
    std::deque<Assignment*> _assignmentQueue;
    
    QFile _staticAssignmentFile;
    uchar* _staticAssignmentFileData;
    
    Assignment* _staticAssignments;
    
    const char* _voxelServerConfig;
    const char* _particleServerConfig;
    
    bool _hasCompletedRestartHold;
private slots:
    void readAvailableDatagrams();
    void addStaticAssignmentsBackToQueueAfterRestart();
    void cleanup();
};

#endif /* defined(__hifi__DomainServer__) */
