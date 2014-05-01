//
//  DomainServer.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtCore/QUrlQuery>

#include <gnutls/dtls.h>

#include <AccountManager.h>
#include <HifiConfigVariantMap.h>
#include <HTTPConnection.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "DomainServerNodeData.h"
#include "DummyDTLSSession.h"

#include "DomainServer.h"

DomainServer::DomainServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _httpManager(DOMAIN_SERVER_HTTP_PORT, QString("%1/resources/web/").arg(QCoreApplication::applicationDirPath()), this),
    _httpsManager(NULL),
    _staticAssignmentHash(),
    _assignmentQueue(),
    _isUsingDTLS(false),
    _x509Credentials(NULL),
    _dhParams(NULL),
    _priorityCache(NULL),
    _dtlsSessions(),
    _oauthProviderURL(),
    _oauthClientID(),
    _hostname()
{
    gnutls_global_init();
    
    setOrganizationName("High Fidelity");
    setOrganizationDomain("highfidelity.io");
    setApplicationName("domain-server");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    
    _argumentVariantMap = HifiConfigVariantMap::mergeCLParametersWithJSONConfig(arguments());
    
    if (optionallyReadX509KeyAndCertificate() && optionallySetupOAuth()) {
        // we either read a certificate and private key or were not passed one, good to load assignments
        // and set up the node list
        qDebug() << "Setting up LimitedNodeList and assignments.";
        setupNodeListAndAssignments();
        
        if (_isUsingDTLS) {
            LimitedNodeList* nodeList = LimitedNodeList::getInstance();
            
            // connect our socket to read datagrams received on the DTLS socket
            connect(&nodeList->getDTLSSocket(), &QUdpSocket::readyRead, this, &DomainServer::readAvailableDTLSDatagrams);
        }
    }
}

DomainServer::~DomainServer() {
    if (_x509Credentials) {
        gnutls_certificate_free_credentials(*_x509Credentials);
        gnutls_priority_deinit(*_priorityCache);
        gnutls_dh_params_deinit(*_dhParams);
        
        delete _x509Credentials;
        delete _priorityCache;
        delete _dhParams;
        delete _cookieKey;
    }
    gnutls_global_deinit();
}

bool DomainServer::optionallyReadX509KeyAndCertificate() {
    const QString X509_CERTIFICATE_OPTION = "cert";
    const QString X509_PRIVATE_KEY_OPTION = "key";
    const QString X509_KEY_PASSPHRASE_ENV = "DOMAIN_SERVER_KEY_PASSPHRASE";
    
    QString certPath = _argumentVariantMap.value(X509_CERTIFICATE_OPTION).toString();
    QString keyPath = _argumentVariantMap.value(X509_PRIVATE_KEY_OPTION).toString();
    
    if (!certPath.isEmpty() && !keyPath.isEmpty()) {
        // the user wants to use DTLS to encrypt communication with nodes
        // let's make sure we can load the key and certificate
        _x509Credentials = new gnutls_certificate_credentials_t;
        gnutls_certificate_allocate_credentials(_x509Credentials);
        
        QString keyPassphraseString = QProcessEnvironment::systemEnvironment().value(X509_KEY_PASSPHRASE_ENV);
        
        qDebug() << "Reading certificate file at" << certPath << "for DTLS.";
        qDebug() << "Reading key file at" << keyPath << "for DTLS.";
        
        int gnutlsReturn = gnutls_certificate_set_x509_key_file2(*_x509Credentials,
                                                                 certPath.toLocal8Bit().constData(),
                                                                 keyPath.toLocal8Bit().constData(),
                                                                 GNUTLS_X509_FMT_PEM,
                                                                 keyPassphraseString.toLocal8Bit().constData(),
                                                                 0);
        
        if (gnutlsReturn < 0) {
            qDebug() << "Unable to load certificate or key file." << "Error" << gnutlsReturn << "- domain-server will now quit.";
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
            return false;
        }
        
        qDebug() << "Successfully read certificate and private key.";
        
        // we need to also pass this certificate and private key to the HTTPS manager
        // this is used for Oauth callbacks when authorizing users against a data server
        
        QFile certFile(certPath);
        certFile.open(QIODevice::ReadOnly);
        
        QFile keyFile(keyPath);
        keyFile.open(QIODevice::ReadOnly);
        
        QSslCertificate sslCertificate(&certFile);
        QSslKey privateKey(&keyFile, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, keyPassphraseString.toUtf8());
        
        _httpsManager = new HTTPSManager(DOMAIN_SERVER_HTTPS_PORT, sslCertificate, privateKey, QString(), this, this);
        
        qDebug() << "TCP server listening for HTTPS connections on" << DOMAIN_SERVER_HTTPS_PORT;
        
    } else if (!certPath.isEmpty() || !keyPath.isEmpty()) {
        qDebug() << "Missing certificate or private key. domain-server will now quit.";
        QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
        return false;
    }
    
    return true;
}

