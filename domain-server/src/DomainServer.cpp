//
//  DomainServer.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <arpa/inet.h>
#include <signal.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStringList>

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "DomainServer.h"

DomainServer* DomainServer::domainServerInstance = NULL;

void DomainServer::signalHandler(int signal) {
    domainServerInstance->cleanup();
    exit(1);
}

void DomainServer::setDomainServerInstance(DomainServer* domainServer) {
    domainServerInstance = domainServer;
}

QJsonObject jsonForSocket(sockaddr* socket) {
    QJsonObject socketJSON;
    
    if (socket->sa_family == AF_INET) {
        sockaddr_in* socketIPv4 = (sockaddr_in*) socket;
        socketJSON["ip"] = QString(inet_ntoa(socketIPv4->sin_addr));
        socketJSON["port"] = (int) ntohs(socketIPv4->sin_port);
    }
    
    return socketJSON;
}

const char JSON_KEY_TYPE[] = "type";
const char JSON_KEY_PUBLIC_SOCKET[] = "public";
const char JSON_KEY_LOCAL_SOCKET[] = "local";
const char JSON_KEY_POOL[] = "pool";

QJsonObject jsonObjectForNode(Node* node) {
    QJsonObject nodeJson;

    // re-format the type name so it matches the target name
    QString nodeTypeName(node->getTypeName());
    nodeTypeName = nodeTypeName.toLower();
    nodeTypeName.replace(' ', '-');
    
    // add the node type
    nodeJson[JSON_KEY_TYPE] = nodeTypeName;
    
    // add the node socket information
    nodeJson[JSON_KEY_PUBLIC_SOCKET] = jsonForSocket(node->getPublicSocket());
    nodeJson[JSON_KEY_LOCAL_SOCKET] = jsonForSocket(node->getLocalSocket());
    
    // if the node has pool information, add it
    if (node->getLinkedData() && ((Assignment*) node->getLinkedData())->hasPool()) {
        nodeJson[JSON_KEY_POOL] = QString(((Assignment*) node->getLinkedData())->getPool());
    }

    return nodeJson;
}

