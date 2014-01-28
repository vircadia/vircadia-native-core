//
//  DomainServer.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <signal.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include <HTTPConnection.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "DomainServer.h"

const int RESTART_HOLD_TIME_MSECS = 5 * 1000;

const char* VOXEL_SERVER_CONFIG = "voxelServerConfig";
const char* PARTICLE_SERVER_CONFIG = "particleServerConfig";
const char* METAVOXEL_SERVER_CONFIG = "metavoxelServerConfig";

void signalhandler(int sig){
    if (sig == SIGINT) {
        qApp->quit();
    }
}

const quint16 DOMAIN_SERVER_HTTP_PORT = 8080;

DomainServer::DomainServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _HTTPManager(DOMAIN_SERVER_HTTP_PORT, QString("%1/resources/web/").arg(QCoreApplication::applicationDirPath()), this),
    _assignmentQueueMutex(),
    _assignmentQueue(),
    _staticAssignmentFile(QString("%1/config.ds").arg(QCoreApplication::applicationDirPath())),
    _staticAssignmentFileData(NULL),
    _voxelServerConfig(NULL),
    _metavoxelServerConfig(NULL),
    _hasCompletedRestartHold(false)
{
    signal(SIGINT, signalhandler);

    const char CUSTOM_PORT_OPTION[] = "-p";
    const char* customPortString = getCmdOption(argc, (const char**) argv, CUSTOM_PORT_OPTION);
    unsigned short domainServerPort = customPortString ? atoi(customPortString) : DEFAULT_DOMAIN_SERVER_PORT;

    const char CONFIG_FILE_OPTION[] = "-c";
    const char* configFilePath = getCmdOption(argc, (const char**) argv, CONFIG_FILE_OPTION);

    if (!readConfigFile(configFilePath)) {
        QByteArray voxelConfigOption = QString("--%1").arg(VOXEL_SERVER_CONFIG).toLocal8Bit();
        _voxelServerConfig = getCmdOption(argc, (const char**) argv, voxelConfigOption.constData());

        QByteArray particleConfigOption = QString("--%1").arg(PARTICLE_SERVER_CONFIG).toLocal8Bit();
        _particleServerConfig = getCmdOption(argc, (const char**) argv, particleConfigOption.constData());

        QByteArray metavoxelConfigOption = QString("--%1").arg(METAVOXEL_SERVER_CONFIG).toLocal8Bit();
        _metavoxelServerConfig = getCmdOption(argc, (const char**) argv, metavoxelConfigOption.constData());
    }

    NodeList* nodeList = NodeList::createInstance(NODE_TYPE_DOMAIN, domainServerPort);

    connect(nodeList, SIGNAL(nodeKilled(SharedNodePointer)), this, SLOT(nodeKilled(SharedNodePointer)));

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
    
    static QByteArray broadcastPacket = byteArrayWithPopluatedHeader(PacketTypeDomainList);
    static int numBroadcastPacketHeaderBytes = broadcastPacket.size();
    
    static QByteArray assignmentPacket = byteArrayWithPopluatedHeader(PacketTypeCreateAssignment);
    static int numAssignmentPacketHeaderBytes = assignmentPacket.size();
    
    QByteArray receivedPacket;
    NODE_TYPE nodeType;
    QUuid nodeUUID;

    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        receivedPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(receivedPacket.data(), receivedPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        if (packetVersionMatch(receivedPacket)) {
            PacketType requestType = packetTypeForPacket(receivedPacket);
            if (requestType == PacketTypeDomainListRequest) {
                
                // this is an RFD or domain list request packet, and there is a version match
                QDataStream packetStream(receivedPacket);
                packetStream.skipRawData(numBytesForPacketHeader(receivedPacket));
                
                deconstructPacketHeader(receivedPacket, nodeUUID);
                packetStream >> nodeType;
                packetStream >> nodePublicAddress >> nodeLocalAddress;
                
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
                
                const QSet<NODE_TYPE> STATICALLY_ASSIGNED_NODES = QSet<NODE_TYPE>() << NODE_TYPE_AUDIO_MIXER
                    << NODE_TYPE_AVATAR_MIXER << NODE_TYPE_VOXEL_SERVER << NODE_TYPE_PARTICLE_SERVER
                    << NODE_TYPE_METAVOXEL_SERVER;
                
                Assignment* matchingStaticAssignment = NULL;
                
                if (!STATICALLY_ASSIGNED_NODES.contains(nodeType)
                    || ((matchingStaticAssignment = matchingStaticAssignmentForCheckIn(nodeUUID, nodeType))
                        || checkInWithUUIDMatchesExistingNode(nodePublicAddress,
                                                              nodeLocalAddress,
                                                              nodeUUID)))
                {
                    SharedNodePointer checkInNode = nodeList->addOrUpdateNode(nodeUUID,
                                                                              nodeType,
                                                                              nodePublicAddress,
                                                                              nodeLocalAddress);
                    
                    // resize our broadcast packet in preparation to set it up again
                    broadcastPacket.resize(numBroadcastPacketHeaderBytes);
                    
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
                    
                    
                    quint8 numInterestTypes = 0;
                    packetStream >> numInterestTypes;
                    
                    NODE_TYPE* nodeTypesOfInterest = reinterpret_cast<NODE_TYPE*>(receivedPacket.data()
                                                                                  + packetStream.device()->pos());
                    
                    if (numInterestTypes > 0) {
                        QDataStream broadcastDataStream(&broadcastPacket, QIODevice::Append);
                        
                        // if the node has sent no types of interest, assume they want nothing but their own ID back
                        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
                            if (node->getUUID() != nodeUUID &&
                                memchr(nodeTypesOfInterest, node->getType(), numInterestTypes)) {
                                
                                // don't send avatar nodes to other avatars, that will come from avatar mixer
                                broadcastDataStream << *node.data();
                            }
                        }
                    }
                    
                    // update last receive to now
                    uint64_t timeNow = usecTimestampNow();
                    checkInNode->setLastHeardMicrostamp(timeNow);
                    
                    // send the constructed list back to this node
                    nodeList->getNodeSocket().writeDatagram(broadcastPacket,
                                                            senderSockAddr.getAddress(), senderSockAddr.getPort());
                }
            } else if (requestType == PacketTypeRequestAssignment) {
                
                if (_assignmentQueue.size() > 0) {
                    // construct the requested assignment from the packet data
                    Assignment requestAssignment(receivedPacket);
                    
                    qDebug("Received a request for assignment type %i from %s.",
                           requestAssignment.getType(), qPrintable(senderSockAddr.getAddress().toString()));
                    
                    Assignment* assignmentToDeploy = deployableAssignmentForRequest(requestAssignment);
                    
                    if (assignmentToDeploy) {
                        // give this assignment out, either the type matches or the requestor said they will take any
                        assignmentPacket.resize(numAssignmentPacketHeaderBytes);
                        
                        QDataStream assignmentStream(&assignmentPacket, QIODevice::Append);
                        
                        assignmentStream << *assignmentToDeploy;
                        
                        nodeList->getNodeSocket().writeDatagram(assignmentPacket,
                                                                senderSockAddr.getAddress(), senderSockAddr.getPort());
                        
                        if (assignmentToDeploy->getNumberOfInstances() == 0) {
                            // there are no more instances of this script to send out, delete it
                            delete assignmentToDeploy;
                        }
                    }
                    
                } else {
                    qDebug() << "Received an invalid assignment request from" << senderSockAddr.getAddress();
                }
            }
        }
    }
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
    if (node->getLinkedData() && !((Assignment*) node->getLinkedData())->getPool().isEmpty()) {
        nodeJson[JSON_KEY_POOL] = ((Assignment*) node->getLinkedData())->getPool();
    }

    return nodeJson;
}

