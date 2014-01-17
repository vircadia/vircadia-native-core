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
#include <HTTPManager.h>
#include <NodeList.h>

const int MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS = 1000;

class DomainServer : public QCoreApplication, public HttpRequestHandler {
    Q_OBJECT
public:
    DomainServer(int argc, char* argv[]);
    
    bool handleHTTPRequest(HTTPConnection* connection, const QString& path);
    
    void exit(int retCode = 0);

    static void setDomainServerInstance(DomainServer* domainServer);
    
public slots:
    /// Called by NodeList to inform us that a node has been killed.
    void nodeKilled(SharedNodePointer node);
    
private:    
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
    
    HTTPManager _HTTPManager;
    
    QMutex _assignmentQueueMutex;
    std::deque<Assignment*> _assignmentQueue;
    
    QFile _staticAssignmentFile;
    uchar* _staticAssignmentFileData;
    
    Assignment* _staticAssignments;
    
    const char* _voxelServerConfig;
    const char* _particleServerConfig;
    const char* _metavoxelServerConfig;
    
    bool _hasCompletedRestartHold;
private slots:
    void readAvailableDatagrams();
    void addStaticAssignmentsBackToQueueAfterRestart();
    void cleanup();
};

#endif /* defined(__hifi__DomainServer__) */