int DomainServer::civetwebRequestHandler(struct mg_connection *connection) {
    const struct mg_request_info* ri = mg_get_request_info(connection);
    
    const char RESPONSE_200[] = "HTTP/1.0 200 OK\r\n\r\n";
    const char RESPONSE_400[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
    
    const char URI_ASSIGNMENT[] = "/assignment";
    const char URI_NODE[] = "/node";
    
    if (strcmp(ri->request_method, "GET") == 0) {
        if (strcmp(ri->uri, "/assignments.json") == 0) {
            // user is asking for json list of assignments
            
            // start with a 200 response
            mg_printf(connection, "%s", RESPONSE_200);
            
            // setup the JSON
            QJsonObject assignmentJSON;
            QJsonObject assignedNodesJSON;
            
            // enumerate the NodeList to find the assigned nodes
            NodeList* nodeList = NodeList::getInstance();
            
            for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                if (node->getLinkedData()) {
                    // add the node using the UUID as the key
                    QString uuidString = uuidStringWithoutCurlyBraces(node->getUUID());
                    assignedNodesJSON[uuidString] = jsonObjectForNode(&(*node));
                }
            }
            
            assignmentJSON["fulfilled"] = assignedNodesJSON;
            
            QJsonObject queuedAssignmentsJSON;
            
            // add the queued but unfilled assignments to the json
            std::deque<Assignment*>::iterator assignment = domainServerInstance->_assignmentQueue.begin();
            
            while (assignment != domainServerInstance->_assignmentQueue.end()) {
                QJsonObject queuedAssignmentJSON;
                
                QString uuidString = uuidStringWithoutCurlyBraces((*assignment)->getUUID());
                queuedAssignmentJSON[JSON_KEY_TYPE] = QString((*assignment)->getTypeName());
                
                // if the assignment has a pool, add it
                if ((*assignment)->hasPool()) {
                    queuedAssignmentJSON[JSON_KEY_POOL] = QString((*assignment)->getPool());
                }
                
                // add this queued assignment to the JSON
                queuedAssignmentsJSON[uuidString] = queuedAssignmentJSON;
                
                // push forward the iterator to check the next assignment
                assignment++;
            }
            
            assignmentJSON["queued"] = queuedAssignmentsJSON;
            
            // print out the created JSON
            QJsonDocument assignmentDocument(assignmentJSON);
            mg_printf(connection, "%s", assignmentDocument.toJson().constData());
            
            // we've processed this request
            return 1;
        } else if (strcmp(ri->uri, "/nodes.json") == 0) {
            // start with a 200 response
            mg_printf(connection, "%s", RESPONSE_200);
            
            // setup the JSON
            QJsonObject rootJSON;
            QJsonObject nodesJSON;
            
            // enumerate the NodeList to find the assigned nodes
            NodeList* nodeList = NodeList::getInstance();
            
            for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                // add the node using the UUID as the key
                QString uuidString = uuidStringWithoutCurlyBraces(node->getUUID());
                nodesJSON[uuidString] = jsonObjectForNode(&(*node));
            }
            
            rootJSON["nodes"] = nodesJSON;
            
            // print out the created JSON
            QJsonDocument nodesDocument(rootJSON);
            mg_printf(connection, "%s", nodesDocument.toJson().constData());
            
            // we've processed this request
            return 1;
        }
        
        // not processed, pass to document root
        return 0;
    } else if (strcmp(ri->request_method, "POST") == 0) {
        if (strcmp(ri->uri, URI_ASSIGNMENT) == 0) {
            // return a 200
            mg_printf(connection, "%s", RESPONSE_200);
            // upload the file
            mg_upload(connection, "/tmp");
            
            return 1;
        }
        
        return 0;
    } else if (strcmp(ri->request_method, "DELETE") == 0) {
        // this is a DELETE request
        
        // check if it is for an assignment
        if (memcmp(ri->uri, URI_NODE, strlen(URI_NODE)) == 0) {
            // pull the UUID from the url
            QUuid deleteUUID = QUuid(QString(ri->uri + strlen(URI_NODE) + sizeof('/')));
            
            if (!deleteUUID.isNull()) {
                Node *nodeToKill = NodeList::getInstance()->nodeWithUUID(deleteUUID);
                
                if (nodeToKill) {
                    // start with a 200 response
                    mg_printf(connection, "%s", RESPONSE_200);
                    
                    // we have a valid UUID and node - kill the node that has this assignment
                    NodeList::getInstance()->killNode(nodeToKill);
                    
                    // successfully processed request
                    return 1;
                }
            }
        }
        
        // request not processed - bad request
        mg_printf(connection, "%s", RESPONSE_400);
        
        // this was processed by civetweb
        return 1;
    } else {
        // have mongoose process this request from the document_root
        return 0;
    }
}

const char ASSIGNMENT_SCRIPT_HOST_LOCATION[] = "resources/web/assignment";

void DomainServer::civetwebUploadHandler(struct mg_connection *connection, const char *path) {
    
    // create an assignment for this saved script, for now make it local only
    Assignment* scriptAssignment = new Assignment(Assignment::CreateCommand,
                                                  Assignment::AgentType,
                                                  NULL,
                                                  Assignment::LocalLocation);
    
    // check how many instances of this assignment the user wants by checking the ASSIGNMENT-INSTANCES header
    const char ASSIGNMENT_INSTANCES_HTTP_HEADER[] = "ASSIGNMENT-INSTANCES";
    const char* requestInstancesHeader = mg_get_header(connection, ASSIGNMENT_INSTANCES_HTTP_HEADER);
    
    if (requestInstancesHeader) {
        // the user has requested a number of instances greater than 1
        // so set that on the created assignment
        scriptAssignment->setNumberOfInstances(atoi(requestInstancesHeader));
    }
    
    QString newPath(ASSIGNMENT_SCRIPT_HOST_LOCATION);
    newPath += "/";
    // append the UUID for this script as the new filename, remove the curly braces
    newPath += uuidStringWithoutCurlyBraces(scriptAssignment->getUUID());
    
    // rename the saved script to the GUID of the assignment and move it to the script host locaiton
    rename(path, newPath.toLocal8Bit().constData());
    
    qDebug("Saved a script for assignment at %s\n", newPath.toLocal8Bit().constData());
    
    // add the script assigment to the assignment queue
    // lock the assignment queue mutex since we're operating on a different thread than DS main
    domainServerInstance->_assignmentQueueMutex.lock();
    domainServerInstance->_assignmentQueue.push_back(scriptAssignment);
    domainServerInstance->_assignmentQueueMutex.unlock();
}