// Attempts to read configuration from specified path
// returns true on success, false otherwise
bool DomainServer::readConfigFile(const char* path) {
    if (!path) {
        // config file not specified
        return false;
    }

    if (!QFile::exists(path)) {
        qWarning("Specified configuration file does not exist!\n");
        return false;
    }

    QFile configFile(path);
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Can't open specified configuration file!\n");
        return false;
    } else {
        qDebug("Reading configuration from %s\n", path);
    }
    QTextStream configStream(&configFile);
    QByteArray configStringByteArray = configStream.readAll().toUtf8();
    QJsonObject configDocObject = QJsonDocument::fromJson(configStringByteArray).object();
    configFile.close();

    QString voxelServerConfig = readServerAssignmentConfig(configDocObject, VOXEL_SERVER_CONFIG);
    _voxelServerConfig = new char[strlen(voxelServerConfig.toLocal8Bit().constData()) +1];
    _voxelServerConfig = strcpy((char *) _voxelServerConfig, voxelServerConfig.toLocal8Bit().constData() + '\0');

    QString particleServerConfig = readServerAssignmentConfig(configDocObject, PARTICLE_SERVER_CONFIG);
    _particleServerConfig = new char[strlen(particleServerConfig.toLocal8Bit().constData()) +1];
    _particleServerConfig = strcpy((char *) _particleServerConfig, particleServerConfig.toLocal8Bit().constData() + '\0');

    QString metavoxelServerConfig = readServerAssignmentConfig(configDocObject, METAVOXEL_SERVER_CONFIG);
    _metavoxelServerConfig = new char[strlen(metavoxelServerConfig.toLocal8Bit().constData()) +1];
    _metavoxelServerConfig = strcpy((char *) _metavoxelServerConfig, metavoxelServerConfig.toLocal8Bit().constData() + '\0');

    return true;
}

