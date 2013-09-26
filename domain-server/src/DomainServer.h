//
//  DomainServer.h
//  hifi
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DomainServer__
#define __hifi__DomainServer__

#include <array>
#include <deque>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QMutex>

#include <Assignment.h>
#include <NodeList.h>

#include <civetweb.h>

const int MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS = 1000;

class DomainServer : public NodeListHook {
public:
    DomainServer(int argc, char* argv[]);
    
    int run();
    
    static void signalHandler(int signal);
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
    int checkInMatchesStaticAssignment(NODE_TYPE nodeType, const uchar* checkInUUID);
    Assignment* deployableAssignmentForRequest(Assignment& requestAssignment);
    
    void cleanup();
   
    
    unsigned char* addNodeToBroadcastPacket(unsigned char* currentPosition, Node* nodeToAdd);
    
    QMutex _assignmentQueueMutex;
    std::deque<Assignment*> _assignmentQueue;
    
    QFile _staticAssignmentFile;
    uchar* _staticAssignmentFileData;
    
    uint16_t* _numAssignmentsInStaticFile;
    std::array<Assignment, MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS>* _staticFileAssignments;
    
    const char* _voxelServerConfig;
};

#endif /* defined(__hifi__DomainServer__) */
