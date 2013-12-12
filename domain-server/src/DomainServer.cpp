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
#include <QtCore/QTimer>

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "DomainServer.h"

const int RESTART_HOLD_TIME_MSECS = 5 * 1000;

void signalhandler(int sig){
    if (sig == SIGINT) {
        qApp->quit();
    }
}

DomainServer* DomainServer::domainServerInstance = NULL;

DomainServer::DomainServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _assignmentQueueMutex(),
    _assignmentQueue(),
    _staticAssignmentFile(QString("%1/config.ds").arg(QCoreApplication::applicationDirPath())),
    _staticAssignmentFileData(NULL),
    _voxelServerConfig(NULL),
    _hasCompletedRestartHold(false)
{
    DomainServer::setDomainServerInstance(this);
    
    signal(SIGINT, signalhandler);
    
    const char CUSTOM_PORT_OPTION[] = "-p";
    const char* customPortString = getCmdOption(argc, (const char**) argv, CUSTOM_PORT_OPTION);
    unsigned short domainServerPort = customPortString ? atoi(customPortString) : DEFAULT_DOMAIN_SERVER_PORT;
    
    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_DOMAIN, domainServerPort);
    
    const char VOXEL_CONFIG_OPTION[] = "--voxelServerConfig";
    _voxelServerConfig = getCmdOption(argc, (const char**) argv, VOXEL_CONFIG_OPTION);

    const char PARTICLE_CONFIG_OPTION[] = "--particleServerConfig";
    _particleServerConfig = getCmdOption(argc, (const char**) argv, PARTICLE_CONFIG_OPTION);
    
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
    
    nodeList->addHook(this);
    
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
    
    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_USECS / 1000);
    
    connect(&nodeList->getNodeSocket(), SIGNAL(readyRead()), SLOT(readAvailableDatagrams()));
    
    // fire a single shot timer to add static assignments back into the queue after a restart
    QTimer::singleShot(RESTART_HOLD_TIME_MSECS, this, SLOT(addStaticAssignmentsBackToQueueAfterRestart()));
    
    connect(this, SIGNAL(aboutToQuit()), SLOT(cleanup()));
}

void DomainServer::readAvailableDatagrams() {
    NodeList* nodeList = NodeList::getInstance();
    
    HifiSockAddr senderSockAddr, nodePublicAddress, nodeLocalAddress;
    
    static unsigned char packetData[MAX_PACKET_SIZE];
    
    static unsigned char broadcastPacket[MAX_PACKET_SIZE];
    
    static unsigned char* currentBufferPos;
    static unsigned char* startPointer;
    
    int receivedBytes = 0;
    
    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        if ((receivedBytes = nodeList->getNodeSocket().readDatagram((char*) packetData, MAX_PACKET_SIZE,
                                                   senderSockAddr.getAddressPointer(),
                                                   senderSockAddr.getPortPointer()))
            && packetVersionMatch((unsigned char*) packetData)) {
            if (packetData[0] == PACKET_TYPE_DOMAIN_REPORT_FOR_DUTY || packetData[0] == PACKET_TYPE_DOMAIN_LIST_REQUEST) {
                // this is an RFD or domain list request packet, and there is a version match
                
                int numBytesSenderHeader = numBytesForPacketHeader((unsigned char*) packetData);
                
                NODE_TYPE nodeType = *(packetData + numBytesSenderHeader);
                
                int packetIndex = numBytesSenderHeader + sizeof(NODE_TYPE);
                QUuid nodeUUID = QUuid::fromRfc4122(QByteArray(((char*) packetData + packetIndex), NUM_BYTES_RFC4122_UUID));
                packetIndex += NUM_BYTES_RFC4122_UUID;
                
                int numBytesPrivateSocket = HifiSockAddr::unpackSockAddr(packetData + packetIndex, nodePublicAddress);
                packetIndex += numBytesPrivateSocket;
                
                if (nodePublicAddress.getAddress().isNull()) {
                    // this node wants to use us its STUN server
                    // so set the node public address to whatever we perceive the public address to be
                    
                    // if the sender is on our box then leave its public address to 0 so that
                    // other users attempt to reach it on the same address they have for the domain-server
                    if (senderSockAddr.getAddress().isLoopback()) {
                        nodePublicAddress.setAddress(QHostAddress());
                    } else {
                        nodePublicAddress.setAddress(senderSockAddr.getAddress());
                    }
                }
                
                int numBytesPublicSocket = HifiSockAddr::unpackSockAddr(packetData + packetIndex, nodeLocalAddress);
                packetIndex += numBytesPublicSocket;
                
                const char STATICALLY_ASSIGNED_NODES[3] = {
                    NODE_TYPE_AUDIO_MIXER,
                    NODE_TYPE_AVATAR_MIXER,
                    NODE_TYPE_VOXEL_SERVER
                };
                
                Assignment* matchingStaticAssignment = NULL;
                
                if (memchr(STATICALLY_ASSIGNED_NODES, nodeType, sizeof(STATICALLY_ASSIGNED_NODES)) == NULL
                    || ((matchingStaticAssignment = matchingStaticAssignmentForCheckIn(nodeUUID, nodeType))
                        || checkInWithUUIDMatchesExistingNode(nodePublicAddress,
                                                              nodeLocalAddress,
                                                              nodeUUID)))
                {
                    Node* checkInNode = nodeList->addOrUpdateNode(nodeUUID,
                                                                  nodeType,
                                                                  nodePublicAddress,
                                                                  nodeLocalAddress);
                    
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
                            if (node->getUUID() != nodeUUID &&
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
                    nodeList->getNodeSocket().writeDatagram((char*) broadcastPacket,
                                                            (currentBufferPos - startPointer) + numHeaderBytes,
                                                            senderSockAddr.getAddress(), senderSockAddr.getPort());
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
                        
                        nodeList->getNodeSocket().writeDatagram((char*) broadcastPacket, numHeaderBytes + numAssignmentBytes,
                                                                senderSockAddr.getAddress(), senderSockAddr.getPort());
                        
                        if (assignmentToDeploy->getNumberOfInstances() == 0) {
                            // there are no more instances of this script to send out, delete it
                            delete assignmentToDeploy;
                        }
                    }
                    
                }
            }
        }
    }
}