// find assignment configurations on the specified node name and json object
// returns a string in the form of its equivalent cmd line params
QString DomainServer::readServerAssignmentConfig(QJsonObject jsonObject, const char* nodeName) {
    QJsonArray nodeArray = jsonObject[nodeName].toArray();

    QStringList serverConfig;
    foreach (const QJsonValue & childValue, nodeArray) {
        QString cmdParams;
        QJsonObject childObject = childValue.toObject();
        QStringList keys = childObject.keys();
        for (int i = 0; i < keys.size(); i++) {
            QString key = keys[i];
            QString value = childObject[key].toString();
            // both cmd line params and json keys are the same
            cmdParams += QString("--%1 %2 ").arg(key, value);
        }
        serverConfig << cmdParams;
    }

    // according to split() calls from DomainServer::prepopulateStaticAssignmentFile
    // we shold simply join them with semicolons
    return serverConfig.join(';');
}

bool DomainServer::handleHTTPRequest(HTTPConnection* connection, const QString& path) {
    const QString JSON_MIME_TYPE = "application/json";
    
    const QString URI_ASSIGNMENT = "/assignment";
    const QString URI_NODE = "/node";
    
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation) {
        if (path == "/assignments.json") {
            // user is asking for json list of assignments
            
            // setup the JSON
            QJsonObject assignmentJSON;
            QJsonObject assignedNodesJSON;
            
            // enumerate the NodeList to find the assigned nodes
            foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
                if (node->getLinkedData()) {
                    // add the node using the UUID as the key
                    QString uuidString = uuidStringWithoutCurlyBraces(node->getUUID());
                    assignedNodesJSON[uuidString] = jsonObjectForNode(node.data());
                }
            }
            
            assignmentJSON["fulfilled"] = assignedNodesJSON;
            
            QJsonObject queuedAssignmentsJSON;
            
            // add the queued but unfilled assignments to the json
            std::deque<Assignment*>::iterator assignment = _assignmentQueue.begin();
            
            while (assignment != _assignmentQueue.end()) {
                QJsonObject queuedAssignmentJSON;
                
                QString uuidString = uuidStringWithoutCurlyBraces((*assignment)->getUUID());
                queuedAssignmentJSON[JSON_KEY_TYPE] = QString((*assignment)->getTypeName());
                
                // if the assignment has a pool, add it
                if (!(*assignment)->getPool().isEmpty()) {
                    queuedAssignmentJSON[JSON_KEY_POOL] = (*assignment)->getPool();
                }
                
                // add this queued assignment to the JSON
                queuedAssignmentsJSON[uuidString] = queuedAssignmentJSON;
                
                // push forward the iterator to check the next assignment
                assignment++;
            }
            
            assignmentJSON["queued"] = queuedAssignmentsJSON;
            
            // print out the created JSON
            QJsonDocument assignmentDocument(assignmentJSON);
            connection->respond(HTTPConnection::StatusCode200, assignmentDocument.toJson(), qPrintable(JSON_MIME_TYPE));
            
            // we've processed this request
            return true;
        } else if (path == "/nodes.json") {
            // setup the JSON
            QJsonObject rootJSON;
            QJsonObject nodesJSON;
            
            // enumerate the NodeList to find the assigned nodes
            NodeList* nodeList = NodeList::getInstance();
            
            foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
                // add the node using the UUID as the key
                QString uuidString = uuidStringWithoutCurlyBraces(node->getUUID());
                nodesJSON[uuidString] = jsonObjectForNode(node.data());
            }
            
            rootJSON["nodes"] = nodesJSON;
            
            // print out the created JSON
            QJsonDocument nodesDocument(rootJSON);
            
            // send the response
            connection->respond(HTTPConnection::StatusCode200, nodesDocument.toJson(), qPrintable(JSON_MIME_TYPE));
        }
    } else if (connection->requestOperation() == QNetworkAccessManager::PostOperation) {
        if (path == URI_ASSIGNMENT) {
            // this is a script upload - ask the HTTPConnection to parse the form data
            QList<FormData> formData = connection->parseFormData();
            
            // create an assignment for this saved script, for now make it local only
            Assignment* scriptAssignment = new Assignment(Assignment::CreateCommand,
                                                          Assignment::AgentType,
                                                          NULL,
                                                          Assignment::LocalLocation);
            
            // check how many instances of this assignment the user wants by checking the ASSIGNMENT-INSTANCES header
            const QString ASSIGNMENT_INSTANCES_HEADER = "ASSIGNMENT-INSTANCES";
            
            QByteArray assignmentInstancesValue = connection->requestHeaders().value(ASSIGNMENT_INSTANCES_HEADER.toLocal8Bit());
            if (!assignmentInstancesValue.isEmpty()) {
                // the user has requested a specific number of instances
                // so set that on the created assignment
                int numInstances = assignmentInstancesValue.toInt();
                if (numInstances > 0) {
                    qDebug() << numInstances;
                    scriptAssignment->setNumberOfInstances(numInstances);
                }
            }
            
            const char ASSIGNMENT_SCRIPT_HOST_LOCATION[] = "resources/web/assignment";
            
            QString newPath(ASSIGNMENT_SCRIPT_HOST_LOCATION);
            newPath += "/";
            // append the UUID for this script as the new filename, remove the curly braces
            newPath += uuidStringWithoutCurlyBraces(scriptAssignment->getUUID());
            
            // create a file with the GUID of the assignment in the script host locaiton
            QFile scriptFile(newPath);
            scriptFile.open(QIODevice::WriteOnly);
            scriptFile.write(formData[0].second);
            
            qDebug("Saved a script for assignment at %s", qPrintable(newPath));
            
            // respond with a 200 code for successful upload
            connection->respond(HTTPConnection::StatusCode200);
            
            // add the script assigment to the assignment queue
            // lock the assignment queue mutex since we're operating on a different thread than DS main
            _assignmentQueueMutex.lock();
            _assignmentQueue.push_back(scriptAssignment);
            _assignmentQueueMutex.unlock();
        }
    } else if (connection->requestOperation() == QNetworkAccessManager::DeleteOperation) {
        if (path.startsWith(URI_NODE)) {
            // this is a request to DELETE a node by UUID
            
            // pull the UUID from the url
            QUuid deleteUUID = QUuid(path.mid(URI_NODE.size() + sizeof('/')));
            
            if (!deleteUUID.isNull()) {
                SharedNodePointer nodeToKill = NodeList::getInstance()->nodeWithUUID(deleteUUID);
                
                if (nodeToKill) {
                    // start with a 200 response
                    connection->respond(HTTPConnection::StatusCode200);
                    
                    // we have a valid UUID and node - kill the node that has this assignment
                    QMetaObject::invokeMethod(NodeList::getInstance(), "killNodeWithUUID", Q_ARG(const QUuid&, deleteUUID));
                    
                    // successfully processed request
                    return true;
                }
            }
            
            // bad request, couldn't pull a node ID
            connection->respond(HTTPConnection::StatusCode400);
            
            return true;
        }
    }
    
    // didn't process the request, let the HTTPManager try and handle
    return false;
}