void DomainServer::addReleasedAssignmentBackToQueue(Assignment* releasedAssignment) {
    qDebug() << "Adding assignment" << *releasedAssignment << " back to queue.\n";
    
    // find this assignment in the static file
    for (int i = 0; i < MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS; i++) {
        if (_staticAssignments[i].getUUID() == releasedAssignment->getUUID()) {
            // reset the UUID on the static assignment
            _staticAssignments[i].resetUUID();
            
            // put this assignment back in the queue so it goes out
            _assignmentQueueMutex.lock();
            _assignmentQueue.push_back(&_staticAssignments[i]);
            _assignmentQueueMutex.unlock();
            
        } else if (_staticAssignments[i].getUUID().isNull()) {
            // we are at the blank part of the static assignments - break out
            break;
        }
    }
}

void DomainServer::nodeAdded(Node* node) {
    
}

void DomainServer::nodeKilled(Node* node) {
    // if this node has linked data it was from an assignment
    if (node->getLinkedData()) {
        Assignment* nodeAssignment =  (Assignment*) node->getLinkedData();
        
        addReleasedAssignmentBackToQueue(nodeAssignment);
    }
}

unsigned char* DomainServer::addNodeToBroadcastPacket(unsigned char* currentPosition, Node* nodeToAdd) {
    *currentPosition++ = nodeToAdd->getType();
    
    
    QByteArray rfcUUID = nodeToAdd->getUUID().toRfc4122();
    memcpy(currentPosition, rfcUUID.constData(), rfcUUID.size());
    currentPosition += rfcUUID.size();
    
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
    _voxelServerConfig(NULL),
    _hasCompletedRestartHold(false)
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
    
    QString documentRootString = QString("%1/resources/web").arg(QCoreApplication::applicationDirPath());
    
    char documentRoot[documentRootString.size() + 1];
    strcpy(documentRoot, documentRootString.toLocal8Bit().constData());
    
    // list of options. Last element must be NULL.
    const char* options[] = {"listening_ports", "8080",
        "document_root", documentRoot, NULL};
    
    callbacks.begin_request = civetwebRequestHandler;
    callbacks.upload = civetwebUploadHandler;
    
    // Start the web server.
    mg_start(&callbacks, NULL, options);
}