void DomainServer::setDomainServerInstance(DomainServer* domainServer) {
    domainServerInstance = domainServer;
}

QJsonObject jsonForSocket(const HifiSockAddr& socket) {
    QJsonObject socketJSON;
    
    socketJSON["ip"] = socket.getAddress().toString();
    socketJSON["port"] = ntohs(socket.getPort());
    
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
    
    currentPosition += HifiSockAddr::packSockAddr(currentPosition, nodeToAdd->getPublicSocket());
    currentPosition += HifiSockAddr::packSockAddr(currentPosition, nodeToAdd->getLocalSocket());
    
    // return the new unsigned char * for broadcast packet
    return currentPosition;
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

    // Handle Domain/Particle Server configuration command line arguments
    if (_particleServerConfig) {
        qDebug("Reading Particle Server Configuration.\n");
        qDebug() << "config: " << _particleServerConfig << "\n";
        
        QString multiConfig((const char*) _particleServerConfig);
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
                qDebug() << "The pool for this particle-assignment is" << assignmentPool << "\n";
            }
            
            Assignment particleServerAssignment(Assignment::CreateCommand,
                                             Assignment::ParticleServerType,
                                             (assignmentPool.isEmpty() ? NULL : assignmentPool.toLocal8Bit().constData()));
            
            int payloadLength = config.length() + sizeof(char);
            particleServerAssignment.setPayload((uchar*)config.toLocal8Bit().constData(), payloadLength);
            
            freshStaticAssignments[numFreshStaticAssignments++] = particleServerAssignment;
        }
    } else {
        Assignment rootParticleServerAssignment(Assignment::CreateCommand, Assignment::ParticleServerType);
        freshStaticAssignments[numFreshStaticAssignments++] = rootParticleServerAssignment;
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

bool DomainServer::checkInWithUUIDMatchesExistingNode(const HifiSockAddr& nodePublicSocket,
                                                      const HifiSockAddr& nodeLocalSocket,
                                                      const QUuid& checkInUUID) {
    NodeList* nodeList = NodeList::getInstance();
    
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (node->getLinkedData()
            && nodePublicSocket == node->getPublicSocket()
            && nodeLocalSocket == node->getLocalSocket()
            && node->getUUID() == checkInUUID) {
            // this is a matching existing node if the public socket, local socket, and UUID match
            return true;
        }
    }
    
    return false;
}

void DomainServer::addStaticAssignmentsBackToQueueAfterRestart() {
    _hasCompletedRestartHold = true;
    
    // if the domain-server has just restarted,
    // check if there are static assignments in the file that we need to
    // throw into the assignment queue
    
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

void DomainServer::cleanup() {
    _staticAssignmentFile.unmap(_staticAssignmentFileData);
    _staticAssignmentFile.close();
}