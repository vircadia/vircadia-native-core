//
//  DomainServer.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <signal.h>

#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "DomainServer.h"

DomainServer* DomainServer::domainServerInstance = NULL;

void DomainServer::signalHandler(int signal) {
    domainServerInstance->cleanup();
}

void DomainServer::setDomainServerInstance(DomainServer* domainServer) {
    domainServerInstance = domainServer;
}

int DomainServer::civetwebRequestHandler(struct mg_connection *connection) {
    const struct mg_request_info* ri = mg_get_request_info(connection);
    
    if (strcmp(ri->uri, "/assignment") == 0 && strcmp(ri->request_method, "POST") == 0) {
        // return a 200
        mg_printf(connection, "%s", "HTTP/1.0 200 OK\r\n\r\n");
        // upload the file
        mg_upload(connection, "/tmp");
        
        return 1;
    } else {
        // have mongoose process this request from the document_root
        return 0;
    }
}

const char ASSIGNMENT_SCRIPT_HOST_LOCATION[] = "resources/web/assignment";

void DomainServer::civetwebUploadHandler(struct mg_connection *connection, const char *path) {
    
    // create an assignment for this saved script, for now make it local only
    Assignment *scriptAssignment = new Assignment(Assignment::CreateCommand, Assignment::AgentType, Assignment::LocalLocation);
    
    // check how many instances of this assignment the user wants by checking the ASSIGNMENT-INSTANCES header
    const char ASSIGNMENT_INSTANCES_HTTP_HEADER[] = "ASSIGNMENT-INSTANCES";
    const char *requestInstancesHeader = mg_get_header(connection, ASSIGNMENT_INSTANCES_HTTP_HEADER);
    
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
    rename(path, newPath.toLocal8Bit().constData());
    
    qDebug("Saved a script for assignment at %s\n", newPath.toLocal8Bit().constData());
    
    // add the script assigment to the assignment queue
    // lock the assignment queue mutex since we're operating on a different thread than DS main
    domainServerInstance->_assignmentQueueMutex.lock();
    domainServerInstance->_assignmentQueue.push_back(scriptAssignment);
    domainServerInstance->_assignmentQueueMutex.unlock();
}

void DomainServer::nodeAdded(Node* node) {
    NodeList::getInstance()->increaseNodeID();
}

void DomainServer::nodeKilled(Node* node) {
    
}

unsigned char* DomainServer::addNodeToBroadcastPacket(unsigned char* currentPosition, Node* nodeToAdd) {
    *currentPosition++ = nodeToAdd->getType();
    
    currentPosition += packNodeId(currentPosition, nodeToAdd->getNodeID());
    currentPosition += packSocket(currentPosition, nodeToAdd->getPublicSocket());
    currentPosition += packSocket(currentPosition, nodeToAdd->getLocalSocket());
    
    // return the new unsigned char * for broadcast packet
    return currentPosition;
}

DomainServer::DomainServer(int argc, char* argv[]) :
    _assignmentQueueMutex(),
    _assignmentQueue(),
    _staticAssignmentFile(QString("%1/config.ds").arg(QCoreApplication::applicationDirPath())),
    _staticAssignmentFileData(NULL),
    _voxelServerConfig(NULL)
{
    DomainServer::setDomainServerInstance(this);
        
    const char CUSTOM_PORT_OPTION[] = "-p";
    const char* customPortString = getCmdOption(argc, (const char**) argv, CUSTOM_PORT_OPTION);
    unsigned short domainServerPort = customPortString ? atoi(customPortString) : DEFAULT_DOMAIN_SERVER_PORT;
    
    NodeList::createInstance(NODE_TYPE_DOMAIN, domainServerPort);
    
    struct sigaction sigIntHandler;
    
    sigIntHandler.sa_handler = DomainServer::signalHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    
    sigaction(SIGINT, &sigIntHandler, NULL);
        
    const char VOXEL_CONFIG_OPTION[] = "--voxelServerConfig";
    _voxelServerConfig = getCmdOption(argc, (const char**) argv, VOXEL_CONFIG_OPTION);
    
    // setup the mongoose web server
    struct mg_callbacks callbacks = {};
    
    QString documentRoot = QString("%1/resources/web").arg(QCoreApplication::applicationDirPath());
    
    // list of options. Last element must be NULL.
    const char* options[] = {"listening_ports", "8080",
        "document_root", documentRoot.toLocal8Bit().constData(), NULL};
    
    callbacks.begin_request = civetwebRequestHandler;
    callbacks.upload = civetwebUploadHandler;
    
    // Start the web server.
    mg_start(&callbacks, NULL, options);
}

