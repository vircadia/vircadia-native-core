//
//  main.cpp
//  Domain Server 
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//
//  The Domain Server keeps a list of nodes that have connected to it, and echoes that list of
//  nodes out to nodes when they check in.
//
//  The connection is stateless... the domain server will set you inactive if it does not hear from
//  you in LOGOFF_CHECK_INTERVAL milliseconds, meaning your info will not be sent to other users.
//
//  Each packet from an node has as first character the type of server:
//
//  I - Interactive Node
//  M - Audio Mixer
//

#include <arpa/inet.h>
#include <fcntl.h>
#include <deque>
#include <map>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QMap>
#include <QtCore/QMutex>

#include <civetweb.h>

#include "Assignment.h"
#include "NodeList.h"
#include "NodeTypes.h"
#include "Logging.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"

const int DOMAIN_LISTEN_PORT = 40102;
unsigned char packetData[MAX_PACKET_SIZE];

const int NODE_COUNT_STAT_INTERVAL_MSECS = 5000;

QMutex assignmentQueueMutex;
std::deque<Assignment*> assignmentQueue;

unsigned char* addNodeToBroadcastPacket(unsigned char* currentPosition, Node* nodeToAdd) {
    *currentPosition++ = nodeToAdd->getType();
    
    currentPosition += packNodeId(currentPosition, nodeToAdd->getNodeID());
    currentPosition += packSocket(currentPosition, nodeToAdd->getPublicSocket());
    currentPosition += packSocket(currentPosition, nodeToAdd->getLocalSocket());
    
    // return the new unsigned char * for broadcast packet
    return currentPosition;
}

static int mongooseRequestHandler(struct mg_connection *conn) {
    const struct mg_request_info* ri = mg_get_request_info(conn);
    
    if (strcmp(ri->uri, "/assignment") == 0 && strcmp(ri->request_method, "POST") == 0) {
        // return a 200
        mg_printf(conn, "%s", "HTTP/1.0 200 OK\r\n\r\n");
        // upload the file
        mg_upload(conn, "/tmp");
        
        return 1;
    } else {
        // have mongoose process this request from the document_root
        return 0;
    }
}

const char ASSIGNMENT_SCRIPT_HOST_LOCATION[] = "resources/web/assignment";

static void mongooseUploadHandler(struct mg_connection *conn, const char *path) {
    
    // create an assignment for this saved script, for now make it local only
    Assignment *scriptAssignment = new Assignment(Assignment::CreateCommand, Assignment::AgentType, Assignment::LocalLocation);
    
    // check how many instances of this assignment the user wants by checking the ASSIGNMENT-INSTANCES header
    const char ASSIGNMENT_INSTANCES_HTTP_HEADER[] = "ASSIGNMENT-INSTANCES";
    const char *requestInstancesHeader = mg_get_header(conn, ASSIGNMENT_INSTANCES_HTTP_HEADER);
    
    if (requestInstancesHeader) {
        // the user has requested a number of instances greater than 1
        // so set that on the created assignment
        scriptAssignment->setNumberOfInstances(atoi(requestInstancesHeader));
    }
    
    QString newPath(ASSIGNMENT_SCRIPT_HOST_LOCATION);
    newPath += "/";
    // append the UUID for this script as the new filename, remove the curly braces
    newPath += scriptAssignment->getUUIDStringWithoutCurlyBraces();
    
    // rename the saved script to the GUID of the assignment and move it to the script host locaiton
    rename(path, newPath.toStdString().c_str());

    qDebug("Saved a script for assignment at %s\n", newPath.toStdString().c_str());
    
    // add the script assigment to the assignment queue
    // lock the assignment queue mutex since we're operating on a different thread than DS main
    ::assignmentQueueMutex.lock();
    ::assignmentQueue.push_back(scriptAssignment);
    ::assignmentQueueMutex.unlock();
}

