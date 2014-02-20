//
//  DomainServer.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <signal.h>

#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include <AccountManager.h>
#include <HTTPConnection.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "DomainServerNodeData.h"

#include "DomainServer.h"

const int RESTART_HOLD_TIME_MSECS = 5 * 1000;

const char* VOXEL_SERVER_CONFIG = "voxelServerConfig";
const char* PARTICLE_SERVER_CONFIG = "particleServerConfig";
const char* METAVOXEL_SERVER_CONFIG = "metavoxelServerConfig";

const quint16 DOMAIN_SERVER_HTTP_PORT = 8080;

DomainServer::DomainServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _HTTPManager(DOMAIN_SERVER_HTTP_PORT, QString("%1/resources/web/").arg(QCoreApplication::applicationDirPath()), this),
    _staticAssignmentHash(),
    _assignmentQueue(),
    _hasCompletedRestartHold(false),
    _nodeAuthenticationURL(DEFAULT_NODE_AUTH_URL)
{
    _argumentList = arguments();
    int argumentIndex = 0;
    
    // check if this domain server should use no authentication or a custom hostname for authentication
    const QString NO_AUTH_OPTION = "--noAuth";
    const QString CUSTOM_AUTH_OPTION = "--customAuth";
    if ((argumentIndex = _argumentList.indexOf(NO_AUTH_OPTION) != -1)) {
        _nodeAuthenticationURL = QUrl();
    } else if ((argumentIndex = _argumentList.indexOf(CUSTOM_AUTH_OPTION)) != -1)  {
        _nodeAuthenticationURL = QUrl(_argumentList.value(argumentIndex + 1));
    }
    
    if (!_nodeAuthenticationURL.isEmpty()) {
        // this domain-server will be using an authentication server, let's make sure we have a username/password
        QProcessEnvironment sysEnvironment = QProcessEnvironment::systemEnvironment();
        
        const QString DATA_SERVER_USERNAME_ENV = "DATA_SERVER_USERNAME";
        const QString DATA_SERVER_PASSWORD_ENV = "DATA_SERVER_PASSWORD";
        QString username = sysEnvironment.value(DATA_SERVER_USERNAME_ENV);
        QString password = sysEnvironment.value(DATA_SERVER_PASSWORD_ENV);
        
        if (!username.isEmpty() && !password.isEmpty()) {
            AccountManager& accountManager = AccountManager::getInstance();
            accountManager.setRootURL(_nodeAuthenticationURL);
            
            // TODO: failure case for not receiving a token
            accountManager.requestAccessToken(username, password);
            
            connect(&accountManager, &AccountManager::receivedAccessToken, this, &DomainServer::requestUUIDFromDataServer);
            
        } else {
            qDebug() << "Authentication was requested against" << qPrintable(_nodeAuthenticationURL.toString())
            << "but both or one of" << qPrintable(DATA_SERVER_USERNAME_ENV)
            << "/" << qPrintable(DATA_SERVER_PASSWORD_ENV) << "are not set. Qutting!";
            
            // bail out
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
            
            return;
        }
    } else {
        // auth is not requested for domain-server, setup NodeList and assignments now
        setupNodeListAndAssignments();
    }
}

void DomainServer::requestUUIDFromDataServer() {
    // this slot is fired when we get a valid access token from the data-server
    // now let's ask it to set us up with a UUID
    AccountManager::getInstance().authenticatedRequest("/api/v1/domains/create", QNetworkAccessManager::PostOperation,
                                                       this, SLOT(parseUUIDFromDataServer()));
}

void DomainServer::parseUUIDFromDataServer() {
    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());
    QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
    
    if (jsonResponse.object()["status"].toString() == "success") {
        // pull out the UUID the data-server is telling us to use, and complete our setup with it
        QUuid newSessionUUID = QUuid(jsonResponse.object()["data"].toObject()["uuid"].toString());
        setupNodeListAndAssignments(newSessionUUID);
    }
}