void DomainServer::prepopulateStaticAssignmentFile() {
    int numFreshStaticAssignments = 0;
    
    // write a fresh static assignment array to file
    
    Assignment freshStaticAssignments[MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS];
    
    // pre-populate the first static assignment list with assignments for root AuM, AvM, VS
    freshStaticAssignments[numFreshStaticAssignments++] = Assignment(Assignment::CreateCommand, Assignment::AudioMixerType);
    freshStaticAssignments[numFreshStaticAssignments++] = Assignment(Assignment::CreateCommand, Assignment::AvatarMixerType);
    
    // Handle Domain/Voxel Server configuration command line arguments
    if (_voxelServerConfig) {
        qDebug("Reading Voxel Server Configuration.\n");
        qDebug() << "config: " << _voxelServerConfig << "\n";
        
        QString multiConfig((const char*) _voxelServerConfig);
        QStringList multiConfigList = multiConfig.split(";");
        
        // read each config to a payload for a VS assignment
        for (int i = 0; i < multiConfigList.size(); i++) {
            QString config = multiConfigList.at(i);
            
            qDebug("config[%d]=%s\n", i, config.toLocal8Bit().constData());
            
            // Now, parse the config to check for a pool
            const char ASSIGNMENT_CONFIG_POOL_OPTION[] = "--pool";
            QString assignmentPool;
            
            int poolIndex = config.indexOf(ASSIGNMENT_CONFIG_POOL_OPTION);
            
            if (poolIndex >= 0) {
                int spaceBeforePoolIndex = config.indexOf(' ', poolIndex);
                int spaceAfterPoolIndex = config.indexOf(' ', spaceBeforePoolIndex);
                
                assignmentPool = config.mid(spaceBeforePoolIndex + 1, spaceAfterPoolIndex);
                qDebug() << "The pool for this voxel-assignment is" << assignmentPool << "\n";
            }
            
            Assignment voxelServerAssignment(Assignment::CreateCommand,
                                             Assignment::VoxelServerType,
                                             (assignmentPool.isEmpty() ? NULL : assignmentPool.toLocal8Bit().constData()));
            
            int payloadLength = config.length() + sizeof(char);
            voxelServerAssignment.setPayload((uchar*)config.toLocal8Bit().constData(), payloadLength);
            
            freshStaticAssignments[numFreshStaticAssignments++] = voxelServerAssignment;
        }
    } else {
        Assignment rootVoxelServerAssignment(Assignment::CreateCommand, Assignment::VoxelServerType);
        freshStaticAssignments[numFreshStaticAssignments++] = rootVoxelServerAssignment;
    }
    
    qDebug() << "Adding" << numFreshStaticAssignments << "static assignments to fresh file.\n";
    
    _staticAssignmentFile.open(QIODevice::WriteOnly);
    _staticAssignmentFile.write((char*) &freshStaticAssignments, sizeof(freshStaticAssignments));
    _staticAssignmentFile.resize(MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS * sizeof(Assignment));
    _staticAssignmentFile.close();
}

Assignment* DomainServer::matchingStaticAssignmentForCheckIn(const QUuid& checkInUUID, NODE_TYPE nodeType) {
    // pull the UUID passed with the check in
    
    if (_hasCompletedRestartHold) {
        _assignmentQueueMutex.lock();
        
        // iterate the assignment queue to check for a match
        std::deque<Assignment*>::iterator assignment = _assignmentQueue.begin();
        while (assignment != _assignmentQueue.end()) {
            if ((*assignment)->getUUID() == checkInUUID) {
                // return the matched assignment
                _assignmentQueueMutex.unlock();
                return *assignment;
            } else {
                // no match, push deque iterator forwards
                assignment++;
            }
        }
        
        _assignmentQueueMutex.unlock();
    } else {
        for (int i = 0; i < MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS; i++) {
            if (_staticAssignments[i].getUUID() == checkInUUID) {
                // return matched assignment
                return &_staticAssignments[i];
            } else if (_staticAssignments[i].getUUID().isNull()) {
                // end of static assignments, no match - return NULL
                return NULL;
            }
        }
    }
    
    return NULL;
}