bool DomainServer::optionallySetupOAuth() {
    const QString OAUTH_PROVIDER_URL_OPTION = "oauth-provider";
    const QString OAUTH_CLIENT_ID_OPTION = "oauth-client-id";
    const QString OAUTH_CLIENT_SECRET_ENV = "DOMAIN_SERVER_CLIENT_SECRET";
    const QString REDIRECT_HOSTNAME_OPTION = "hostname";
    
    _oauthProviderURL = QUrl(_argumentVariantMap.value(OAUTH_PROVIDER_URL_OPTION).toString());
    _oauthClientID = _argumentVariantMap.value(OAUTH_CLIENT_ID_OPTION).toString();
    _oauthClientSecret = QProcessEnvironment::systemEnvironment().value(OAUTH_CLIENT_SECRET_ENV);
    _hostname = _argumentVariantMap.value(REDIRECT_HOSTNAME_OPTION).toString();
    
    if (!_oauthProviderURL.isEmpty() || !_hostname.isEmpty() || !_oauthClientID.isEmpty()) {
        if (_oauthProviderURL.isEmpty()
            || _hostname.isEmpty()
            || _oauthClientID.isEmpty()
            || _oauthClientSecret.isEmpty()) {
            qDebug() << "Missing OAuth provider URL, hostname, client ID, or client secret. domain-server will now quit.";
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
            return false;
        } else {
            qDebug() << "OAuth will be used to identify clients using provider at" << _oauthProviderURL.toString();
            qDebug() << "OAuth Client ID is" << _oauthClientID;
        }
    }
    
    return true;
}

bool DomainServer::optionallySetupDTLS() {
    if (_x509Credentials) {
        qDebug() << "Generating Diffie-Hellman parameters.";
        
        // generate Diffie-Hellman parameters
        // When short bit length is used, it might be wise to regenerate parameters often.
        int dhBits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, GNUTLS_SEC_PARAM_LEGACY);
        
        _dhParams =  new gnutls_dh_params_t;
        gnutls_dh_params_init(_dhParams);
        gnutls_dh_params_generate2(*_dhParams, dhBits);
        
        qDebug() << "Successfully generated Diffie-Hellman parameters.";
        
        // set the D-H paramters on the X509 credentials
        gnutls_certificate_set_dh_params(*_x509Credentials, *_dhParams);
        
        // setup the key used for cookie verification
        _cookieKey = new gnutls_datum_t;
        gnutls_key_generate(_cookieKey, GNUTLS_COOKIE_KEY_SIZE);
        
        _priorityCache = new gnutls_priority_t;
        const char DTLS_PRIORITY_STRING[] = "PERFORMANCE:-VERS-TLS-ALL:+VERS-DTLS1.2:%SERVER_PRECEDENCE";
        gnutls_priority_init(_priorityCache, DTLS_PRIORITY_STRING, NULL);
        
        _isUsingDTLS = true;
        
        qDebug() << "Initial DTLS setup complete.";
    }
    
    return true;
}