void DomainServer::setupNodeListAndAssignments(const QUuid& sessionUUID) {
    
    int argumentIndex = 0;
    
    const QString CUSTOM_PORT_OPTION = "-p";
    unsigned short domainServerPort = DEFAULT_DOMAIN_SERVER_PORT;
    
    if ((argumentIndex = _argumentList.indexOf(CUSTOM_PORT_OPTION)) != -1) {
        domainServerPort = _argumentList.value(argumentIndex + 1).toUShort();
    }
    
    QSet<Assignment::Type> parsedTypes(QSet<Assignment::Type>() << Assignment::AgentType);
    parseCommandLineTypeConfigs(_argumentList, parsedTypes);
    
    const QString CONFIG_FILE_OPTION = "--configFile";
    if ((argumentIndex = _argumentList.indexOf(CONFIG_FILE_OPTION)) != -1) {
        QString configFilePath = _argumentList.value(argumentIndex + 1);
        readConfigFile(configFilePath, parsedTypes);
    }
    
    populateDefaultStaticAssignmentsExcludingTypes(parsedTypes);
    
    NodeList* nodeList = NodeList::createInstance(NodeType::DomainServer, domainServerPort);
    
    // create a random UUID for this session for the domain-server
    nodeList->setSessionUUID(sessionUUID);
    
    connect(nodeList, &NodeList::nodeAdded, this, &DomainServer::nodeAdded);
    connect(nodeList, &NodeList::nodeKilled, this, &DomainServer::nodeKilled);
    
    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_USECS / 1000);
    
    connect(&nodeList->getNodeSocket(), SIGNAL(readyRead()), SLOT(readAvailableDatagrams()));
    
    // fire a single shot timer to add static assignments back into the queue after a restart
    QTimer::singleShot(RESTART_HOLD_TIME_MSECS, this, SLOT(addStaticAssignmentsBackToQueueAfterRestart()));
}

void DomainServer::parseCommandLineTypeConfigs(const QStringList& argumentList, QSet<Assignment::Type>& excludedTypes) {
    // check for configs from the command line, these take precedence
    const QString CONFIG_TYPE_OPTION = "--configType";
    int clConfigIndex = argumentList.indexOf(CONFIG_TYPE_OPTION);
    
    // enumerate all CL config overrides and parse them to files
    while (clConfigIndex != -1) {
        int clConfigType = argumentList.value(clConfigIndex + 1).toInt();
        if (clConfigType < Assignment::AllTypes && !excludedTypes.contains((Assignment::Type) clConfigIndex)) {
            Assignment::Type assignmentType = (Assignment::Type) clConfigType;
            createStaticAssignmentsForTypeGivenConfigString((Assignment::Type) assignmentType,
                                                            argumentList.value(clConfigIndex + 2));
            excludedTypes.insert(assignmentType);
        }
        
        clConfigIndex = argumentList.indexOf(CONFIG_TYPE_OPTION, clConfigIndex + 1);
    }
}

// Attempts to read configuration from specified path
// returns true on success, false otherwise
void DomainServer::readConfigFile(const QString& path, QSet<Assignment::Type>& excludedTypes) {
    if (path.isEmpty()) {
        // config file not specified
        return;
    }
    
    if (!QFile::exists(path)) {
        qWarning("Specified configuration file does not exist!");
        return;
    }
    
    QFile configFile(path);
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Can't open specified configuration file!");
        return;
    } else {
        qDebug() << "Reading configuration from" << path;
    }
    
    QTextStream configStream(&configFile);
    QByteArray configStringByteArray = configStream.readAll().toUtf8();
    QJsonObject configDocObject = QJsonDocument::fromJson(configStringByteArray).object();
    configFile.close();
    
    QSet<Assignment::Type> appendedExcludedTypes = excludedTypes;
    
    foreach (const QString& rootStringValue, configDocObject.keys()) {
        int possibleConfigType = rootStringValue.toInt();
        
        if (possibleConfigType < Assignment::AllTypes
            && !excludedTypes.contains((Assignment::Type) possibleConfigType)) {
            // this is an appropriate config type and isn't already in our excluded types
            // we are good to parse it
            Assignment::Type assignmentType = (Assignment::Type) possibleConfigType;
            QString configString = readServerAssignmentConfig(configDocObject, rootStringValue);
            createStaticAssignmentsForTypeGivenConfigString(assignmentType, configString);
            
            excludedTypes.insert(assignmentType);
        }
    }
}