Assignment* DomainServer::deployableAssignmentForRequest(Assignment& requestAssignment) {
    _assignmentQueueMutex.lock();
    
    // this is an unassigned client talking to us directly for an assignment
    // go through our queue and see if there are any assignments to give out
    std::deque<Assignment*>::iterator assignment = _assignmentQueue.begin();
    
    while (assignment != _assignmentQueue.end()) {
        bool requestIsAllTypes = requestAssignment.getType() == Assignment::AllTypes;
        bool assignmentTypesMatch = (*assignment)->getType() == requestAssignment.getType();
        bool nietherHasPool = !(*assignment)->hasPool() && !requestAssignment.hasPool();
        bool assignmentPoolsMatch = memcmp((*assignment)->getPool(),
                                           requestAssignment.getPool(),
                                           MAX_ASSIGNMENT_POOL_BYTES) == 0;
        
        if ((requestIsAllTypes || assignmentTypesMatch) && (nietherHasPool || assignmentPoolsMatch)) {
            
            Assignment* deployableAssignment = *assignment;
            
            if ((*assignment)->getType() == Assignment::AgentType) {
                // if there is more than one instance to send out, simply decrease the number of instances
                
                if ((*assignment)->getNumberOfInstances() == 1) {
                    _assignmentQueue.erase(assignment);
                }
                
                deployableAssignment->decrementNumberOfInstances();
                
            } else {
                // remove the assignment from the queue
                _assignmentQueue.erase(assignment);
                
                // until we get a check-in from that GUID
                // put assignment back in queue but stick it at the back so the others have a chance to go out
                _assignmentQueue.push_back(deployableAssignment);
            }
            
            // stop looping, we've handed out an assignment
            _assignmentQueueMutex.unlock();
            return deployableAssignment;
        } else {
            // push forward the iterator to check the next assignment
            assignment++;
        }
    }
    
    _assignmentQueueMutex.unlock();
    return NULL;
}

void DomainServer::removeAssignmentFromQueue(Assignment* removableAssignment) {
    
    _assignmentQueueMutex.lock();
    
    std::deque<Assignment*>::iterator assignment = _assignmentQueue.begin();
    
    while (assignment != _assignmentQueue.end()) {
        if ((*assignment)->getUUID() == removableAssignment->getUUID()) {
            _assignmentQueue.erase(assignment);
            break;
        } else {
            // push forward the iterator to check the next assignment
            assignment++;
        }
    }
    
    _assignmentQueueMutex.unlock();
}

bool DomainServer::checkInWithUUIDMatchesExistingNode(sockaddr* nodePublicSocket,
                                                      sockaddr* nodeLocalSocket,
                                                      const QUuid& checkInUUID) {
    NodeList* nodeList = NodeList::getInstance();
    
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (node->getLinkedData()
            && socketMatch(node->getPublicSocket(), nodePublicSocket)
            && socketMatch(node->getLocalSocket(), nodeLocalSocket)
            && node->getUUID() == checkInUUID) {
            // this is a matching existing node if the public socket, local socket, and UUID match
            return true;
        }
    }
    
    return false;
}