void DomainServer::setupNodeListAndAssignments(const QUuid& sessionUUID) {
    
    const QString CUSTOM_PORT_OPTION = "port";
    unsigned short domainServerPort = DEFAULT_DOMAIN_SERVER_PORT;
    
    if (_argumentVariantMap.contains(CUSTOM_PORT_OPTION)) {
        domainServerPort = (unsigned short) _argumentVariantMap.value(CUSTOM_PORT_OPTION).toUInt();
    }
    
    unsigned short domainServerDTLSPort = 0;
    
    if (_isUsingDTLS) {
        domainServerDTLSPort = DEFAULT_DOMAIN_SERVER_DTLS_PORT;
        
        const QString CUSTOM_DTLS_PORT_OPTION = "dtls-port";
        
        if (_argumentVariantMap.contains(CUSTOM_DTLS_PORT_OPTION)) {
            domainServerDTLSPort = (unsigned short) _argumentVariantMap.value(CUSTOM_DTLS_PORT_OPTION).toUInt();
        }
    }
    
    QSet<Assignment::Type> parsedTypes;
    parseAssignmentConfigs(parsedTypes);
    
    populateDefaultStaticAssignmentsExcludingTypes(parsedTypes);
    
    LimitedNodeList* nodeList = LimitedNodeList::createInstance(domainServerPort, domainServerDTLSPort);
    
    connect(nodeList, &LimitedNodeList::nodeAdded, this, &DomainServer::nodeAdded);
    connect(nodeList, &LimitedNodeList::nodeKilled, this, &DomainServer::nodeKilled);
    
    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_MSECS);
    
    connect(&nodeList->getNodeSocket(), SIGNAL(readyRead()), SLOT(readAvailableDatagrams()));
    
    // add whatever static assignments that have been parsed to the queue
    addStaticAssignmentsToQueue();
}

void DomainServer::parseAssignmentConfigs(QSet<Assignment::Type>& excludedTypes) {
    // check for configs from the command line, these take precedence
    const QString ASSIGNMENT_CONFIG_REGEX_STRING = "config-([\\d]+)";
    QRegExp assignmentConfigRegex(ASSIGNMENT_CONFIG_REGEX_STRING);
    
    // scan for assignment config keys
    QStringList variantMapKeys = _argumentVariantMap.keys();
    int configIndex = variantMapKeys.indexOf(assignmentConfigRegex);
    
    while (configIndex != -1) {
        // figure out which assignment type this matches
        Assignment::Type assignmentType = (Assignment::Type) assignmentConfigRegex.cap(1).toInt();
        
        if (assignmentType < Assignment::AllTypes && !excludedTypes.contains(assignmentType)) {
            QVariant mapValue = _argumentVariantMap[variantMapKeys[configIndex]];
            QJsonArray assignmentArray;
            
            if (mapValue.type() == QVariant::String) {
                QJsonDocument deserializedDocument = QJsonDocument::fromJson(mapValue.toString().toUtf8());
                assignmentArray = deserializedDocument.array();
            } else {
                assignmentArray = mapValue.toJsonValue().toArray();
            }
            
            if (assignmentType != Assignment::AgentType) {
                createStaticAssignmentsForType(assignmentType, assignmentArray);
            } else {
                createScriptedAssignmentsFromArray(assignmentArray);
            }
            
            excludedTypes.insert(assignmentType);
        }
        
        configIndex = variantMapKeys.indexOf(assignmentConfigRegex, configIndex + 1);
    }
}

void DomainServer::addStaticAssignmentToAssignmentHash(Assignment* newAssignment) {
    qDebug() << "Inserting assignment" << *newAssignment << "to static assignment hash.";
    _staticAssignmentHash.insert(newAssignment->getUUID(), SharedAssignmentPointer(newAssignment));
}

void DomainServer::createScriptedAssignmentsFromArray(const QJsonArray &configArray) {    
    foreach(const QJsonValue& jsonValue, configArray) {
        if (jsonValue.isObject()) {
            QJsonObject jsonObject = jsonValue.toObject();
            
            // make sure we were passed a URL, otherwise this is an invalid scripted assignment
            const QString  ASSIGNMENT_URL_KEY = "url";
            QString assignmentURL = jsonObject[ASSIGNMENT_URL_KEY].toString();
            
            if (!assignmentURL.isEmpty()) {
                // check the json for a pool
                const QString ASSIGNMENT_POOL_KEY = "pool";
                QString assignmentPool = jsonObject[ASSIGNMENT_POOL_KEY].toString();
                
                // check for a number of instances, if not passed then default is 1
                const QString ASSIGNMENT_INSTANCES_KEY = "instances";
                int numInstances = jsonObject[ASSIGNMENT_INSTANCES_KEY].toInt();
                numInstances = (numInstances == 0 ? 1 : numInstances);
                
                for (int i = 0; i < numInstances; i++) {
                    // add a scripted assignment to the queue for this instance
                    Assignment* scriptAssignment = new Assignment(Assignment::CreateCommand,
                                                                  Assignment::AgentType,
                                                                  assignmentPool);
                    scriptAssignment->setPayload(assignmentURL.toUtf8());
                    
                    qDebug() << "Adding scripted assignment to queue -" << *scriptAssignment;
                    qDebug() << "URL for script is" << assignmentURL;
                    
                    // scripts passed on CL or via JSON are static - so they are added back to the queue if the node dies
                    _assignmentQueue.enqueue(SharedAssignmentPointer(scriptAssignment));
                }
            }
        }
    }
}