// find assignment configurations on the specified node name and json object
// returns a string in the form of its equivalent cmd line params
QString DomainServer::readServerAssignmentConfig(const QJsonObject& jsonObject, const QString& nodeName) {
    QJsonArray nodeArray = jsonObject[nodeName].toArray();
    
    QStringList serverConfig;
    foreach (const QJsonValue& childValue, nodeArray) {
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

void DomainServer::addStaticAssignmentToAssignmentHash(Assignment* newAssignment) {
    qDebug() << "Inserting assignment" << *newAssignment << "to static assignment hash.";
    _staticAssignmentHash.insert(newAssignment->getUUID(), SharedAssignmentPointer(newAssignment));
}

void DomainServer::createStaticAssignmentsForTypeGivenConfigString(Assignment::Type type, const QString& configString) {
    // we have a string for config for this type
    qDebug() << "Parsing command line config for assignment type" << type;
    
    QStringList multiConfigList = configString.split(";", QString::SkipEmptyParts);
    
    const QString ASSIGNMENT_CONFIG_POOL_REGEX = "--pool\\s*([\\w-]+)";
    QRegExp poolRegex(ASSIGNMENT_CONFIG_POOL_REGEX);
    
    // read each config to a payload for this type of assignment
    for (int i = 0; i < multiConfigList.size(); i++) {
        QString config = multiConfigList.at(i);
        
        // check the config string for a pool
        QString assignmentPool;
        
        int poolIndex = poolRegex.indexIn(config);
        
        if (poolIndex != -1) {
            assignmentPool = poolRegex.cap(1);
            
            // remove the pool from the config string, the assigned node doesn't need it
            config.remove(poolIndex, poolRegex.matchedLength());
        }
        
        qDebug("Type %d config[%d] = %s", type, i, config.toLocal8Bit().constData());
        
        Assignment* configAssignment = new Assignment(Assignment::CreateCommand, type, assignmentPool);
        
        configAssignment->setPayload(config.toUtf8());
        
        addStaticAssignmentToAssignmentHash(configAssignment);
    }
}

void DomainServer::populateDefaultStaticAssignmentsExcludingTypes(const QSet<Assignment::Type>& excludedTypes) {
    // enumerate over all assignment types and see if we've already excluded it
    for (int defaultedType = Assignment::AudioMixerType; defaultedType != Assignment::AllTypes; defaultedType++) {
        if (!excludedTypes.contains((Assignment::Type) defaultedType)) {
            // type has not been set from a command line or config file config, use the default
            // by clearing whatever exists and writing a single default assignment with no payload
            Assignment* newAssignment = new Assignment(Assignment::CreateCommand, (Assignment::Type) defaultedType);
            addStaticAssignmentToAssignmentHash(newAssignment);
        }
    }
}

const NodeSet STATICALLY_ASSIGNED_NODES = NodeSet() << NodeType::AudioMixer
    << NodeType::AvatarMixer << NodeType::VoxelServer << NodeType::ParticleServer
    << NodeType::MetavoxelServer;

void DomainServer::readAvailableDatagrams() {
    NodeList* nodeList = NodeList::getInstance();

    HifiSockAddr senderSockAddr, nodePublicAddress, nodeLocalAddress;
    
    static QByteArray broadcastPacket = byteArrayWithPopluatedHeader(PacketTypeDomainList);
    static int numBroadcastPacketHeaderBytes = broadcastPacket.size();
    
    static QByteArray assignmentPacket = byteArrayWithPopluatedHeader(PacketTypeCreateAssignment);
    static int numAssignmentPacketHeaderBytes = assignmentPacket.size();
    
    QByteArray receivedPacket;
    NodeType_t nodeType;
   

    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        receivedPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(receivedPacket.data(), receivedPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            PacketType requestType = packetTypeForPacket(receivedPacket);
            if (requestType == PacketTypeDomainListRequest) {
                
                QUuid nodeUUID = uuidFromPacketHeader(receivedPacket);
                
                if (!_nodeAuthenticationURL.isEmpty() &&
                    (nodeUUID.isNull() || !nodeList->nodeWithUUID(nodeUUID))) {
                    // this is a node we do not recognize and we need authentication - ask them to do so
                    // by providing them the hostname they should authenticate with
                    QByteArray authenticationRequestPacket = byteArrayWithPopluatedHeader(PacketTypeDomainServerAuthRequest);
                    
                    QDataStream authPacketStream(&authenticationRequestPacket, QIODevice::Append);
                    authPacketStream << _nodeAuthenticationURL;
                    
                    qDebug() << "Asking node at" << senderSockAddr << "to authenticate.";
                    
                    // send the authentication request back to the node
                    nodeList->getNodeSocket().writeDatagram(authenticationRequestPacket,
                                                            senderSockAddr.getAddress(), senderSockAddr.getPort());
                    
                } else {
                    // this is an RFD or domain list request packet, and there is a match
                    QDataStream packetStream(receivedPacket);
                    packetStream.skipRawData(numBytesForPacketHeader(receivedPacket));
                    
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
                    
                    SharedAssignmentPointer matchingStaticAssignment;
                    
                    // check if this is a non-statically assigned node, a node that is assigned and checking in for the first time
                    // or a node that has already checked in and is continuing to report for duty
                    if (!STATICALLY_ASSIGNED_NODES.contains(nodeType)
                        || (matchingStaticAssignment = matchingStaticAssignmentForCheckIn(nodeUUID, nodeType))
                        || nodeList->getInstance()->nodeWithUUID(nodeUUID)) {
                        
                        if (nodeUUID.isNull()) {
                            // this is a check in from an unidentified node
                            // we need to generate a session UUID for this node
                            nodeUUID = QUuid::createUuid();
                        }
                        
                        SharedNodePointer checkInNode = nodeList->addOrUpdateNode(nodeUUID,
                                                                                  nodeType,
                                                                                  nodePublicAddress,
                                                                                  nodeLocalAddress);
                        
                        // resize our broadcast packet in preparation to set it up again
                        broadcastPacket.resize(numBroadcastPacketHeaderBytes);
                        
                        if (matchingStaticAssignment) {
                            // this was a newly added node with a matching static assignment
                            
                            // remove the matching assignment from the assignment queue so we don't take the next check in
                            // (if it exists)
                            if (_hasCompletedRestartHold) {
                                removeMatchingAssignmentFromQueue(matchingStaticAssignment);
                            }
                        }
                        
                        quint8 numInterestTypes = 0;
                        packetStream >> numInterestTypes;
                        
                        NodeType_t* nodeTypesOfInterest = reinterpret_cast<NodeType_t*>(receivedPacket.data()
                                                                                        + packetStream.device()->pos());
                        
                        // always send the node their own UUID back
                        QDataStream broadcastDataStream(&broadcastPacket, QIODevice::Append);
                        broadcastDataStream << checkInNode->getUUID();
                        
                        DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(checkInNode->getLinkedData());
                        
                        if (numInterestTypes > 0) {
                            // if the node has any interest types, send back those nodes as well
                            foreach (const SharedNodePointer& otherNode, nodeList->getNodeHash()) {
                                if (otherNode->getUUID() != nodeUUID &&
                                    memchr(nodeTypesOfInterest, otherNode->getType(), numInterestTypes)) {
                                    
                                    // don't send avatar nodes to other avatars, that will come from avatar mixer
                                    broadcastDataStream << *otherNode.data();
                                    
                                    // pack the secret that these two nodes will use to communicate with each other
                                    QUuid secretUUID = nodeData->getSessionSecretHash().value(otherNode->getUUID());
                                    if (secretUUID.isNull()) {
                                        // generate a new secret UUID these two nodes can use
                                        secretUUID = QUuid::createUuid();
                                        
                                        // set that on the current Node's sessionSecretHash
                                        nodeData->getSessionSecretHash().insert(otherNode->getUUID(), secretUUID);
                                        
                                        // set it on the other Node's sessionSecretHash
                                        reinterpret_cast<DomainServerNodeData*>(otherNode->getLinkedData())
                                        ->getSessionSecretHash().insert(nodeUUID, secretUUID);
                                        
                                    }
                                    
                                    broadcastDataStream << secretUUID;
                                }
                            }
                        }
                        
                        // update last receive to now
                        quint64 timeNow = usecTimestampNow();
                        checkInNode->setLastHeardMicrostamp(timeNow);
                        
                        // send the constructed list back to this node
                        nodeList->getNodeSocket().writeDatagram(broadcastPacket,
                                                                senderSockAddr.getAddress(), senderSockAddr.getPort());
                    }
                }                
            } else if (requestType == PacketTypeDomainConnectRequest) {
                QDataStream packetStream(receivedPacket);
                packetStream.skipRawData(numBytesForPacketHeader(receivedPacket));
                
                QByteArray registrationToken;
                packetStream >> registrationToken;
                
                qDebug() << "The token received is" << registrationToken.toHex();
            } else if (requestType == PacketTypeRequestAssignment) {
                
                // construct the requested assignment from the packet data
                Assignment requestAssignment(receivedPacket);
                
                qDebug() << "Received a request for assignment type" << requestAssignment.getType()
                    << "from" << senderSockAddr;
                
                SharedAssignmentPointer assignmentToDeploy = deployableAssignmentForRequest(requestAssignment);
                
                if (assignmentToDeploy) {
                    qDebug() << "Deploying assignment -" << *assignmentToDeploy.data() << "- to" << senderSockAddr;
                    
                    // give this assignment out, either the type matches or the requestor said they will take any
                    assignmentPacket.resize(numAssignmentPacketHeaderBytes);
                    
                    QDataStream assignmentStream(&assignmentPacket, QIODevice::Append);
                    
                    assignmentStream << *assignmentToDeploy.data();
                    
                    nodeList->getNodeSocket().writeDatagram(assignmentPacket,
                                                            senderSockAddr.getAddress(), senderSockAddr.getPort());
                } else {
                    qDebug() << "Unable to fulfill assignment request of type" << requestAssignment.getType()
                        << "from" << senderSockAddr;
                }
            }
        }
    }
}

QJsonObject DomainServer::jsonForSocket(const HifiSockAddr& socket) {
    QJsonObject socketJSON;

    socketJSON["ip"] = socket.getAddress().toString();
    socketJSON["port"] = ntohs(socket.getPort());

    return socketJSON;
}

const char JSON_KEY_TYPE[] = "type";
const char JSON_KEY_PUBLIC_SOCKET[] = "public";
const char JSON_KEY_LOCAL_SOCKET[] = "local";
const char JSON_KEY_POOL[] = "pool";

QJsonObject DomainServer::jsonObjectForNode(const SharedNodePointer& node) {
    QJsonObject nodeJson;

    // re-format the type name so it matches the target name
    QString nodeTypeName = NodeType::getNodeTypeName(node->getType());
    nodeTypeName = nodeTypeName.toLower();
    nodeTypeName.replace(' ', '-');

    // add the node type
    nodeJson[JSON_KEY_TYPE] = nodeTypeName;

    // add the node socket information
    nodeJson[JSON_KEY_PUBLIC_SOCKET] = jsonForSocket(node->getPublicSocket());
    nodeJson[JSON_KEY_LOCAL_SOCKET] = jsonForSocket(node->getLocalSocket());
    
    // if the node has pool information, add it
    SharedAssignmentPointer matchingAssignment = _staticAssignmentHash.value(node->getUUID());
    if (matchingAssignment) {
        nodeJson[JSON_KEY_POOL] = matchingAssignment->getPool();
    }
    
    return nodeJson;
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
                if (_staticAssignmentHash.value(node->getUUID())) {
                    // add the node using the UUID as the key
                    QString uuidString = uuidStringWithoutCurlyBraces(node->getUUID());
                    assignedNodesJSON[uuidString] = jsonObjectForNode(node);
                }
            }
            
            assignmentJSON["fulfilled"] = assignedNodesJSON;
            
            QJsonObject queuedAssignmentsJSON;
            
            // add the queued but unfilled assignments to the json
            foreach(const SharedAssignmentPointer& assignment, _assignmentQueue) {
                QJsonObject queuedAssignmentJSON;
                
                QString uuidString = uuidStringWithoutCurlyBraces(assignment->getUUID());
                queuedAssignmentJSON[JSON_KEY_TYPE] = QString(assignment->getTypeName());
                
                // if the assignment has a pool, add it
                if (!assignment->getPool().isEmpty()) {
                    queuedAssignmentJSON[JSON_KEY_POOL] = assignment->getPool();
                }
                
                // add this queued assignment to the JSON
                queuedAssignmentsJSON[uuidString] = queuedAssignmentJSON;
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
                nodesJSON[uuidString] = jsonObjectForNode(node);
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
            
            // create an assignment for this saved script
            Assignment* scriptAssignment = new Assignment(Assignment::CreateCommand, Assignment::AgentType);
            
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
            _assignmentQueue.enqueue(SharedAssignmentPointer(scriptAssignment));
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

void DomainServer::refreshStaticAssignmentAndAddToQueue(SharedAssignmentPointer& assignment) {
    QUuid oldUUID = assignment->getUUID();
    assignment->resetUUID();
    
    qDebug() << "Reset UUID for assignment -" << *assignment.data() << "- and added to queue. Old UUID was"
        << uuidStringWithoutCurlyBraces(oldUUID);
    
    // add the static assignment back under the right UUID, and to the queue
    _staticAssignmentHash.insert(assignment->getUUID(), assignment);
    
    _assignmentQueue.enqueue(assignment);
    
    // remove the old assignment from the _staticAssignmentHash
    // this must be done last so copies are created before the assignment passed by reference is killed
    _staticAssignmentHash.remove(oldUUID);
}

void DomainServer::nodeAdded(SharedNodePointer node) {
    // we don't use updateNodeWithData, so add the DomainServerNodeData to the node here
    node->setLinkedData(new DomainServerNodeData());
}

void DomainServer::nodeKilled(SharedNodePointer node) {
    // if this node's UUID matches a static assignment we need to throw it back in the assignment queue
    SharedAssignmentPointer matchedAssignment = _staticAssignmentHash.value(node->getUUID());
    
    if (matchedAssignment) {
        refreshStaticAssignmentAndAddToQueue(matchedAssignment);
    }
    
    // cleanup the connection secrets that we set up for this node (on the other nodes)
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());
    foreach (const QUuid& otherNodeSessionUUID, nodeData->getSessionSecretHash().keys()) {
        SharedNodePointer otherNode = NodeList::getInstance()->nodeWithUUID(otherNodeSessionUUID);
        if (otherNode) {
            reinterpret_cast<DomainServerNodeData*>(otherNode->getLinkedData())->getSessionSecretHash().remove(node->getUUID());
        }
    }
}

SharedAssignmentPointer DomainServer::matchingStaticAssignmentForCheckIn(const QUuid& checkInUUID, NodeType_t nodeType) {
    if (_hasCompletedRestartHold) {
        // look for a match in the assignment hash
        
        QQueue<SharedAssignmentPointer>::iterator i = _assignmentQueue.begin();
        
        while (i != _assignmentQueue.end()) {
            if (i->data()->getType() == Assignment::typeForNodeType(nodeType) && i->data()->getUUID() == checkInUUID) {
                return _assignmentQueue.takeAt(i - _assignmentQueue.begin());
            } else {
                ++i;
            }
        }
    } else {
        SharedAssignmentPointer matchingStaticAssignment = _staticAssignmentHash.value(checkInUUID);
        if (matchingStaticAssignment && matchingStaticAssignment->getType() == nodeType) {
            return matchingStaticAssignment;
        }
    }

    return SharedAssignmentPointer();
}

SharedAssignmentPointer DomainServer::deployableAssignmentForRequest(const Assignment& requestAssignment) {
    // this is an unassigned client talking to us directly for an assignment
    // go through our queue and see if there are any assignments to give out
    QQueue<SharedAssignmentPointer>::iterator sharedAssignment = _assignmentQueue.begin();

    while (sharedAssignment != _assignmentQueue.end()) {
        Assignment* assignment = sharedAssignment->data();
        bool requestIsAllTypes = requestAssignment.getType() == Assignment::AllTypes;
        bool assignmentTypesMatch = assignment->getType() == requestAssignment.getType();
        bool nietherHasPool = assignment->getPool().isEmpty() && requestAssignment.getPool().isEmpty();
        bool assignmentPoolsMatch = assignment->getPool() == requestAssignment.getPool();
        
        if ((requestIsAllTypes || assignmentTypesMatch) && (nietherHasPool || assignmentPoolsMatch)) {

            if (assignment->getType() == Assignment::AgentType) {
                // if there is more than one instance to send out, simply decrease the number of instances

                if (assignment->getNumberOfInstances() == 1) {
                    return _assignmentQueue.takeAt(sharedAssignment - _assignmentQueue.begin());
                } else {
                    assignment->decrementNumberOfInstances();
                    return *sharedAssignment;
                }

            } else {
                // remove the assignment from the queue
                SharedAssignmentPointer deployableAssignment = _assignmentQueue.takeAt(sharedAssignment
                                                                                       - _assignmentQueue.begin());

                // until we get a check-in from that GUID
                // put assignment back in queue but stick it at the back so the others have a chance to go out
                _assignmentQueue.enqueue(deployableAssignment);
                
                // stop looping, we've handed out an assignment
                return deployableAssignment;
            }
        } else {
            // push forward the iterator to check the next assignment
            ++sharedAssignment;
        }
    }
    
    return SharedAssignmentPointer();
}

void DomainServer::removeMatchingAssignmentFromQueue(const SharedAssignmentPointer& removableAssignment) {
    QQueue<SharedAssignmentPointer>::iterator potentialMatchingAssignment = _assignmentQueue.begin();
    while (potentialMatchingAssignment != _assignmentQueue.end()) {
        if (potentialMatchingAssignment->data()->getUUID() == removableAssignment->getUUID()) {
            _assignmentQueue.erase(potentialMatchingAssignment);
            
            // we matched and removed an assignment, bail out
            break;
        } else {
            ++potentialMatchingAssignment;
        }
    }
}

void DomainServer::addStaticAssignmentsBackToQueueAfterRestart() {
    _hasCompletedRestartHold = true;

    // if the domain-server has just restarted,
    // check if there are static assignments that we need to throw into the assignment queue
    QHash<QUuid, SharedAssignmentPointer> staticHashCopy = _staticAssignmentHash;
    QHash<QUuid, SharedAssignmentPointer>::iterator staticAssignment = staticHashCopy.begin();
    while (staticAssignment != staticHashCopy.end()) {
        // add any of the un-matched static assignments to the queue
        bool foundMatchingAssignment = false;
        
        // enumerate the nodes and check if there is one with an attached assignment with matching UUID
        foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
            if (node->getUUID() == staticAssignment->data()->getUUID()) {
                foundMatchingAssignment = true;
            }
        }
        
        if (!foundMatchingAssignment) {
            // this assignment has not been fulfilled - reset the UUID and add it to the assignment queue
            refreshStaticAssignmentAndAddToQueue(*staticAssignment);
        }
        
        ++staticAssignment;
    }
}
