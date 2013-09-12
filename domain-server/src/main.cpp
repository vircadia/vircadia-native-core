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

#include "Assignment.h"
#include "NodeList.h"
#include "NodeTypes.h"
#include "Logging.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"

#include "mongoose.h"

const int DOMAIN_LISTEN_PORT = 40102;
unsigned char packetData[MAX_PACKET_SIZE];

const int NODE_COUNT_STAT_INTERVAL_MSECS = 5000;

unsigned char* addNodeToBroadcastPacket(unsigned char* currentPosition, Node* nodeToAdd) {
    *currentPosition++ = nodeToAdd->getType();
    
    currentPosition += packNodeId(currentPosition, nodeToAdd->getNodeID());
    currentPosition += packSocket(currentPosition, nodeToAdd->getPublicSocket());
    currentPosition += packSocket(currentPosition, nodeToAdd->getLocalSocket());
    
    // return the new unsigned char * for broadcast packet
    return currentPosition;
}

static int mongooseRequestHandler(struct mg_connection *conn) {
    const struct mg_request_info *ri = mg_get_request_info(conn);
    
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

const char ASSIGNMENT_SCRIPT_HOST_LOCATION[] = "web/assignment";

static void mongooseUploadHandler(struct mg_connection *conn, const char *path) {
    // create an assignment for this saved script
    Assignment scriptAssignment(Assignment::CreateCommand, Assignment::AgentType);
    
    QString newPath(ASSIGNMENT_SCRIPT_HOST_LOCATION);
    newPath += "/";
    // append the UUID for this script as the new filename, remove the curly braces
    newPath += scriptAssignment.getGUID().toString().mid(1, scriptAssignment.getGUID().toString().length() - 2);
    
    // rename the saved script to the GUID of the assignment and move it to the script host locaiton
    rename(path, newPath.toStdString().c_str());

    qDebug("Saved a script for assignment at %s\n", newPath.toStdString().c_str());
}

int main(int argc, const char* argv[]) {
    
    qInstallMessageHandler(Logging::verboseMessageHandler);
    
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_DOMAIN, DOMAIN_LISTEN_PORT);

	// If user asks to run in "local" mode then we do NOT replace the IP
	// with the EC2 IP. Otherwise, we will replace the IP like we used to
	// this allows developers to run a local domain without recompiling the
	// domain server
	bool isLocalMode = cmdOptionExists(argc, (const char**) argv, "--local");
	if (isLocalMode) {
		printf("NOTE: Running in local mode!\n");
	} else {
		printf("--------------------------------------------------\n");
		printf("NOTE: Not running in local mode. \n");
		printf("If you're a developer testing a local system, you\n");
		printf("probably want to include --local on command line.\n");
		printf("--------------------------------------------------\n");
	}

    setvbuf(stdout, NULL, _IOLBF, 0);
    
    ssize_t receivedBytes = 0;
    char nodeType = '\0';
    
    unsigned char broadcastPacket[MAX_PACKET_SIZE];
    
    unsigned char* currentBufferPos;
    unsigned char* startPointer;
    
    sockaddr_in nodePublicAddress, nodeLocalAddress;
    nodeLocalAddress.sin_family = AF_INET;
    
    in_addr_t serverLocalAddress = getLocalAddress();
    
    nodeList->startSilentNodeRemovalThread();
    
    timeval lastStatSendTime = {};
    const char ASSIGNMENT_SERVER_OPTION[] = "-a";
    
    // grab the overriden assignment-server hostname from argv, if it exists
    const char* customAssignmentServer = getCmdOption(argc, argv, ASSIGNMENT_SERVER_OPTION);
    if (customAssignmentServer) {
        sockaddr_in customAssignmentSocket = socketForHostnameAndHostOrderPort(customAssignmentServer, ASSIGNMENT_SERVER_PORT);
        nodeList->setAssignmentServerSocket((sockaddr*) &customAssignmentSocket);
    }
    
    // use a map to keep track of iterations of silence for assignment creation requests
    const long long GLOBAL_ASSIGNMENT_REQUEST_INTERVAL_USECS = 1 * 1000 * 1000;
    timeval lastGlobalAssignmentRequest = {};
    
    // setup the assignment queue
    std::deque<Assignment*> assignmentQueue;
    
    // as a domain-server we will always want an audio mixer and avatar mixer
    // setup the create assignments for those
    Assignment audioMixerAssignment(Assignment::CreateCommand,
                                    Assignment::AudioMixerType,
                                    Assignment::LocalLocation);
    
    Assignment avatarMixerAssignment(Assignment::CreateCommand,
                                     Assignment::AvatarMixerType,
                                     Assignment::LocalLocation);
    
    // construct a local socket to send with our created assignments to the global AS
    sockaddr_in localSocket = {};
    localSocket.sin_family = AF_INET;
    localSocket.sin_port = htons(nodeList->getInstance()->getNodeSocket()->getListeningPort());
    localSocket.sin_addr.s_addr = serverLocalAddress;
    
    // setup the mongoose web server
    struct mg_context *ctx;
    struct mg_callbacks callbacks = {};
    
    // list of options. Last element must be NULL.
    const char *options[] = {"listening_ports", "8080",
                             "document_root", "./web", NULL};
    
    callbacks.begin_request = mongooseRequestHandler;
    callbacks.upload = mongooseUploadHandler;
    
    // Start the web server.
    ctx = mg_start(&callbacks, NULL, options);
    
    while (true) {
        
        // check if our audio-mixer or avatar-mixer are dead and we don't have existing assignments in the queue
        // so we can add those assignments back to the front of the queue since they are high-priority
        if (!nodeList->soloNodeOfType(NODE_TYPE_AVATAR_MIXER) &&
            std::find(assignmentQueue.begin(), assignmentQueue.end(), &avatarMixerAssignment) == assignmentQueue.end()) {
            qDebug("Missing an avatar mixer and assignment not in queue. Adding.\n");
            assignmentQueue.push_front(&avatarMixerAssignment);
        }
        
        if (!nodeList->soloNodeOfType(NODE_TYPE_AUDIO_MIXER) &&
            std::find(assignmentQueue.begin(), assignmentQueue.end(), &audioMixerAssignment) == assignmentQueue.end()) {
            qDebug("Missing an audio mixer and assignment not in queue. Adding.\n");
            assignmentQueue.push_front(&audioMixerAssignment);
        }
        
        while (nodeList->getNodeSocket()->receive((sockaddr *)&nodePublicAddress, packetData, &receivedBytes) &&
               packetVersionMatch(packetData)) {
            if (packetData[0] == PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY || packetData[0] == PACKET_TYPE_DOMAIN_LIST_REQUEST) {
                // this is an RFD or domain list request packet, and there is a version match
                std::map<char, Node *> newestSoloNodes;
                
                int numBytesSenderHeader = numBytesForPacketHeader(packetData);
                
                nodeType = *(packetData + numBytesSenderHeader);
                int numBytesSocket = unpackSocket(packetData + numBytesSenderHeader + sizeof(NODE_TYPE),
                                                  (sockaddr*) &nodeLocalAddress);
                
                sockaddr* destinationSocket = (sockaddr*) &nodePublicAddress;
                
                // check the node public address
                // if it matches our local address we're on the same box
                // so hardcode the EC2 public address for now
                if (nodePublicAddress.sin_addr.s_addr == serverLocalAddress) {
                    // If we're not running "local" then we do replace the IP
                    // with 0. This designates to clients that the server is reachable
                    // at the same IP address
                    if (!isLocalMode) {
                        nodePublicAddress.sin_addr.s_addr = 0;
                        destinationSocket = (sockaddr*) &nodeLocalAddress;
                    }
                }
                
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
                    
                    unsigned char* nodeTypesOfInterest = packetData + numBytesSenderHeader + sizeof(NODE_TYPE)
                    + numBytesSocket + sizeof(unsigned char);
                    int numInterestTypes = *(nodeTypesOfInterest - 1);
                    
                    if (numInterestTypes > 0) {
                        // if the node has sent no types of interest, assume they want nothing but their own ID back
                        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                            if (!node->matches((sockaddr*) &nodePublicAddress, (sockaddr*) &nodeLocalAddress, nodeType) &&
                                memchr(nodeTypesOfInterest, node->getType(), numInterestTypes)) {
                                // this is not the node themselves
                                // and this is an node of a type in the passed node types of interest
                                // or the node did not pass us any specific types they are interested in
                                
                                if (memchr(SOLO_NODE_TYPES, node->getType(), sizeof(SOLO_NODE_TYPES)) == NULL) {
                                    // this is an node of which there can be multiple, just add them to the packet
                                    // don't send avatar nodes to other avatars, that will come from avatar mixer
                                    if (nodeType != NODE_TYPE_AGENT || node->getType() != NODE_TYPE_AGENT) {
                                        currentBufferPos = addNodeToBroadcastPacket(currentBufferPos, &(*node));
                                    }
                                    
                                } else {
                                    // solo node, we need to only send newest
                                    if (newestSoloNodes[node->getType()] == NULL ||
                                        newestSoloNodes[node->getType()]->getWakeMicrostamp() < node->getWakeMicrostamp()) {
                                        // we have to set the newer solo node to add it to the broadcast later
                                        newestSoloNodes[node->getType()] = &(*node);
                                    }
                                }
                            }
                        }
                        
                        for (std::map<char, Node *>::iterator soloNode = newestSoloNodes.begin();
                             soloNode != newestSoloNodes.end();
                             soloNode++) {
                            // this is the newest alive solo node, add them to the packet
                            currentBufferPos = addNodeToBroadcastPacket(currentBufferPos, soloNode->second);
                        }
                    }
                    
                    // update last receive to now
                    uint64_t timeNow = usecTimestampNow();
                    newNode->setLastHeardMicrostamp(timeNow);
                    
                    if (packetData[0] == PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY
                        && memchr(SOLO_NODE_TYPES, nodeType, sizeof(SOLO_NODE_TYPES))) {
                        newNode->setWakeMicrostamp(timeNow);
                    }
                    
                    // add the node ID to the end of the pointer
                    currentBufferPos += packNodeId(currentBufferPos, newNode->getNodeID());
                    
                    // send the constructed list back to this node
                    nodeList->getNodeSocket()->send(destinationSocket,
                                                    broadcastPacket,
                                                    (currentBufferPos - startPointer) + numHeaderBytes);
                }
            } else if (packetData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
                
                qDebug("Received a request for assignment.\n");
                
                // this is an unassigned client talking to us directly for an assignment
                // go through our queue and see if there are any assignments to give out
                std::deque<Assignment*>::iterator assignment = assignmentQueue.begin();
                
                while (assignment != assignmentQueue.end()) {
                    
                    // give this assignment out, no conditions stop us from giving it to the local assignment client
                    int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_CREATE_ASSIGNMENT);
                    int numAssignmentBytes = (*assignment)->packToBuffer(broadcastPacket + numHeaderBytes);
                    
                    nodeList->getNodeSocket()->send((sockaddr*) &nodePublicAddress,
                                                    broadcastPacket,
                                                    numHeaderBytes + numAssignmentBytes);
                    
                    // remove the assignment from the queue
                    assignmentQueue.erase(assignment);
                    
                    // stop looping, we've handed out an assignment
                    break;
                }
            }
        }
        
        // if ASSIGNMENT_REQUEST_INTERVAL_USECS have passed since last global assignment request then fire off another
        if (usecTimestampNow() - usecTimestamp(&lastGlobalAssignmentRequest) >= GLOBAL_ASSIGNMENT_REQUEST_INTERVAL_USECS) {
            gettimeofday(&lastGlobalAssignmentRequest, NULL);
            
            // go through our queue and see if there are any assignments to send to the global assignment server
            std::deque<Assignment*>::iterator assignment = assignmentQueue.begin();
            
            while (assignment != assignmentQueue.end()) {
                
                if ((*assignment)->getLocation() != Assignment::LocalLocation) {                    
                    // attach our local socket to the assignment so the assignment-server can optionally hand it out
                    (*assignment)->setAttachedLocalSocket((sockaddr*) &localSocket);
                    
                    nodeList->sendAssignment(*(*assignment));
                    
                    // remove the assignment from the queue
                    assignmentQueue.erase(assignment);
                    
                    // stop looping, we've handed out an assignment
                    break;
                } else {
                    // push forward the iterator to check the next assignment
                    assignment++;
                }
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

