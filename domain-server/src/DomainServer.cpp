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
#include <QtCore/QTimer>

#include <AccountManager.h>
#include <HTTPConnection.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "DomainServerNodeData.h"

#include "DomainServer.h"

const quint16 DOMAIN_SERVER_HTTP_PORT = 8080;

DomainServer::DomainServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _HTTPManager(DOMAIN_SERVER_HTTP_PORT, QString("%1/resources/web/").arg(QCoreApplication::applicationDirPath()), this),
    _staticAssignmentHash(),
    _assignmentQueue(),
    _nodeAuthenticationURL(),
    _redeemedTokenResponses()
{
    setOrganizationName("High Fidelity");
    setOrganizationDomain("highfidelity.io");
    setApplicationName("domain-server");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    
    _argumentList = arguments();
    int argumentIndex = 0;
    
    // check if this domain server should use no authentication or a custom hostname for authentication
    const QString DEFAULT_AUTH_OPTION = "--defaultAuth";
    const QString CUSTOM_AUTH_OPTION = "--customAuth";
    if ((argumentIndex = _argumentList.indexOf(DEFAULT_AUTH_OPTION) != -1)) {
        _nodeAuthenticationURL = QUrl(DEFAULT_NODE_AUTH_URL);
    } else if ((argumentIndex = _argumentList.indexOf(CUSTOM_AUTH_OPTION)) != -1)  {
        _nodeAuthenticationURL = QUrl(_argumentList.value(argumentIndex + 1));
    }
    
    if (!_nodeAuthenticationURL.isEmpty()) {
        const QString DATA_SERVER_USERNAME_ENV = "HIFI_DS_USERNAME";
        const QString DATA_SERVER_PASSWORD_ENV = "HIFI_DS_PASSWORD";
        
        // this node will be using an authentication server, let's make sure we have a username/password
        QProcessEnvironment sysEnvironment = QProcessEnvironment::systemEnvironment();
        
        QString username = sysEnvironment.value(DATA_SERVER_USERNAME_ENV);
        QString password = sysEnvironment.value(DATA_SERVER_PASSWORD_ENV);
        
        AccountManager& accountManager = AccountManager::getInstance();
        accountManager.setAuthURL(_nodeAuthenticationURL);
        
        if (!username.isEmpty() && !password.isEmpty()) {
            
            connect(&accountManager, &AccountManager::loginComplete, this, &DomainServer::requestCreationFromDataServer);
            
            // ask the account manager to log us in from the env variables
            accountManager.requestAccessToken(username, password);
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

void DomainServer::requestCreationFromDataServer() {
    // this slot is fired when we get a valid access token from the data-server
    // now let's ask it to set us up with a UUID
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "processCreateResponseFromDataServer";
    
    AccountManager::getInstance().authenticatedRequest("/api/v1/domains/create",
                                                       QNetworkAccessManager::PostOperation,
                                                       callbackParams);
}

void DomainServer::processCreateResponseFromDataServer(const QJsonObject& jsonObject) {
    if (jsonObject["status"].toString() == "success") {
        // pull out the UUID the data-server is telling us to use, and complete our setup with it
        QUuid newSessionUUID = QUuid(jsonObject["data"].toObject()["uuid"].toString());
        setupNodeListAndAssignments(newSessionUUID);
    }
}

void DomainServer::processTokenRedeemResponse(const QJsonObject& jsonObject) {
    // pull out the registration token this is associated with
    QString registrationToken = jsonObject["data"].toObject()["registration_token"].toString();
    
    // if we have a registration token add it to our hash of redeemed token responses
    if (!registrationToken.isEmpty()) {
        qDebug() << "Redeemed registration token" << registrationToken;
        _redeemedTokenResponses.insert(registrationToken, jsonObject);
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
    
    // add whatever static assignments that have been parsed to the queue
    addStaticAssignmentsToQueue();
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

void DomainServer::requestAuthenticationFromPotentialNode(const HifiSockAddr& senderSockAddr) {
    // this is a node we do not recognize and we need authentication - ask them to do so
    // by providing them the hostname they should authenticate with
    QByteArray authenticationRequestPacket = byteArrayWithPopulatedHeader(PacketTypeDomainServerAuthRequest);
    
    QDataStream authPacketStream(&authenticationRequestPacket, QIODevice::Append);
    authPacketStream << _nodeAuthenticationURL;
    
    qDebug() << "Asking node at" << senderSockAddr << "to authenticate.";
    
    // send the authentication request back to the node
    NodeList::getInstance()->getNodeSocket().writeDatagram(authenticationRequestPacket,
                                                           senderSockAddr.getAddress(), senderSockAddr.getPort());
}

const NodeSet STATICALLY_ASSIGNED_NODES = NodeSet() << NodeType::AudioMixer
    << NodeType::AvatarMixer << NodeType::VoxelServer << NodeType::ParticleServer
    << NodeType::MetavoxelServer;


void DomainServer::addNodeToNodeListAndConfirmConnection(const QByteArray& packet, const HifiSockAddr& senderSockAddr,
                                                         const QJsonObject& authJsonObject) {

    NodeType_t nodeType;
    HifiSockAddr publicSockAddr, localSockAddr;
    
    int numPreInterestBytes = parseNodeDataFromByteArray(nodeType, publicSockAddr, localSockAddr, packet, senderSockAddr);
    
    QUuid assignmentUUID = uuidFromPacketHeader(packet);
    SharedAssignmentPointer matchingAssignment;
    
    if (!assignmentUUID.isNull() && (matchingAssignment = matchingStaticAssignmentForCheckIn(assignmentUUID, nodeType))
        && matchingAssignment) {
        // this is an assigned node, make sure the UUID sent is for an assignment we're actually trying to give out
        
        // remove the matching assignment from the assignment queue so we don't take the next check in
        // (if it exists)
        removeMatchingAssignmentFromQueue(matchingAssignment);
    } else {
        assignmentUUID = QUuid();
    }
    
    // create a new session UUID for this node
    QUuid nodeUUID = QUuid::createUuid();
    
    SharedNodePointer newNode = NodeList::getInstance()->addOrUpdateNode(nodeUUID, nodeType, publicSockAddr, localSockAddr);
    
    // when the newNode is created the linked data is also created, if this was a static assignment set the UUID
    reinterpret_cast<DomainServerNodeData*>(newNode->getLinkedData())->setStaticAssignmentUUID(assignmentUUID);
    
    if (!authJsonObject.isEmpty()) {
        // pull the connection secret from the authJsonObject and set it as the connection secret for this node
        QUuid connectionSecret(authJsonObject["data"].toObject()["connection_secret"].toString());
        newNode->setConnectionSecret(connectionSecret);
    }
    
    // reply back to the user with a PacketTypeDomainList
    sendDomainListToNode(newNode, senderSockAddr, nodeInterestListFromPacket(packet, numPreInterestBytes));
}

int DomainServer::parseNodeDataFromByteArray(NodeType_t& nodeType, HifiSockAddr& publicSockAddr,
                                              HifiSockAddr& localSockAddr, const QByteArray& packet,
                                              const HifiSockAddr& senderSockAddr) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    if (packetTypeForPacket(packet) == PacketTypeDomainConnectRequest) {
        // we need to skip a quint8 that indicates if there is a registration token
        // and potentially the registration token itself
        quint8 hasRegistrationToken;
        packetStream >> hasRegistrationToken;
        
        if (hasRegistrationToken) {
            QByteArray registrationToken;
            packetStream >> registrationToken;
        }
    }
    
    packetStream >> nodeType;
    packetStream >> publicSockAddr >> localSockAddr;
    
    if (publicSockAddr.getAddress().isNull()) {
        // this node wants to use us its STUN server
        // so set the node public address to whatever we perceive the public address to be
        
        // if the sender is on our box then leave its public address to 0 so that
        // other users attempt to reach it on the same address they have for the domain-server
        if (senderSockAddr.getAddress().isLoopback()) {
            publicSockAddr.setAddress(QHostAddress());
        } else {
            publicSockAddr.setAddress(senderSockAddr.getAddress());
        }
    }
    
    return packetStream.device()->pos();
}

NodeSet DomainServer::nodeInterestListFromPacket(const QByteArray& packet, int numPreceedingBytes) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numPreceedingBytes);
    
    quint8 numInterestTypes = 0;
    packetStream >> numInterestTypes;
    
    quint8 nodeType;
    NodeSet nodeInterestSet;
    
    for (int i = 0; i < numInterestTypes; i++) {
        packetStream >> nodeType;
        nodeInterestSet.insert((NodeType_t) nodeType);
    }
    
    return nodeInterestSet;
}

void DomainServer::sendDomainListToNode(const SharedNodePointer& node, const HifiSockAddr &senderSockAddr,
                                        const NodeSet& nodeInterestList) {
    
    QByteArray broadcastPacket = byteArrayWithPopulatedHeader(PacketTypeDomainList);
    
    // always send the node their own UUID back
    QDataStream broadcastDataStream(&broadcastPacket, QIODevice::Append);
    broadcastDataStream << node->getUUID();
    
    int numBroadcastPacketLeadBytes = broadcastDataStream.device()->pos();
    
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());
    
    NodeList* nodeList = NodeList::getInstance();
    
    
    if (nodeInterestList.size() > 0) {
        // if the node has any interest types, send back those nodes as well
        foreach (const SharedNodePointer& otherNode, nodeList->getNodeHash()) {
            
            // reset our nodeByteArray and nodeDataStream
            QByteArray nodeByteArray;
            QDataStream nodeDataStream(&nodeByteArray, QIODevice::Append);
            
            if (otherNode->getUUID() != node->getUUID() && nodeInterestList.contains(otherNode->getType())) {
                
                // don't send avatar nodes to other avatars, that will come from avatar mixer
                nodeDataStream << *otherNode.data();
                
                // pack the secret that these two nodes will use to communicate with each other
                QUuid secretUUID = nodeData->getSessionSecretHash().value(otherNode->getUUID());
                if (secretUUID.isNull()) {
                    // generate a new secret UUID these two nodes can use
                    secretUUID = QUuid::createUuid();
                    
                    // set that on the current Node's sessionSecretHash
                    nodeData->getSessionSecretHash().insert(otherNode->getUUID(), secretUUID);
                    
                    // set it on the other Node's sessionSecretHash
                    reinterpret_cast<DomainServerNodeData*>(otherNode->getLinkedData())
                        ->getSessionSecretHash().insert(node->getUUID(), secretUUID);
                    
                }
                
                nodeDataStream << secretUUID;
                
                if (broadcastPacket.size() +  nodeByteArray.size() > MAX_PACKET_SIZE) {
                    // we need to break here and start a new packet
                    // so send the current one
                    
                    nodeList->writeDatagram(broadcastPacket, node, senderSockAddr);
                    
                    // reset the broadcastPacket structure
                    broadcastPacket.resize(numBroadcastPacketLeadBytes);
                    broadcastDataStream.device()->seek(numBroadcastPacketLeadBytes);
                }
                
                // append the nodeByteArray to the current state of broadcastDataStream
                broadcastPacket.append(nodeByteArray);
            }
        }
    }
    
    // always write the last broadcastPacket
    nodeList->writeDatagram(broadcastPacket, node, senderSockAddr);
}

void DomainServer::readAvailableDatagrams() {
    NodeList* nodeList = NodeList::getInstance();
    AccountManager& accountManager = AccountManager::getInstance();

    HifiSockAddr senderSockAddr;
    
    static QByteArray assignmentPacket = byteArrayWithPopulatedHeader(PacketTypeCreateAssignment);
    static int numAssignmentPacketHeaderBytes = assignmentPacket.size();
    
    QByteArray receivedPacket;

    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        receivedPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(receivedPacket.data(), receivedPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            PacketType requestType = packetTypeForPacket(receivedPacket);
            
            if (requestType == PacketTypeDomainConnectRequest) {
                QDataStream packetStream(receivedPacket);
                packetStream.skipRawData(numBytesForPacketHeader(receivedPacket));
                
                quint8 hasRegistrationToken;
                packetStream >> hasRegistrationToken;
                
                if (requiresAuthentication() && !hasRegistrationToken) {
                    // we need authentication and this node did not give us a registration token - tell it to auth
                    requestAuthenticationFromPotentialNode(senderSockAddr);
                } else if (requiresAuthentication()) {
                    QByteArray registrationToken;
                    packetStream >> registrationToken;
                    
                    QString registrationTokenString(registrationToken.toHex());
                    QJsonObject jsonForRedeemedToken = _redeemedTokenResponses.value(registrationTokenString);
                    
                    // check if we have redeemed this token and are ready to check the node in
                    if (jsonForRedeemedToken.isEmpty()) {
                        // make a request against the data-server to get information required to connect to this node
                        JSONCallbackParameters tokenCallbackParams;
                        tokenCallbackParams.jsonCallbackReceiver = this;
                        tokenCallbackParams.jsonCallbackMethod = "processTokenRedeemResponse";
                        
                        QString redeemURLString = QString("/api/v1/nodes/redeem/%1.json").arg(registrationTokenString);
                        accountManager.authenticatedRequest(redeemURLString, QNetworkAccessManager::GetOperation,
                                                            tokenCallbackParams);
                    } else if (jsonForRedeemedToken["status"].toString() != "success") {
                        // we redeemed the token, but it was invalid - get the node to get another
                        requestAuthenticationFromPotentialNode(senderSockAddr);
                    } else {
                        // we've redeemed the token for this node and are ready to start communicating with it
                        // add the node to our NodeList
                        addNodeToNodeListAndConfirmConnection(receivedPacket, senderSockAddr, jsonForRedeemedToken);
                    }
                    
                    // if it exists, remove this response from the in-memory hash
                    _redeemedTokenResponses.remove(registrationTokenString);
                    
                } else {
                    // we don't require authentication - add this node to our NodeList
                    // and send back session UUID right away
                    addNodeToNodeListAndConfirmConnection(receivedPacket, senderSockAddr);
                }
                
            } else if (requestType == PacketTypeDomainListRequest) {
                QUuid nodeUUID = uuidFromPacketHeader(receivedPacket);
                NodeType_t throwawayNodeType;
                HifiSockAddr nodePublicAddress, nodeLocalAddress;
                
                int numNodeInfoBytes = parseNodeDataFromByteArray(throwawayNodeType, nodePublicAddress, nodeLocalAddress,
                                                                  receivedPacket, senderSockAddr);
                
                SharedNodePointer checkInNode = nodeList->updateSocketsForNode(nodeUUID, nodePublicAddress, nodeLocalAddress);
            
                // update last receive to now
                quint64 timeNow = usecTimestampNow();
                checkInNode->setLastHeardMicrostamp(timeNow);
                
                
                sendDomainListToNode(checkInNode, senderSockAddr, nodeInterestListFromPacket(receivedPacket, numNodeInfoBytes));
                
            } else if (requestType == PacketTypeRequestAssignment) {
                
                // construct the requested assignment from the packet data
                Assignment requestAssignment(receivedPacket);
                
                // Suppress these for Assignment::AgentType to once per 5 seconds
                static quint64 lastNoisyMessage = usecTimestampNow();
                quint64 timeNow = usecTimestampNow();
                const quint64 NOISY_TIME_ELAPSED = 5 * USECS_PER_SECOND;
                bool noisyMessage = false;
                if (requestAssignment.getType() != Assignment::AgentType || (timeNow - lastNoisyMessage) > NOISY_TIME_ELAPSED) {
                    qDebug() << "Received a request for assignment type" << requestAssignment.getType()
                        << "from" << senderSockAddr;
                    noisyMessage = true;
                }
                
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
                    if (requestAssignment.getType() != Assignment::AgentType || (timeNow - lastNoisyMessage) > NOISY_TIME_ELAPSED) {
                        qDebug() << "Unable to fulfill assignment request of type" << requestAssignment.getType()
                            << "from" << senderSockAddr;
                        noisyMessage = true;
                    }
                }
                
                if (noisyMessage) {
                    lastNoisyMessage = timeNow;
                }
            } else if (requestType == PacketTypeNodeJsonStats) {
                SharedNodePointer matchingNode = nodeList->sendingNodeForPacket(receivedPacket);
                if (matchingNode) {
                    reinterpret_cast<DomainServerNodeData*>(matchingNode->getLinkedData())->parseJSONStatsPacket(receivedPacket);
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

const char JSON_KEY_UUID[] = "uuid";
const char JSON_KEY_TYPE[] = "type";
const char JSON_KEY_PUBLIC_SOCKET[] = "public";
const char JSON_KEY_LOCAL_SOCKET[] = "local";
const char JSON_KEY_POOL[] = "pool";
const char JSON_KEY_WAKE_TIMESTAMP[] = "wake_timestamp";

QJsonObject DomainServer::jsonObjectForNode(const SharedNodePointer& node) {
    QJsonObject nodeJson;

    // re-format the type name so it matches the target name
    QString nodeTypeName = NodeType::getNodeTypeName(node->getType());
    nodeTypeName = nodeTypeName.toLower();
    nodeTypeName.replace(' ', '-');

    // add the node UUID
    nodeJson[JSON_KEY_UUID] = uuidStringWithoutCurlyBraces(node->getUUID());
    
    // add the node type
    nodeJson[JSON_KEY_TYPE] = nodeTypeName;

    // add the node socket information
    nodeJson[JSON_KEY_PUBLIC_SOCKET] = jsonForSocket(node->getPublicSocket());
    nodeJson[JSON_KEY_LOCAL_SOCKET] = jsonForSocket(node->getLocalSocket());
    
    // add the node uptime in our list
    nodeJson[JSON_KEY_WAKE_TIMESTAMP] = QString::number(node->getWakeTimestamp());
    
    // if the node has pool information, add it
    SharedAssignmentPointer matchingAssignment = _staticAssignmentHash.value(node->getUUID());
    if (matchingAssignment) {
        nodeJson[JSON_KEY_POOL] = matchingAssignment->getPool();
    }
    
    return nodeJson;
}

bool DomainServer::handleHTTPRequest(HTTPConnection* connection, const QUrl& url) {
    const QString JSON_MIME_TYPE = "application/json";
    
    const QString URI_ASSIGNMENT = "/assignment";
    const QString URI_NODES = "/nodes";
    
    const QString UUID_REGEX_STRING = "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}";
    
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation) {
        if (url.path() == "/assignments.json") {
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
        } else if (url.path() == QString("%1.json").arg(URI_NODES)) {
            // setup the JSON
            QJsonObject rootJSON;
            QJsonArray nodesJSONArray;
            
            // enumerate the NodeList to find the assigned nodes
            NodeList* nodeList = NodeList::getInstance();
            
            foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
                // add the node using the UUID as the key
                nodesJSONArray.append(jsonObjectForNode(node));
            }
            
            rootJSON["nodes"] = nodesJSONArray;
            
            // print out the created JSON
            QJsonDocument nodesDocument(rootJSON);
            
            // send the response
            connection->respond(HTTPConnection::StatusCode200, nodesDocument.toJson(), qPrintable(JSON_MIME_TYPE));
            
            return true;
        } else {
            const QString NODE_JSON_REGEX_STRING = QString("\\%1\\/(%2).json\\/?$").arg(URI_NODES).arg(UUID_REGEX_STRING);
            QRegExp nodeShowRegex(NODE_JSON_REGEX_STRING);
            
            if (nodeShowRegex.indexIn(url.path()) != -1) {
                QUuid matchingUUID = QUuid(nodeShowRegex.cap(1));
                
                // see if we have a node that matches this ID
                SharedNodePointer matchingNode = NodeList::getInstance()->nodeWithUUID(matchingUUID);
                if (matchingNode) {
                    // create a QJsonDocument with the stats QJsonObject
                    QJsonObject statsObject =
                        reinterpret_cast<DomainServerNodeData*>(matchingNode->getLinkedData())->getStatsJSONObject();
                    
                    // add the node type to the JSON data for output purposes
                    statsObject["node_type"] = NodeType::getNodeTypeName(matchingNode->getType()).toLower().replace(' ', '-');
                    
                    QJsonDocument statsDocument(statsObject);
                    
                    // send the response
                    connection->respond(HTTPConnection::StatusCode200, statsDocument.toJson(), qPrintable(JSON_MIME_TYPE));
                    
                    // tell the caller we processed the request
                    return true;
                }
            }
        }
    } else if (connection->requestOperation() == QNetworkAccessManager::PostOperation) {
        if (url.path() == URI_ASSIGNMENT) {
            // this is a script upload - ask the HTTPConnection to parse the form data
            QList<FormData> formData = connection->parseFormData();
            
            // check how many instances of this assignment the user wants by checking the ASSIGNMENT-INSTANCES header
            const QString ASSIGNMENT_INSTANCES_HEADER = "ASSIGNMENT-INSTANCES";
            
            QByteArray assignmentInstancesValue = connection->requestHeaders().value(ASSIGNMENT_INSTANCES_HEADER.toLocal8Bit());
            
            int numInstances = 1;
            
            if (!assignmentInstancesValue.isEmpty()) {
                // the user has requested a specific number of instances
                // so set that on the created assignment
                
                numInstances = assignmentInstancesValue.toInt();
            }

            const char ASSIGNMENT_SCRIPT_HOST_LOCATION[] = "resources/web/assignment";
            
            for (int i = 0; i < numInstances; i++) {
                
                // create an assignment for this saved script
                Assignment* scriptAssignment = new Assignment(Assignment::CreateCommand, Assignment::AgentType);
                
                QString newPath(ASSIGNMENT_SCRIPT_HOST_LOCATION);
                newPath += "/";
                // append the UUID for this script as the new filename, remove the curly braces
                newPath += uuidStringWithoutCurlyBraces(scriptAssignment->getUUID());
                
                // create a file with the GUID of the assignment in the script host locaiton
                QFile scriptFile(newPath);
                scriptFile.open(QIODevice::WriteOnly);
                scriptFile.write(formData[0].second);
                
                qDebug("Saved a script for assignment at %s", qPrintable(newPath));
                
                // add the script assigment to the assignment queue
                _assignmentQueue.enqueue(SharedAssignmentPointer(scriptAssignment));
            }
            
            // respond with a 200 code for successful upload
            connection->respond(HTTPConnection::StatusCode200);
            
            return true;
        }
    } else if (connection->requestOperation() == QNetworkAccessManager::DeleteOperation) {
        const QString ALL_NODE_DELETE_REGEX_STRING = QString("\\%1\\/?$").arg(URI_NODES);
        const QString NODE_DELETE_REGEX_STRING = QString("\\%1\\/(%2)\\/$").arg(URI_NODES).arg(UUID_REGEX_STRING);
        
        QRegExp allNodesDeleteRegex(ALL_NODE_DELETE_REGEX_STRING);
        QRegExp nodeDeleteRegex(NODE_DELETE_REGEX_STRING);
       
        if (nodeDeleteRegex.indexIn(url.path()) != -1) {
            // this is a request to DELETE one node by UUID
            
            // pull the captured string, if it exists
            QUuid deleteUUID = QUuid(nodeDeleteRegex.cap(1));
            
            SharedNodePointer nodeToKill = NodeList::getInstance()->nodeWithUUID(deleteUUID);
            
            if (nodeToKill) {
                // start with a 200 response
                connection->respond(HTTPConnection::StatusCode200);
                
                // we have a valid UUID and node - kill the node that has this assignment
                QMetaObject::invokeMethod(NodeList::getInstance(), "killNodeWithUUID", Q_ARG(const QUuid&, deleteUUID));
                
                // successfully processed request
                return true;
            }
            
            return true;
        } else if (allNodesDeleteRegex.indexIn(url.path()) != -1) {
            qDebug() << "Received request to kill all nodes.";
            NodeList::getInstance()->eraseAllNodes();
            
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
    
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());
    if (nodeData) {
        // if this node's UUID matches a static assignment we need to throw it back in the assignment queue
        if (!nodeData->getStaticAssignmentUUID().isNull()) {
            SharedAssignmentPointer matchedAssignment = _staticAssignmentHash.value(nodeData->getStaticAssignmentUUID());
            
            if (matchedAssignment) {
                refreshStaticAssignmentAndAddToQueue(matchedAssignment);
            }
        }
        
        // cleanup the connection secrets that we set up for this node (on the other nodes)
        foreach (const QUuid& otherNodeSessionUUID, nodeData->getSessionSecretHash().keys()) {
            SharedNodePointer otherNode = NodeList::getInstance()->nodeWithUUID(otherNodeSessionUUID);
            if (otherNode) {
                reinterpret_cast<DomainServerNodeData*>(otherNode->getLinkedData())->getSessionSecretHash().remove(node->getUUID());
            }
        }
    }
}

SharedAssignmentPointer DomainServer::matchingStaticAssignmentForCheckIn(const QUuid& checkInUUID, NodeType_t nodeType) {
    QQueue<SharedAssignmentPointer>::iterator i = _assignmentQueue.begin();
    
    while (i != _assignmentQueue.end()) {
        if (i->data()->getType() == Assignment::typeForNodeType(nodeType) && i->data()->getUUID() == checkInUUID) {
            return _assignmentQueue.takeAt(i - _assignmentQueue.begin());
        } else {
            ++i;
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
                return _assignmentQueue.takeAt(sharedAssignment - _assignmentQueue.begin());
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

void DomainServer::addStaticAssignmentsToQueue() {

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