void DomainServer::createStaticAssignmentsForType(Assignment::Type type, const QJsonArray& configArray) {
    // we have a string for config for this type
    qDebug() << "Parsing config for assignment type" << type;
    
    int configCounter = 0;
    
    foreach(const QJsonValue& jsonValue, configArray) {
        if (jsonValue.isObject()) {
            QJsonObject jsonObject = jsonValue.toObject();
            
            // check the config string for a pool
            const QString ASSIGNMENT_POOL_KEY = "pool";
            QString assignmentPool;
            
            QJsonValue poolValue = jsonObject[ASSIGNMENT_POOL_KEY];
            if (!poolValue.isUndefined()) {
                assignmentPool = poolValue.toString();
                
                jsonObject.remove(ASSIGNMENT_POOL_KEY);
            }
            
            ++configCounter;
            qDebug() << "Type" << type << "config" << configCounter << "=" << jsonObject;
            
            Assignment* configAssignment = new Assignment(Assignment::CreateCommand, type, assignmentPool);
            
            // setup the payload as a semi-colon separated list of key = value
            QStringList payloadStringList;
            foreach(const QString& payloadKey, jsonObject.keys()) {
                QString dashes = payloadKey.size() == 1 ? "-" : "--";
                payloadStringList << QString("%1%2 %3").arg(dashes).arg(payloadKey).arg(jsonObject[payloadKey].toString());
            }
            
            configAssignment->setPayload(payloadStringList.join(' ').toUtf8());
            
            addStaticAssignmentToAssignmentHash(configAssignment);
        }
    }
}