void DomainServer::possiblyAddStaticAssignmentsBackToQueueAfterRestart(timeval* startTime) {
    // if the domain-server has just restarted,
    // check if there are static assignments in the file that we need to
    // throw into the assignment queue
    const uint64_t RESTART_HOLD_TIME_USECS = 5 * 1000 * 1000;
    
    if (!_hasCompletedRestartHold && usecTimestampNow() - usecTimestamp(startTime) > RESTART_HOLD_TIME_USECS) {
        _hasCompletedRestartHold = true;
        
        // pull anything in the static assignment file that isn't spoken for and add to the assignment queue
        for (int i = 0; i < MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS; i++) {
            if (_staticAssignments[i].getUUID().isNull()) {
                // reached the end of static assignments, bail
                break;
            }
            
            bool foundMatchingAssignment = false;
            
            NodeList* nodeList = NodeList::getInstance();
            
            // enumerate the nodes and check if there is one with an attached assignment with matching UUID
            for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                if (node->getLinkedData()) {
                    Assignment* linkedAssignment = (Assignment*) node->getLinkedData();
                    if (linkedAssignment->getUUID() == _staticAssignments[i].getUUID()) {
                        foundMatchingAssignment = true;
                        break;
                    }
                }
            }
            
            if (!foundMatchingAssignment) {
                // this assignment has not been fulfilled - reset the UUID and add it to the assignment queue
                _staticAssignments[i].resetUUID();
                
                qDebug() << "Adding static assignment to queue -" << _staticAssignments[i] << "\n";
                
                _assignmentQueueMutex.lock();
                _assignmentQueue.push_back(&_staticAssignments[i]);
                _assignmentQueueMutex.unlock();
            }
        }
    }
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
    
    sockaddr_in senderAddress, nodePublicAddress, nodeLocalAddress;
    nodePublicAddress.sin_family = AF_INET;
    nodeLocalAddress.sin_family = AF_INET;
    
    nodeList->startSilentNodeRemovalThread();
    
    if (!_staticAssignmentFile.exists() || _voxelServerConfig) {
        
        if (_voxelServerConfig) {
            // we have a new VS config, clear the existing file to start fresh
            _staticAssignmentFile.remove();
        }
        
        prepopulateStaticAssignmentFile();
    }
    
    _staticAssignmentFile.open(QIODevice::ReadWrite);
    
    _staticAssignmentFileData = _staticAssignmentFile.map(0, _staticAssignmentFile.size());
    
    _staticAssignments = (Assignment*) _staticAssignmentFileData;
    
    timeval startTime;
    gettimeofday(&startTime, NULL);
    
    while (true) {
        while (nodeList->getNodeSocket()->receive((sockaddr *)&senderAddress, packetData, &receivedBytes) &&
               packetVersionMatch(packetData)) {
            if (packetData[0] == PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY || packetData[0] == PACKET_TYPE_DOMAIN_LIST_REQUEST) {
                // this is an RFD or domain list request packet, and there is a version match
                
                int numBytesSenderHeader = numBytesForPacketHeader(packetData);
                
                nodeType = *(packetData + numBytesSenderHeader);
                
                int packetIndex = numBytesSenderHeader + sizeof(NODE_TYPE);
                QUuid nodeUUID = QUuid::fromRfc4122(QByteArray(((char*) packetData + packetIndex), NUM_BYTES_RFC4122_UUID));
                packetIndex += NUM_BYTES_RFC4122_UUID;
                
                int numBytesPrivateSocket = unpackSocket(packetData + packetIndex, (sockaddr*) &nodePublicAddress);
                packetIndex += numBytesPrivateSocket;
                
                if (nodePublicAddress.sin_addr.s_addr == 0) {
                    // this node wants to use us its STUN server
                    // so set the node public address to whatever we perceive the public address to be
                    
                    nodePublicAddress = senderAddress;
                    
                    // if the sender is on our box then leave its public address to 0 so that
                    // other users attempt to reach it on the same address they have for the domain-server
                    if (senderAddress.sin_addr.s_addr == htonl(INADDR_LOOPBACK)) {
                        nodePublicAddress.sin_addr.s_addr = 0;
                    }
                }
                
                int numBytesPublicSocket = unpackSocket(packetData + packetIndex, (sockaddr*) &nodeLocalAddress);
                packetIndex += numBytesPublicSocket;
                
                const char STATICALLY_ASSIGNED_NODES[3] = {
                    NODE_TYPE_AUDIO_MIXER,
                    NODE_TYPE_AVATAR_MIXER,
                    NODE_TYPE_VOXEL_SERVER
                };
                
                Assignment* matchingStaticAssignment = NULL;
                
                if (memchr(STATICALLY_ASSIGNED_NODES, nodeType, sizeof(STATICALLY_ASSIGNED_NODES)) == NULL
                    || ((matchingStaticAssignment = matchingStaticAssignmentForCheckIn(nodeUUID, nodeType))
                        || checkInWithUUIDMatchesExistingNode((sockaddr*) &nodePublicAddress,
                                                              (sockaddr*) &nodeLocalAddress,
                                                              nodeUUID)))
                {
                    Node* checkInNode = nodeList->addOrUpdateNode(nodeUUID,
                                                                  nodeType,
                                                                  (sockaddr*) &nodePublicAddress,
                                                                  (sockaddr*) &nodeLocalAddress);
                    
                    if (matchingStaticAssignment) {
                        // this was a newly added node with a matching static assignment
                        
                        if (_hasCompletedRestartHold) {
                            // remove the matching assignment from the assignment queue so we don't take the next check in
                            removeAssignmentFromQueue(matchingStaticAssignment);
                        }
                        
                        // set the linked data for this node to a copy of the matching assignment
                        // so we can re-queue it should the node die
                        Assignment* nodeCopyOfMatchingAssignment = new Assignment(*matchingStaticAssignment);
                
                        checkInNode->setLinkedData(nodeCopyOfMatchingAssignment);
                    }
                    
                    int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_DOMAIN);
                    
                    currentBufferPos = broadcastPacket + numHeaderBytes;
                    startPointer = currentBufferPos;
                    
                    unsigned char* nodeTypesOfInterest = packetData + packetIndex + sizeof(unsigned char);
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
                    
                    // send the constructed list back to this node
                    nodeList->getNodeSocket()->send((sockaddr*)&senderAddress,
                                                    broadcastPacket,
                                                    (currentBufferPos - startPointer) + numHeaderBytes);
                }
            } else if (packetData[0] == PACKET_TYPE_REQUEST_ASSIGNMENT) {
                
                qDebug("Received a request for assignment.\n");
                
                if (!_hasCompletedRestartHold) {
                    possiblyAddStaticAssignmentsBackToQueueAfterRestart(&startTime);
                }
                
                if (_assignmentQueue.size() > 0) {
                    // construct the requested assignment from the packet data
                    Assignment requestAssignment(packetData, receivedBytes);
                    
                    Assignment* assignmentToDeploy = deployableAssignmentForRequest(requestAssignment);
                    
                    if (assignmentToDeploy) {
                    
                        // give this assignment out, either the type matches or the requestor said they will take any
                        int numHeaderBytes = populateTypeAndVersion(broadcastPacket, PACKET_TYPE_CREATE_ASSIGNMENT);
                        int numAssignmentBytes = assignmentToDeploy->packToBuffer(broadcastPacket + numHeaderBytes);
        
                        nodeList->getNodeSocket()->send((sockaddr*) &senderAddress,
                                                        broadcastPacket,
                                                        numHeaderBytes + numAssignmentBytes);
                        
                        if (assignmentToDeploy->getNumberOfInstances() == 0) {
                            // there are no more instances of this script to send out, delete it
                            delete assignmentToDeploy;
                        }
                    }
                    
                }
            } else if (packetData[0] == PACKET_TYPE_CREATE_ASSIGNMENT) {
                // this is a create assignment likely recieved from a server needed more clients to help with load
                
                // unpack it
                Assignment* createAssignment = new Assignment(packetData, receivedBytes);
                
                qDebug() << "Received a create assignment -" << *createAssignment << "\n";
                
                // make sure we have a matching node with the UUID packed with the assignment
                // if the node has sent no types of interest, assume they want nothing but their own ID back
                for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                    if (node->getLinkedData()
                        && socketMatch((sockaddr*) &senderAddress, node->getPublicSocket())
                        && ((Assignment*) node->getLinkedData())->getUUID() == createAssignment->getUUID()) {
                        
                        // give the create assignment a new UUID
                        createAssignment->resetUUID();
                        
                        // add the assignment at the back of the queue
                        _assignmentQueueMutex.lock();
                        _assignmentQueue.push_back(createAssignment);
                        _assignmentQueueMutex.unlock();
                        
                        // find the first available spot in the static assignments and put this assignment there
                        for (int i = 0; i < MAX_STATIC_ASSIGNMENT_FILE_ASSIGNMENTS; i++) {
                            if (_staticAssignments[i].getUUID().isNull()) {
                                _staticAssignments[i] = *createAssignment;
                                
                                // we've stuck the assignment in, break out
                                break;
                            }
                        }
                        
                        // we found the matching node that asked for create assignment, break out
                        break;
                    }
                }
            }
        }
        
        if (!_hasCompletedRestartHold) {
            possiblyAddStaticAssignmentsBackToQueueAfterRestart(&startTime);
        }
    }
    
    this->cleanup();
    
    return 0;
}