void DomainServer::prepopulateStaticAssignmentFile() {
    const uint NUM_FRESH_STATIC_ASSIGNMENTS = 3;
    
    // write a fresh static assignment array to file
    
    std::array<Assignment, MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS> freshStaticAssignments;
    
    // pre-populate the first static assignment list with assignments for root AuM, AvM, VS
    freshStaticAssignments[0] = Assignment(Assignment::CreateCommand,
                                           Assignment::AudioMixerType,
                                           Assignment::LocalLocation);
    freshStaticAssignments[1] = Assignment(Assignment::CreateCommand,
                                           Assignment::AvatarMixerType,
                                           Assignment::LocalLocation);
    
    Assignment voxelServerAssignment(Assignment::CreateCommand, Assignment::VoxelServerType, Assignment::LocalLocation);
    
    // Handle Domain/Voxel Server configuration command line arguments
    if (_voxelServerConfig) {
        qDebug("Reading Voxel Server Configuration.\n");
        qDebug() << "   config: " << _voxelServerConfig << "\n";
        int payloadLength = strlen(_voxelServerConfig) + sizeof(char);
        voxelServerAssignment.setPayload((const uchar*)_voxelServerConfig, payloadLength);
    }
    
    freshStaticAssignments[2] = voxelServerAssignment;
    
    _staticAssignmentFile.open(QIODevice::WriteOnly);
    
    _staticAssignmentFile.write((char*) &NUM_FRESH_STATIC_ASSIGNMENTS,
                                sizeof(uint16_t));
    _staticAssignmentFile.write((char*) &freshStaticAssignments, sizeof(freshStaticAssignments));
    _staticAssignmentFile.resize(MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS * sizeof(Assignment));
    _staticAssignmentFile.close();
}

int DomainServer::checkInMatchesStaticAssignment(NODE_TYPE nodeType, const uchar* checkInData) {
    // pull the UUID passed with the check in
    QUuid checkInUUID = QUuid::fromRfc4122(QByteArray((const char*) checkInData + numBytesForPacketHeader(checkInData) +
                                                      sizeof(NODE_TYPE),
                                                      NUM_BYTES_RFC4122_UUID));
    int staticAssignmentIndex = 0;
    
    while (staticAssignmentIndex < _staticFileAssignments->size() - 1
           && !(*_staticFileAssignments)[staticAssignmentIndex].getUUID().isNull()) {
        Assignment* staticAssignment = &(*_staticFileAssignments)[staticAssignmentIndex];
        
        if (staticAssignment->getType() == Assignment::typeForNodeType(nodeType)
            && staticAssignment->getUUID() == checkInUUID) {
            // return index of match
            return staticAssignmentIndex;
        } else {
            // no match, keep looking
            staticAssignmentIndex++;
        }
    }
    
    // return -1 for no match
    return -1;
    
}

Assignment* DomainServer::deployableAssignmentForRequest(Assignment& requestAssignment) {
    _assignmentQueueMutex.lock();
    
    // this is an unassigned client talking to us directly for an assignment
    // go through our queue and see if there are any assignments to give out
    std::deque<Assignment*>::iterator assignment = _assignmentQueue.begin();
    
    while (assignment != _assignmentQueue.end()) {
        
        if (requestAssignment.getType() == Assignment::AllTypes ||
            (*assignment)->getType() == requestAssignment.getType()) {
            
            if ((*assignment)->getType() == Assignment::AgentType) {
                // if this is a script assignment we need to delete it to avoid a memory leak
                // or if there is more than one instance to send out, simpy decrease the number of instances
                if ((*assignment)->getNumberOfInstances() > 1) {
                    (*assignment)->decrementNumberOfInstances();
                } else {
                    _assignmentQueue.erase(assignment);
                    delete *assignment;
                }
            } else {
                Assignment *sentAssignment = *assignment;
                // remove the assignment from the queue
                _assignmentQueue.erase(assignment);
                
                if (sentAssignment->getType() != Assignment::VoxelServerType) {
                    // keep audio-mixer and avatar-mixer assignments in the queue
                    // until we get a check-in from that GUID
                    // but stick it at the back so the others have a chance to go out
                    
                    _assignmentQueue.push_back(sentAssignment);
                }
            }
            
            // stop looping, we've handed out an assignment
            _assignmentQueueMutex.unlock();
            return *assignment;
        } else {
            // push forward the iterator to check the next assignment
            assignment++;
        }
    }
    
    _assignmentQueueMutex.unlock();
    return NULL;
}

void DomainServer::cleanup() {
    _staticAssignmentFile.unmap(_staticAssignmentFileData);
    _staticAssignmentFile.close();
}