void DomainServer::addReleasedAssignmentBackToQueue(Assignment* releasedAssignment) {
    qDebug() << "Adding assignment" << *releasedAssignment << " back to queue.";

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

void DomainServer::nodeKilled(SharedNodePointer node) {
    // if this node has linked data it was from an assignment
    if (node->getLinkedData()) {
        Assignment* nodeAssignment = (Assignment*) node->getLinkedData();

        addReleasedAssignmentBackToQueue(nodeAssignment);
    }
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
        qDebug("Reading Voxel Server Configuration.");
        qDebug() << "config: " << _voxelServerConfig;

        QString multiConfig((const char*) _voxelServerConfig);
        QStringList multiConfigList = multiConfig.split(";");

        // read each config to a payload for a VS assignment
        for (int i = 0; i < multiConfigList.size(); i++) {
            QString config = multiConfigList.at(i);

            qDebug("config[%d]=%s", i, config.toLocal8Bit().constData());

            // Now, parse the config to check for a pool
            const char ASSIGNMENT_CONFIG_POOL_OPTION[] = "--pool";
            QString assignmentPool;

            int poolIndex = config.indexOf(ASSIGNMENT_CONFIG_POOL_OPTION);

            if (poolIndex >= 0) {
                int spaceBeforePoolIndex = config.indexOf(' ', poolIndex);
                int spaceAfterPoolIndex = config.indexOf(' ', spaceBeforePoolIndex);

                assignmentPool = config.mid(spaceBeforePoolIndex + 1, spaceAfterPoolIndex);
                qDebug() << "The pool for this voxel-assignment is" << assignmentPool;
            }

            Assignment voxelServerAssignment(Assignment::CreateCommand,
                                             Assignment::VoxelServerType,
                                             (assignmentPool.isEmpty() ? NULL : assignmentPool.toLocal8Bit().constData()));
            
            voxelServerAssignment.setPayload(config.toUtf8());

            freshStaticAssignments[numFreshStaticAssignments++] = voxelServerAssignment;
        }
    } else {
        Assignment rootVoxelServerAssignment(Assignment::CreateCommand, Assignment::VoxelServerType);
        freshStaticAssignments[numFreshStaticAssignments++] = rootVoxelServerAssignment;
    }

    // Handle Domain/Particle Server configuration command line arguments
    if (_particleServerConfig) {
        qDebug("Reading Particle Server Configuration.");
        qDebug() << "config: " << _particleServerConfig;

        QString multiConfig((const char*) _particleServerConfig);
        QStringList multiConfigList = multiConfig.split(";");

        // read each config to a payload for a VS assignment
        for (int i = 0; i < multiConfigList.size(); i++) {
            QString config = multiConfigList.at(i);

            qDebug("config[%d]=%s", i, config.toLocal8Bit().constData());

            // Now, parse the config to check for a pool
            const char ASSIGNMENT_CONFIG_POOL_OPTION[] = "--pool";
            QString assignmentPool;

            int poolIndex = config.indexOf(ASSIGNMENT_CONFIG_POOL_OPTION);

            if (poolIndex >= 0) {
                int spaceBeforePoolIndex = config.indexOf(' ', poolIndex);
                int spaceAfterPoolIndex = config.indexOf(' ', spaceBeforePoolIndex);

                assignmentPool = config.mid(spaceBeforePoolIndex + 1, spaceAfterPoolIndex);
                qDebug() << "The pool for this particle-assignment is" << assignmentPool;
            }

            Assignment particleServerAssignment(Assignment::CreateCommand,
                                             Assignment::ParticleServerType,
                                             (assignmentPool.isEmpty() ? NULL : assignmentPool.toLocal8Bit().constData()));
            
            particleServerAssignment.setPayload(config.toLocal8Bit());

            freshStaticAssignments[numFreshStaticAssignments++] = particleServerAssignment;
        }
    } else {
        Assignment rootParticleServerAssignment(Assignment::CreateCommand, Assignment::ParticleServerType);
        freshStaticAssignments[numFreshStaticAssignments++] = rootParticleServerAssignment;
    }

    // handle metavoxel configuration command line argument
    Assignment& metavoxelAssignment = (freshStaticAssignments[numFreshStaticAssignments++] =
        Assignment(Assignment::CreateCommand, Assignment::MetavoxelServerType));
    if (_metavoxelServerConfig) {
        metavoxelAssignment.setPayload(QByteArray(_metavoxelServerConfig));
    }

    qDebug() << "Adding" << numFreshStaticAssignments << "static assignments to fresh file.";

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
        bool nietherHasPool = (*assignment)->getPool().isEmpty() && requestAssignment.getPool().isEmpty();
        bool assignmentPoolsMatch = (*assignment)->getPool() == requestAssignment.getPool();

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

    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
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
        foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
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

            qDebug() << "Adding static assignment to queue -" << _staticAssignments[i];

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