int main(int argc, const char* argv[]) {
    
    QCoreApplication domainServer(argc, (char**) argv);
    
    qInstallMessageHandler(Logging::verboseMessageHandler);
    
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_DOMAIN, DOMAIN_LISTEN_PORT);

    setvbuf(stdout, NULL, _IOLBF, 0);
    
    ssize_t receivedBytes = 0;
    char nodeType = '\0';
    
    unsigned char broadcastPacket[MAX_PACKET_SIZE];
    
    unsigned char* currentBufferPos;
    unsigned char* startPointer;
    
    sockaddr_in nodePublicAddress, nodeLocalAddress, replyDestinationSocket;
    nodeLocalAddress.sin_family = AF_INET;
    
    in_addr_t serverLocalAddress = getLocalAddress();
    
    nodeList->startSilentNodeRemovalThread();

    timeval lastStatSendTime = {};
    
    // as a domain-server we will always want an audio mixer and avatar mixer
    // setup the create assignments for those
    Assignment audioMixerAssignment(Assignment::CreateCommand,
                                    Assignment::AudioMixerType,
                                    Assignment::LocalLocation);
    
    Assignment avatarMixerAssignment(Assignment::CreateCommand,
                                     Assignment::AvatarMixerType,
                                     Assignment::LocalLocation);
    
    Assignment voxelServerAssignment(Assignment::CreateCommand,
                                     Assignment::VoxelServerType,
                                     Assignment::LocalLocation);

    // Handle Domain/Voxel Server configuration command line arguments
    const char VOXEL_CONFIG_OPTION[] = "--voxelServerConfig";
    const char* voxelServerConfig = getCmdOption(argc, argv, VOXEL_CONFIG_OPTION);
    if (voxelServerConfig) {
        qDebug("Reading Voxel Server Configuration.\n");
        qDebug() << "   config: " << voxelServerConfig << "\n";
        voxelServerAssignment.setPayload((uchar*)voxelServerConfig, strlen(voxelServerConfig) + 1);
    }
    
    // construct a local socket to send with our created assignments to the global AS
    sockaddr_in localSocket = {};
    localSocket.sin_family = AF_INET;
    localSocket.sin_port = htons(nodeList->getInstance()->getNodeSocket()->getListeningPort());
    localSocket.sin_addr.s_addr = serverLocalAddress;
    
    // setup the mongoose web server
    struct mg_context* ctx;
    struct mg_callbacks callbacks = {};
    
    QString documentRoot = QString("%1/resources/web").arg(QCoreApplication::applicationDirPath());
    
    // list of options. Last element must be NULL.
    const char* options[] = {"listening_ports", "8080",
                             "document_root", documentRoot.toStdString().c_str(), NULL};
    
    callbacks.begin_request = mongooseRequestHandler;
    callbacks.upload = mongooseUploadHandler;
    
    // Start the web server.
    ctx = mg_start(&callbacks, NULL, options);
    
    // wait to check on voxel-servers till we've given our NodeList a chance to get a good list
    int checkForVoxelServerAttempt = 0;
    
    while (true) {
        
        ::assignmentQueueMutex.lock();
        // check if our audio-mixer or avatar-mixer are dead and we don't have existing assignments in the queue
        // so we can add those assignments back to the front of the queue since they are high-priority
        if (!nodeList->soloNodeOfType(NODE_TYPE_AVATAR_MIXER) &&
            std::find(::assignmentQueue.begin(), assignmentQueue.end(), &avatarMixerAssignment) == ::assignmentQueue.end()) {
            qDebug("Missing an avatar mixer and assignment not in queue. Adding.\n");
            ::assignmentQueue.push_front(&avatarMixerAssignment);
        }
        
        if (!nodeList->soloNodeOfType(NODE_TYPE_AUDIO_MIXER) &&
            std::find(::assignmentQueue.begin(), ::assignmentQueue.end(), &audioMixerAssignment) == ::assignmentQueue.end()) {
            qDebug("Missing an audio mixer and assignment not in queue. Adding.\n");
            ::assignmentQueue.push_front(&audioMixerAssignment);
        }

        // Now handle voxel servers. Couple of things are special about voxel servers. 
        // 1) They can run standalone, and so we want to wait to ask for an assignment until we've given them sufficient
        //    time to check in with us. So we will look for them, but we want actually add assignments unless we haven't 
        //    seen one after a few tries.
        // 2) They aren't soloNodeOfType() so we have to count them.
        int voxelServerCount = 0;
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
                voxelServerCount++;
            }
        }
        const int MIN_VOXEL_SERVER_CHECKS = 10;
        if (checkForVoxelServerAttempt > MIN_VOXEL_SERVER_CHECKS && voxelServerCount == 0 &&
            std::find(::assignmentQueue.begin(), ::assignmentQueue.end(), &voxelServerAssignment) == ::assignmentQueue.end()) {
            qDebug("Missing a voxel server and assignment not in queue. Adding.\n");
            ::assignmentQueue.push_front(&voxelServerAssignment);
        }
        checkForVoxelServerAttempt++;

        ::assignmentQueueMutex.unlock();
        
        while (nodeList->getNodeSocket()->receive((sockaddr *)&nodePublicAddress, packetData, &receivedBytes) &&
               packetVersionMatch(packetData)) {
            if (packetData[0] == PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY || packetData[0] == PACKET_TYPE_DOMAIN_LIST_REQUEST) {
                // this is an RFD or domain list request packet, and there is a version match
                std::map<char, Node *> newestSoloNodes;
                
                int numBytesSenderHeader = numBytesForPacketHeader(packetData);
                
                nodeType = *(packetData + numBytesSenderHeader);
                int numBytesSocket = unpackSocket(packetData + numBytesSenderHeader + sizeof(NODE_TYPE),
                                                  (sockaddr*) &nodeLocalAddress);
                
                replyDestinationSocket = nodePublicAddress;
                
                // check the node public address
                // if it matches our local address
                // or if it's the loopback address we're on the same box
                if (nodePublicAddress.sin_addr.s_addr == serverLocalAddress ||
                    nodePublicAddress.sin_addr.s_addr == htonl(INADDR_LOOPBACK)) {
                    
                    nodePublicAddress.sin_addr.s_addr = 0;
                }
                
                bool matchedUUID = true;
                
                if ((nodeType == NODE_TYPE_AVATAR_MIXER || nodeType == NODE_TYPE_AUDIO_MIXER) &&
                    !nodeList->soloNodeOfType(nodeType)) {
                    // if this is an audio-mixer or an avatar-mixer and we don't have one yet
                    // we need to check the GUID of the assignment in the queue
                    // (if it exists) to make sure there is a match
                    
                    // reset matchedUUID to false so there is no match by default
                    matchedUUID = false;
                    
                    // pull the UUID passed with the check in
                    QUuid checkInUUID = QUuid::fromRfc4122(QByteArray((const char*) packetData + numBytesSenderHeader +
                                                                      sizeof(NODE_TYPE),
                                                                      NUM_BYTES_RFC4122_UUID));
                    
                    // lock the assignment queue
                    ::assignmentQueueMutex.lock();
                    
                    std::deque<Assignment*>::iterator assignment = ::assignmentQueue.begin();
                    
                    Assignment::Type matchType = nodeType == NODE_TYPE_AUDIO_MIXER
                        ? Assignment::AudioMixerType : Assignment::AvatarMixerType;
                    
                    
                    // enumerate the assignments and see if there is a type and UUID match
                    while (assignment != ::assignmentQueue.end()) {
                        
                        if ((*assignment)->getType() == matchType
                            && (*assignment)->getUUID() == checkInUUID) {
                            // type and UUID match
                            matchedUUID = true;
                            
                            // remove this assignment from the queue
                            ::assignmentQueue.erase(assignment);
                            
                            break;
                        } else {
                            // no match, keep looking
                            assignment++;
                        }
                    }
                    
                    // unlock the assignment queue
                    ::assignmentQueueMutex.unlock();
                }
                
                if (matchedUUID) {
                    Node* newNode = nodeList->addOrUpdateNode((sockaddr*) &nodePublicAddress,
                                                              (sockaddr*) &nodeLocalAddress,
                                                              nodeType,
                                                              nodeList->getLastNodeID());
                    
                    // if addOrUpdateNode returns NULL this was a solo node we already have, don't talk back to it
                    if (newNode) {
                        if (newNode->getNodeID() == nodeList->getLastNodeID()) {
                            nodeList->increaseNodeID();
                        }
                        
                        int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_DOMAIN);
                        
                        currentBufferPos = broadcastPacket + numHeaderBytes;
                        startPointer = currentBufferPos;
                        
                        int numBytesUUID = (nodeType == NODE_TYPE_AUDIO_MIXER || nodeType == NODE_TYPE_AVATAR_MIXER)
                            ? NUM_BYTES_RFC4122_UUID
                            : 0;
                        
                        unsigned char* nodeTypesOfInterest = packetData + numBytesSenderHeader + numBytesUUID +
                            sizeof(NODE_TYPE) + numBytesSocket + sizeof(unsigned char);
                        int numInterestTypes = *(nodeTypesOfInterest - 1);
                        
                        if (numInterestTypes > 0) {
                            // if the node has sent no types of interest, assume they want nothing but their own ID back
                            for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                                if (!node->matches((sockaddr*) &nodePublicAddress, (sockaddr*) &nodeLocalAddress, nodeType) &&
                                    memchr(nodeTypesOfInterest, node->getType(), numInterestTypes)) {
                
                                    // don't send avatar nodes to other avatars, that will come from avatar mixer
                                    if (nodeType != NODE_TYPE_AGENT || node->getType() != NODE_TYPE_AGENT) {
                                        currentBufferPos = addNodeToBroadcastPacket(currentBufferPos, &(*node));
                                    }
                                    
                                }
                            }
                            
                            
                        }
                        
                        // update last receive to now
                        uint64_t timeNow = usecTimestampNow();
                        newNode->setLastHeardMicrostamp(timeNow);
                        
                        // add the node ID to the end of the pointer
                        currentBufferPos += packNodeId(currentBufferPos, newNode->getNodeID());
                        
                        // send the constructed list back to this node
                        nodeList->getNodeSocket()->send((sockaddr*)&replyDestinationSocket,
                                                        broadcastPacket,
                                                        (currentBufferPos - startPointer) + numHeaderBytes);
                    }
                }
            } else if (packetData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
                
                qDebug("Received a request for assignment.\n");
                
                ::assignmentQueueMutex.lock();
                
                // this is an unassigned client talking to us directly for an assignment
                // go through our queue and see if there are any assignments to give out
                std::deque<Assignment*>::iterator assignment = ::assignmentQueue.begin();
                
                while (assignment != ::assignmentQueue.end()) {
                    // construct the requested assignment from the packet data
                    Assignment requestAssignment(packetData, receivedBytes);
                    
                    if (requestAssignment.getType() == Assignment::AllTypes ||
                        (*assignment)->getType() == requestAssignment.getType()) {
                        // attach our local socket to the assignment
                        (*assignment)->setAttachedLocalSocket((sockaddr*) &localSocket);
                        
                        // give this assignment out, either the type matches or the requestor said they will take any
                        int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_CREATE_ASSIGNMENT);
                        int numAssignmentBytes = (*assignment)->packToBuffer(broadcastPacket + numHeaderBytes);
                        
                        nodeList->getNodeSocket()->send((sockaddr*) &nodePublicAddress,
                                                        broadcastPacket,
                                                        numHeaderBytes + numAssignmentBytes);
                        
                        if ((*assignment)->getType() == Assignment::AgentType) {
                            // if this is a script assignment we need to delete it to avoid a memory leak
                            // or if there is more than one instance to send out, simpy decrease the number of instances
                            if ((*assignment)->getNumberOfInstances() > 1) {
                                (*assignment)->decrementNumberOfInstances();
                            } else {
                                ::assignmentQueue.erase(assignment);
                                delete *assignment;
                            }
                        } else {
                            // remove the assignment from the queue
                            ::assignmentQueue.erase(assignment);
                            
                            if ((*assignment)->getType() != Assignment::VoxelServerType) {
                                // keep audio-mixer and avatar-mixer assignments in the queue
                                // until we get a check-in from that GUID
                                // but stick it at the back so the others have a chance to go out
                                ::assignmentQueue.push_back(*assignment);
                            }
                        }
                        
                        // stop looping, we've handed out an assignment
                        break;
                    } else {
                        // push forward the iterator to check the next assignment
                        assignment++;
                    }
                }
                
                ::assignmentQueueMutex.unlock();
            } else if (packetData[0] == PACKET_TYPE_CREATE_ASSIGNMENT) {
                // this is a create assignment likely recieved from a server needed more clients to help with load
                
                // unpack it
                Assignment* createAssignment = new Assignment(packetData, receivedBytes);
                
                qDebug() << "Received a create assignment -" << *createAssignment << "\n";
                
                // add the assignment at the back of the queue
                ::assignmentQueueMutex.lock();
                ::assignmentQueue.push_back(createAssignment);
                ::assignmentQueueMutex.unlock();
            }
        }
        
        if (Logging::shouldSendStats()) {
            if (usecTimestampNow() - usecTimestamp(&lastStatSendTime) >= (NODE_COUNT_STAT_INTERVAL_MSECS * 1000)) {
                // time to send our count of nodes and servers to logstash
                const char NODE_COUNT_LOGSTASH_KEY[] = "ds-node-count";
                
                Logging::stashValue(STAT_TYPE_TIMER, NODE_COUNT_LOGSTASH_KEY, nodeList->getNumAliveNodes());
                
                gettimeofday(&lastStatSendTime, NULL);
            }
        }
    }

    return 0;
}