int DomainServer::run() {
    NodeList* nodeList = NodeList::getInstance();
    
    nodeList->addHook(this);
    
    ssize_t receivedBytes = 0;
    char nodeType = '\0';
    
    unsigned char broadcastPacket[MAX_PACKET_SIZE];
    unsigned char packetData[MAX_PACKET_SIZE];
    
    unsigned char* currentBufferPos;
    unsigned char* startPointer;
    
    sockaddr_in nodePublicAddress, nodeLocalAddress, replyDestinationSocket;
    nodeLocalAddress.sin_family = AF_INET;
    
    in_addr_t serverLocalAddress = getLocalAddress();
    
    nodeList->startSilentNodeRemovalThread();
    
    if (!_staticAssignmentFile.exists()) {
        prepopulateStaticAssignmentFile();
    }
    
    _staticAssignmentFile.open(QIODevice::ReadWrite);
    
    _staticAssignmentFileData = _staticAssignmentFile.map(0, _staticAssignmentFile.size());
    
    _numAssignmentsInStaticFile = (uint16_t*) _staticAssignmentFileData;
    _staticFileAssignments = (std::array<Assignment, MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS>*)
        (_staticAssignmentFileData + sizeof(*_numAssignmentsInStaticFile));
    
    while (true) {
        while (nodeList->getNodeSocket()->receive((sockaddr *)&nodePublicAddress, packetData, &receivedBytes) &&
               packetVersionMatch(packetData)) {
            if (packetData[0] == PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY || packetData[0] == PACKET_TYPE_DOMAIN_LIST_REQUEST) {
                // this is an RFD or domain list request packet, and there is a version match
                
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
                
                const char STATICALLY_ASSIGNED_NODES[3] = {
                    NODE_TYPE_AUDIO_MIXER,
                    NODE_TYPE_AVATAR_MIXER,
                    NODE_TYPE_VOXEL_SERVER
                };
                
                int matchingStaticAssignmentIndex = -1;
                
                if (memchr(STATICALLY_ASSIGNED_NODES, nodeType, sizeof(STATICALLY_ASSIGNED_NODES)) == NULL ||
                    (matchingStaticAssignmentIndex = checkInMatchesStaticAssignment(nodeType, packetData)) != -1) {
                    
                    Node* checkInNode = nodeList->addOrUpdateNode((sockaddr*) &nodePublicAddress,
                                                              (sockaddr*) &nodeLocalAddress,
                                                              nodeType,
                                                              nodeList->getLastNodeID());
                    
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
                    checkInNode->setLastHeardMicrostamp(timeNow);
                    
                    // add the node ID to the end of the pointer
                    currentBufferPos += packNodeId(currentBufferPos, checkInNode->getNodeID());
                    
                    // send the constructed list back to this node
                    nodeList->getNodeSocket()->send((sockaddr*)&replyDestinationSocket,
                                                    broadcastPacket,
                                                    (currentBufferPos - startPointer) + numHeaderBytes);
                }
            } else if (packetData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
                
                qDebug("Received a request for assignment.\n");
                
                if (_assignmentQueue.size() > 0) {
                    // construct the requested assignment from the packet data
                    Assignment requestAssignment(packetData, receivedBytes);
                    
                    Assignment* assignmentToDeploy = deployableAssignmentForRequest(requestAssignment);
                    
                    if (assignmentToDeploy) {
                        
                        // give this assignment out, either the type matches or the requestor said they will take any
                        int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_CREATE_ASSIGNMENT);
                        int numAssignmentBytes = assignmentToDeploy->packToBuffer(broadcastPacket + numHeaderBytes);
        
                        nodeList->getNodeSocket()->send((sockaddr*) &nodePublicAddress,
                                                        broadcastPacket,
                                                        numHeaderBytes + numAssignmentBytes);
                    }
                    
                }
            } else if (packetData[0] == PACKET_TYPE_CREATE_ASSIGNMENT) {
                // this is a create assignment likely recieved from a server needed more clients to help with load
                
                // unpack it
                Assignment* createAssignment = new Assignment(packetData, receivedBytes);
                
                qDebug() << "Received a create assignment -" << *createAssignment << "\n";
                
                // add the assignment at the back of the queue
                _assignmentQueueMutex.lock();
                _assignmentQueue.push_back(createAssignment);
                _assignmentQueueMutex.unlock();
                
                // find the first available spot in the static assignments and put this assignment there
                for (int i = 0; i < _staticFileAssignments->size() - 1; i++) {
                    if ((*_staticFileAssignments)[i].getUUID().isNull()) {
                        (*_staticFileAssignments)[i] = *createAssignment;
                    }
                }
            }
        }
    }
    
    this->cleanup();
    
    return 0;
}