void DomainServer::populateDefaultStaticAssignmentsExcludingTypes(const QSet<Assignment::Type>& excludedTypes) {
    // enumerate over all assignment types and see if we've already excluded it
    for (Assignment::Type defaultedType = Assignment::AudioMixerType;
         defaultedType != Assignment::AllTypes;
         defaultedType =  static_cast<Assignment::Type>(static_cast<int>(defaultedType) + 1)) {
        if (!excludedTypes.contains(defaultedType) && defaultedType != Assignment::AgentType) {
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

void DomainServer::addNodeToNodeListAndConfirmConnection(const QByteArray& packet, const HifiSockAddr& senderSockAddr) {

    NodeType_t nodeType;
    HifiSockAddr publicSockAddr, localSockAddr;
    
    int numPreInterestBytes = parseNodeDataFromByteArray(nodeType, publicSockAddr, localSockAddr, packet, senderSockAddr);
    
    QUuid assignmentUUID = uuidFromPacketHeader(packet);
    bool isStaticAssignment = _staticAssignmentHash.contains(assignmentUUID);
    SharedAssignmentPointer matchingAssignment = SharedAssignmentPointer();
    
    if (assignmentUUID.isNull() && !_oauthProviderURL.isEmpty()) {
        // we have an OAuth provider, ask this interface client to auth against it
        
        QByteArray oauthRequestByteArray = byteArrayWithPopulatedHeader(PacketTypeDomainOAuthRequest);
        QDataStream oauthRequestStream(&oauthRequestByteArray, QIODevice::Append);
        oauthRequestStream << oauthAuthorizationURL();
        
        // send this oauth request datagram back to the client
        LimitedNodeList::getInstance()->writeUnverifiedDatagram(oauthRequestByteArray, senderSockAddr);
    } else {
        if (isStaticAssignment) {
            // this is a static assignment, make sure the UUID sent is for an assignment we're actually trying to give out
            matchingAssignment = matchingQueuedAssignmentForCheckIn(assignmentUUID, nodeType);
            
            if (matchingAssignment) {
                // remove the matching assignment from the assignment queue so we don't take the next check in
                // (if it exists)
                removeMatchingAssignmentFromQueue(matchingAssignment);
            }
        }
        
        // make sure this was either not a static assignment or it was and we had a matching one in teh queue
        if ((!isStaticAssignment && !STATICALLY_ASSIGNED_NODES.contains(nodeType)) || (isStaticAssignment && matchingAssignment)) {
            // create a new session UUID for this node
            QUuid nodeUUID = QUuid::createUuid();
            
            SharedNodePointer newNode = LimitedNodeList::getInstance()->addOrUpdateNode(nodeUUID, nodeType,
                                                                                        publicSockAddr, localSockAddr);
            
            // when the newNode is created the linked data is also created
            // if this was a static assignment set the UUID, set the sendingSockAddr
            DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(newNode->getLinkedData());
            
            nodeData->setStaticAssignmentUUID(assignmentUUID);
            nodeData->setSendingSockAddr(senderSockAddr);
            
            // reply back to the user with a PacketTypeDomainList
            sendDomainListToNode(newNode, senderSockAddr, nodeInterestListFromPacket(packet, numPreInterestBytes));
        }
    }
}

QUrl DomainServer::oauthAuthorizationURL() {
    // for now these are all interface clients that have a GUI
    // so just send them back the full authorization URL
    QUrl authorizationURL = _oauthProviderURL;
    
    const QString OAUTH_AUTHORIZATION_PATH = "/oauth/authorize";
    authorizationURL.setPath(OAUTH_AUTHORIZATION_PATH);
    
    QUrlQuery authorizationQuery;
    
    const QString OAUTH_CLIENT_ID_QUERY_KEY = "client_id";
    authorizationQuery.addQueryItem(OAUTH_CLIENT_ID_QUERY_KEY, _oauthClientID);
    
    const QString OAUTH_RESPONSE_TYPE_QUERY_KEY = "response_type";
    const QString OAUTH_REPSONSE_TYPE_QUERY_VALUE = "code";
    authorizationQuery.addQueryItem(OAUTH_RESPONSE_TYPE_QUERY_KEY, OAUTH_REPSONSE_TYPE_QUERY_VALUE);
    
    const QString OAUTH_STATE_QUERY_KEY = "state";
    // create a new UUID that will be the state parameter for oauth authorization AND the new session UUID for that node
    authorizationQuery.addQueryItem(OAUTH_STATE_QUERY_KEY, uuidStringWithoutCurlyBraces(QUuid::createUuid()));
    
    QString redirectURL = QString("https://%1:%2/oauth").arg(_hostname).arg(_httpsManager->serverPort());
    
    const QString OAUTH_REDIRECT_URI_QUERY_KEY = "redirect_uri";
    authorizationQuery.addQueryItem(OAUTH_REDIRECT_URI_QUERY_KEY, redirectURL);
    
    authorizationURL.setQuery(authorizationQuery);
    
    return authorizationURL;
}

int DomainServer::parseNodeDataFromByteArray(NodeType_t& nodeType, HifiSockAddr& publicSockAddr,
                                              HifiSockAddr& localSockAddr, const QByteArray& packet,
                                              const HifiSockAddr& senderSockAddr) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
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
    
    LimitedNodeList* nodeList = LimitedNodeList::getInstance();
    
    if (nodeInterestList.size() > 0) {
        
        DTLSServerSession* dtlsSession = _isUsingDTLS ? _dtlsSessions[senderSockAddr] : NULL;
        int dataMTU = dtlsSession ? (int)gnutls_dtls_get_data_mtu(*dtlsSession->getGnuTLSSession()) : MAX_PACKET_SIZE;
        
        if (nodeData->isAuthenticated()) {
            // if this authenticated node has any interest types, send back those nodes as well
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
                    
                    if (broadcastPacket.size() +  nodeByteArray.size() > dataMTU) {
                        // we need to break here and start a new packet
                        // so send the current one
                        
                        if (!dtlsSession) {
                            nodeList->writeDatagram(broadcastPacket, node, senderSockAddr);
                        } else {
                            dtlsSession->writeDatagram(broadcastPacket);
                        }
                        
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
        if (!dtlsSession) {
            nodeList->writeDatagram(broadcastPacket, node, senderSockAddr);
        } else {
            dtlsSession->writeDatagram(broadcastPacket);
        }
    }
}

void DomainServer::readAvailableDatagrams() {
    LimitedNodeList* nodeList = LimitedNodeList::getInstance();

    HifiSockAddr senderSockAddr;
    QByteArray receivedPacket;
    
    static QByteArray assignmentPacket = byteArrayWithPopulatedHeader(PacketTypeCreateAssignment);
    static int numAssignmentPacketHeaderBytes = assignmentPacket.size();

    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        receivedPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(receivedPacket.data(), receivedPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        if (packetTypeForPacket(receivedPacket) == PacketTypeRequestAssignment
            && nodeList->packetVersionAndHashMatch(receivedPacket)) {

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
        } else if (!_isUsingDTLS) {
            // not using DTLS, process datagram normally
            processDatagram(receivedPacket, senderSockAddr);
        } else {
            // we're using DTLS, so tell the sender to get back to us using DTLS
            static QByteArray dtlsRequiredPacket = byteArrayWithPopulatedHeader(PacketTypeDomainServerRequireDTLS);
            static int numBytesDTLSHeader = numBytesForPacketHeaderGivenPacketType(PacketTypeDomainServerRequireDTLS);
            
            if (dtlsRequiredPacket.size() == numBytesDTLSHeader) {
                // pack the port that we accept DTLS traffic on
                unsigned short dtlsPort = nodeList->getDTLSSocket().localPort();
                dtlsRequiredPacket.replace(numBytesDTLSHeader, sizeof(dtlsPort), reinterpret_cast<const char*>(&dtlsPort));
            }

            nodeList->writeUnverifiedDatagram(dtlsRequiredPacket, senderSockAddr);
        }
    }
}

void DomainServer::readAvailableDTLSDatagrams() {
    LimitedNodeList* nodeList = LimitedNodeList::getInstance();
    
    QUdpSocket& dtlsSocket = nodeList->getDTLSSocket();
    
    static sockaddr senderSockAddr;
    static socklen_t sockAddrSize = sizeof(senderSockAddr);
    
    while (dtlsSocket.hasPendingDatagrams()) {
        // check if we have an active DTLS session for this sender
        QByteArray peekDatagram(dtlsSocket.pendingDatagramSize(), 0);
       
        recvfrom(dtlsSocket.socketDescriptor(), peekDatagram.data(), dtlsSocket.pendingDatagramSize(),
                 MSG_PEEK, &senderSockAddr, &sockAddrSize);
        
        HifiSockAddr senderHifiSockAddr(&senderSockAddr);
        DTLSServerSession* existingSession = _dtlsSessions.value(senderHifiSockAddr);
        
        if (existingSession) {
            if (!existingSession->completedHandshake()) {
                // check if we have completed handshake with this user
                int handshakeReturn = gnutls_handshake(*existingSession->getGnuTLSSession());
                
                if (handshakeReturn == 0) {
                    existingSession->setCompletedHandshake(true);
                } else if (gnutls_error_is_fatal(handshakeReturn)) {
                    // this was a fatal error handshaking, so remove this session
                    qDebug() << "Fatal error -" << gnutls_strerror(handshakeReturn) << "- during DTLS handshake with"
                        << senderHifiSockAddr;
                    _dtlsSessions.remove(senderHifiSockAddr);
                }
            } else {
                // pull the data from this user off the stack and process it
                int receivedBytes = gnutls_record_recv(*existingSession->getGnuTLSSession(),
                                                       peekDatagram.data(), peekDatagram.size());
                if (receivedBytes > 0) {
                    processDatagram(peekDatagram.left(receivedBytes), senderHifiSockAddr);
                } else if (gnutls_error_is_fatal(receivedBytes)) {
                    qDebug() << "Fatal error -" << gnutls_strerror(receivedBytes) << "- during DTLS handshake with"
                        << senderHifiSockAddr;
                }
            }
        } else {
            // first we verify the cookie
            // see http://gnutls.org/manual/html_node/DTLS-sessions.html for why this is required
            gnutls_dtls_prestate_st prestate;
            memset(&prestate, 0, sizeof(prestate));
            int cookieValid = gnutls_dtls_cookie_verify(_cookieKey, &senderSockAddr, sizeof(senderSockAddr),
                                                        peekDatagram.data(), peekDatagram.size(), &prestate);
            
            if (cookieValid < 0) {
                // the cookie sent by the client was not valid
                // send a valid one
                DummyDTLSSession tempServerSession(LimitedNodeList::getInstance()->getDTLSSocket(), senderHifiSockAddr);
                
                gnutls_dtls_cookie_send(_cookieKey, &senderSockAddr, sizeof(senderSockAddr), &prestate,
                                        &tempServerSession, DTLSSession::socketPush);
                
                // acutally pull the peeked data off the network stack so that it gets discarded
                dtlsSocket.readDatagram(peekDatagram.data(), peekDatagram.size());
            } else {
                // cookie valid but no existing session - set up a new session now
                DTLSServerSession* newServerSession = new DTLSServerSession(LimitedNodeList::getInstance()->getDTLSSocket(),
                                                                            senderHifiSockAddr);
                gnutls_session_t* gnutlsSession = newServerSession->getGnuTLSSession();
                
                gnutls_priority_set(*gnutlsSession, *_priorityCache);
                gnutls_credentials_set(*gnutlsSession, GNUTLS_CRD_CERTIFICATE, *_x509Credentials);
                gnutls_dtls_prestate_set(*gnutlsSession, &prestate);
                
                // handshake to begin the session
                gnutls_handshake(*gnutlsSession);
                
                qDebug() << "Beginning DTLS session with node at" << senderHifiSockAddr;
                _dtlsSessions[senderHifiSockAddr] = newServerSession;
            }
        }
    }
}

void DomainServer::processDatagram(const QByteArray& receivedPacket, const HifiSockAddr& senderSockAddr) {
    LimitedNodeList* nodeList = LimitedNodeList::getInstance();
    
    if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
        PacketType requestType = packetTypeForPacket(receivedPacket);
        
        if (requestType == PacketTypeDomainConnectRequest) {
            // add this node to our NodeList
            // and send back session UUID right away
            addNodeToNodeListAndConfirmConnection(receivedPacket, senderSockAddr);
        } else if (requestType == PacketTypeDomainListRequest) {
            QUuid nodeUUID = uuidFromPacketHeader(receivedPacket);
            
            if (!nodeUUID.isNull() && nodeList->nodeWithUUID(nodeUUID)) {
                NodeType_t throwawayNodeType;
                HifiSockAddr nodePublicAddress, nodeLocalAddress;
                
                int numNodeInfoBytes = parseNodeDataFromByteArray(throwawayNodeType, nodePublicAddress, nodeLocalAddress,
                                                                  receivedPacket, senderSockAddr);
                
                SharedNodePointer checkInNode = nodeList->updateSocketsForNode(nodeUUID, nodePublicAddress, nodeLocalAddress);
                
                // update last receive to now
                quint64 timeNow = usecTimestampNow();
                checkInNode->setLastHeardMicrostamp(timeNow);
            
                sendDomainListToNode(checkInNode, senderSockAddr, nodeInterestListFromPacket(receivedPacket, numNodeInfoBytes));
            }
        } else if (requestType == PacketTypeNodeJsonStats) {
            SharedNodePointer matchingNode = nodeList->sendingNodeForPacket(receivedPacket);
            if (matchingNode) {
                reinterpret_cast<DomainServerNodeData*>(matchingNode->getLinkedData())->parseJSONStatsPacket(receivedPacket);
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

const char ASSIGNMENT_SCRIPT_HOST_LOCATION[] = "resources/web/assignment";

QString pathForAssignmentScript(const QUuid& assignmentUUID) {
    QString newPath(ASSIGNMENT_SCRIPT_HOST_LOCATION);
    newPath += "/";
    // append the UUID for this script as the new filename, remove the curly braces
    newPath += uuidStringWithoutCurlyBraces(assignmentUUID);
    return newPath;
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
            foreach (const SharedNodePointer& node, LimitedNodeList::getInstance()->getNodeHash()) {
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
            LimitedNodeList* nodeList = LimitedNodeList::getInstance();
            
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
                SharedNodePointer matchingNode = LimitedNodeList::getInstance()->nodeWithUUID(matchingUUID);
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
            
            // check optional headers for # of instances and pool
            const QString ASSIGNMENT_INSTANCES_HEADER = "ASSIGNMENT-INSTANCES";
            const QString ASSIGNMENT_POOL_HEADER = "ASSIGNMENT-POOL";
            
            QByteArray assignmentInstancesValue = connection->requestHeaders().value(ASSIGNMENT_INSTANCES_HEADER.toLocal8Bit());
            
            int numInstances = 1;
            
            if (!assignmentInstancesValue.isEmpty()) {
                // the user has requested a specific number of instances
                // so set that on the created assignment
                
                numInstances = assignmentInstancesValue.toInt();
            }
            
            QString assignmentPool = emptyPool;
            QByteArray assignmentPoolValue = connection->requestHeaders().value(ASSIGNMENT_POOL_HEADER.toLocal8Bit());
            
            if (!assignmentPoolValue.isEmpty()) {
                // specific pool requested, set that on the created assignment
                assignmentPool = QString(assignmentPoolValue);
            }
            
            
            for (int i = 0; i < numInstances; i++) {
                
                // create an assignment for this saved script
                Assignment* scriptAssignment = new Assignment(Assignment::CreateCommand, Assignment::AgentType, assignmentPool);
                
                QString newPath = pathForAssignmentScript(scriptAssignment->getUUID());
                
                // create a file with the GUID of the assignment in the script host location
                QFile scriptFile(newPath);
                scriptFile.open(QIODevice::WriteOnly);
                scriptFile.write(formData[0].second);
                
                qDebug() << qPrintable(QString("Saved a script for assignment at %1%2")
                                       .arg(newPath).arg(assignmentPool == emptyPool ? "" : " - pool is " + assignmentPool));
                
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
            
            SharedNodePointer nodeToKill = LimitedNodeList::getInstance()->nodeWithUUID(deleteUUID);
            
            if (nodeToKill) {
                // start with a 200 response
                connection->respond(HTTPConnection::StatusCode200);
                
                // we have a valid UUID and node - kill the node that has this assignment
                QMetaObject::invokeMethod(LimitedNodeList::getInstance(), "killNodeWithUUID", Q_ARG(const QUuid&, deleteUUID));
                
                // successfully processed request
                return true;
            }
            
            return true;
        } else if (allNodesDeleteRegex.indexIn(url.path()) != -1) {
            qDebug() << "Received request to kill all nodes.";
            LimitedNodeList::getInstance()->eraseAllNodes();
            
            return true;
        }
    }
    
    // didn't process the request, let the HTTPManager try and handle
    return false;
}

bool DomainServer::handleHTTPSRequest(HTTPSConnection* connection, const QUrl &url) {
    const QString URI_OAUTH = "/oauth";
    if (url.path() == URI_OAUTH) {
        qDebug() << "Handling an OAuth authorization.";
        
        const QString CODE_QUERY_KEY = "code";
        QString authorizationCode = QUrlQuery(url).queryItemValue(CODE_QUERY_KEY);
        
        if (!authorizationCode.isEmpty()) {
            
        }
        
        // respond with a 200 code indicating that login is complete
        connection->respond(HTTPConnection::StatusCode200);
        
        return true;
    } else {
        return false;
    }
}

void DomainServer::refreshStaticAssignmentAndAddToQueue(SharedAssignmentPointer& assignment) {
    QUuid oldUUID = assignment->getUUID();
    assignment->resetUUID();
    
    qDebug() << "Reset UUID for assignment -" << *assignment.data() << "- and added to queue. Old UUID was"
        << uuidStringWithoutCurlyBraces(oldUUID);
    
    if (assignment->getType() == Assignment::AgentType && assignment->getPayload().isEmpty()) {
        // if this was an Agent without a script URL, we need to rename the old file so it can be retrieved at the new UUID
        QFile::rename(pathForAssignmentScript(oldUUID), pathForAssignmentScript(assignment->getUUID()));
    }
    
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
            SharedNodePointer otherNode = LimitedNodeList::getInstance()->nodeWithUUID(otherNodeSessionUUID);
            if (otherNode) {
                reinterpret_cast<DomainServerNodeData*>(otherNode->getLinkedData())->getSessionSecretHash().remove(node->getUUID());
            }
        }
        
        if (_isUsingDTLS) {
            // check if we need to remove a DTLS session from our in-memory hash
            DTLSServerSession* existingSession = _dtlsSessions.take(nodeData->getSendingSockAddr());
            if (existingSession) {
                delete existingSession;
            }
        }
    }
}

SharedAssignmentPointer DomainServer::matchingQueuedAssignmentForCheckIn(const QUuid& checkInUUID, NodeType_t nodeType) {
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
            
            // remove the assignment from the queue
            SharedAssignmentPointer deployableAssignment = _assignmentQueue.takeAt(sharedAssignment
                                                                                   - _assignmentQueue.begin());
            
            if (deployableAssignment->getType() != Assignment::AgentType
                || _staticAssignmentHash.contains(deployableAssignment->getUUID())) {
                // this is a static assignment
                // until we get a check-in from that GUID
                // put assignment back in queue but stick it at the back so the others have a chance to go out
                _assignmentQueue.enqueue(deployableAssignment);
            }
            
            // stop looping, we've handed out an assignment
            return deployableAssignment;
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
        foreach (const SharedNodePointer& node, LimitedNodeList::getInstance()->getNodeHash()) {
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
