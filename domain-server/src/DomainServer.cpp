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

#include "DomainServer.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QSharedMemory>
#include <QStandardPaths>
#include <QTimer>
#include <QUrlQuery>

#include <AccountManager.h>
#include <ApplicationVersion.h>
#include <HifiConfigVariantMap.h>
#include <HTTPConnection.h>
#include <JSONBreakableMarshal.h>
#include <LogUtils.h>
#include <NetworkingConstants.h>
#include <udt/PacketHeaders.h>
#include <SettingHandle.h>
#include <SharedUtil.h>
#include <ShutdownEventListener.h>
#include <UUID.h>
#include <LogHandler.h>

#include "DomainServerNodeData.h"
#include "NodeConnectionData.h"

int const DomainServer::EXIT_CODE_REBOOT = 234923;

const QString ICE_SERVER_DEFAULT_HOSTNAME = "ice.highfidelity.io";

DomainServer::DomainServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _gatekeeper(this),
    _httpManager(DOMAIN_SERVER_HTTP_PORT, QString("%1/resources/web/").arg(QCoreApplication::applicationDirPath()), this),
    _httpsManager(NULL),
    _allAssignments(),
    _unfulfilledAssignments(),
    _isUsingDTLS(false),
    _oauthProviderURL(),
    _oauthClientID(),
    _hostname(),
    _webAuthenticationStateSet(),
    _cookieSessionHash(),
    _automaticNetworkingSetting(),
    _settingsManager(),
    _iceServerSocket(ICE_SERVER_DEFAULT_HOSTNAME, ICE_SERVER_DEFAULT_PORT)
{
    qInstallMessageHandler(LogHandler::verboseMessageHandler);

    LogUtils::init();
    Setting::init();

    connect(this, &QCoreApplication::aboutToQuit, this, &DomainServer::aboutToQuit);

    setOrganizationName("High Fidelity");
    setOrganizationDomain("highfidelity.io");
    setApplicationName("domain-server");
    setApplicationVersion(BUILD_VERSION);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // make sure we have a fresh AccountManager instance
    // (need this since domain-server can restart itself and maintain static variables)
    AccountManager::getInstance(true);

    _settingsManager.setupConfigMap(arguments());

    // setup a shutdown event listener to handle SIGTERM or WM_CLOSE for us
#ifdef _WIN32
    installNativeEventFilter(&ShutdownEventListener::getInstance());
#else
    ShutdownEventListener::getInstance();
#endif

    qRegisterMetaType<DomainServerWebSessionData>("DomainServerWebSessionData");
    qRegisterMetaTypeStreamOperators<DomainServerWebSessionData>("DomainServerWebSessionData");
    
    // make sure we hear about newly connected nodes from our gatekeeper
    connect(&_gatekeeper, &DomainGatekeeper::connectedNode, this, &DomainServer::handleConnectedNode);

    if (optionallyReadX509KeyAndCertificate() && optionallySetupOAuth() && optionallySetupAssignmentPayment()) {
        // we either read a certificate and private key or were not passed one
        // and completed login or did not need to

        qDebug() << "Setting up LimitedNodeList and assignments.";
        setupNodeListAndAssignments();

        loadExistingSessionsFromSettings();

        // setup automatic networking settings with data server
        setupAutomaticNetworking();

        // preload some user public keys so they can connect on first request
        _gatekeeper.preloadAllowedUserPublicKeys();
    }
}

DomainServer::~DomainServer() {
    // destroy the LimitedNodeList before the DomainServer QCoreApplication is down
    DependencyManager::destroy<LimitedNodeList>();
}

void DomainServer::aboutToQuit() {

    // clear the log handler so that Qt doesn't call the destructor on LogHandler
    qInstallMessageHandler(0);
}

void DomainServer::restart() {
    qDebug() << "domain-server is restarting.";

    exit(DomainServer::EXIT_CODE_REBOOT);
}

bool DomainServer::optionallyReadX509KeyAndCertificate() {
    const QString X509_CERTIFICATE_OPTION = "cert";
    const QString X509_PRIVATE_KEY_OPTION = "key";
    const QString X509_KEY_PASSPHRASE_ENV = "DOMAIN_SERVER_KEY_PASSPHRASE";

    QString certPath = _settingsManager.getSettingsMap().value(X509_CERTIFICATE_OPTION).toString();
    QString keyPath = _settingsManager.getSettingsMap().value(X509_PRIVATE_KEY_OPTION).toString();

    if (!certPath.isEmpty() && !keyPath.isEmpty()) {
        // the user wants to use DTLS to encrypt communication with nodes
        // let's make sure we can load the key and certificate
//        _x509Credentials = new gnutls_certificate_credentials_t;
//        gnutls_certificate_allocate_credentials(_x509Credentials);

        QString keyPassphraseString = QProcessEnvironment::systemEnvironment().value(X509_KEY_PASSPHRASE_ENV);

        qDebug() << "Reading certificate file at" << certPath << "for DTLS.";
        qDebug() << "Reading key file at" << keyPath << "for DTLS.";

//        int gnutlsReturn = gnutls_certificate_set_x509_key_file2(*_x509Credentials,
//                                                                 certPath.toLocal8Bit().constData(),
//                                                                 keyPath.toLocal8Bit().constData(),
//                                                                 GNUTLS_X509_FMT_PEM,
//                                                                 keyPassphraseString.toLocal8Bit().constData(),
//                                                                 0);
//
//        if (gnutlsReturn < 0) {
//            qDebug() << "Unable to load certificate or key file." << "Error" << gnutlsReturn << "- domain-server will now quit.";
//            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
//            return false;
//        }

//        qDebug() << "Successfully read certificate and private key.";

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

    const QVariantMap& settingsMap = _settingsManager.getSettingsMap();
    _oauthProviderURL = QUrl(settingsMap.value(OAUTH_PROVIDER_URL_OPTION).toString());

    // if we don't have an oauth provider URL then we default to the default node auth url
    if (_oauthProviderURL.isEmpty()) {
        _oauthProviderURL = NetworkingConstants::METAVERSE_SERVER_URL;
    }

    AccountManager& accountManager = AccountManager::getInstance();
    accountManager.disableSettingsFilePersistence();
    accountManager.setAuthURL(_oauthProviderURL);

    _oauthClientID = settingsMap.value(OAUTH_CLIENT_ID_OPTION).toString();
    _oauthClientSecret = QProcessEnvironment::systemEnvironment().value(OAUTH_CLIENT_SECRET_ENV);
    _hostname = settingsMap.value(REDIRECT_HOSTNAME_OPTION).toString();

    if (!_oauthClientID.isEmpty()) {
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

const QString DOMAIN_CONFIG_ID_KEY = "id";

const QString METAVERSE_AUTOMATIC_NETWORKING_KEY_PATH = "metaverse.automatic_networking";
const QString FULL_AUTOMATIC_NETWORKING_VALUE = "full";
const QString IP_ONLY_AUTOMATIC_NETWORKING_VALUE = "ip";
const QString DISABLED_AUTOMATIC_NETWORKING_VALUE = "disabled";

void DomainServer::setupNodeListAndAssignments(const QUuid& sessionUUID) {

    const QString CUSTOM_LOCAL_PORT_OPTION = "metaverse.local_port";

    QVariant localPortValue = _settingsManager.valueOrDefaultValueForKeyPath(CUSTOM_LOCAL_PORT_OPTION);
    unsigned short domainServerPort = (unsigned short) localPortValue.toUInt();

    QVariantMap& settingsMap = _settingsManager.getSettingsMap();

    unsigned short domainServerDTLSPort = 0;

    if (_isUsingDTLS) {
        domainServerDTLSPort = DEFAULT_DOMAIN_SERVER_DTLS_PORT;

        const QString CUSTOM_DTLS_PORT_OPTION = "dtls-port";

        if (settingsMap.contains(CUSTOM_DTLS_PORT_OPTION)) {
            domainServerDTLSPort = (unsigned short) settingsMap.value(CUSTOM_DTLS_PORT_OPTION).toUInt();
        }
    }

    QSet<Assignment::Type> parsedTypes;
    parseAssignmentConfigs(parsedTypes);

    populateDefaultStaticAssignmentsExcludingTypes(parsedTypes);

    // check for scripts the user wants to persist from their domain-server config
    populateStaticScriptedAssignmentsFromSettings();

    auto nodeList = DependencyManager::set<LimitedNodeList>(domainServerPort, domainServerDTLSPort);

    // no matter the local port, save it to shared mem so that local assignment clients can ask what it is
    nodeList->putLocalPortIntoSharedMemory(DOMAIN_SERVER_LOCAL_PORT_SMEM_KEY, this, nodeList->getSocketLocalPort());

    // store our local http ports in shared memory
    quint16 localHttpPort = DOMAIN_SERVER_HTTP_PORT;
    nodeList->putLocalPortIntoSharedMemory(DOMAIN_SERVER_LOCAL_HTTP_PORT_SMEM_KEY, this, localHttpPort);
    quint16 localHttpsPort = DOMAIN_SERVER_HTTPS_PORT;
    nodeList->putLocalPortIntoSharedMemory(DOMAIN_SERVER_LOCAL_HTTPS_PORT_SMEM_KEY, this, localHttpsPort);


    // set our LimitedNodeList UUID to match the UUID from our config
    // nodes will currently use this to add resources to data-web that relate to our domain
    const QString METAVERSE_DOMAIN_ID_KEY_PATH = "metaverse.id";
    const QVariant* idValueVariant = valueForKeyPath(settingsMap, METAVERSE_DOMAIN_ID_KEY_PATH);
    if (idValueVariant) {
        nodeList->setSessionUUID(idValueVariant->toString());
    }

    connect(nodeList.data(), &LimitedNodeList::nodeAdded, this, &DomainServer::nodeAdded);
    connect(nodeList.data(), &LimitedNodeList::nodeKilled, this, &DomainServer::nodeKilled);

    // register as the packet receiver for the types we want
    PacketReceiver& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::RequestAssignment, this, "processRequestAssignmentPacket");
    packetReceiver.registerListener(PacketType::DomainListRequest, this, "processListRequestPacket");
    packetReceiver.registerListener(PacketType::DomainServerPathQuery, this, "processPathQueryPacket");
    packetReceiver.registerListener(PacketType::NodeJsonStats, this, "processNodeJSONStatsPacket");
    
    // NodeList won't be available to the settings manager when it is created, so call registerListener here
    packetReceiver.registerListener(PacketType::DomainSettingsRequest, &_settingsManager, "processSettingsRequestPacket");
    
    // register the gatekeeper for the packets it needs to receive
    packetReceiver.registerListener(PacketType::DomainConnectRequest, &_gatekeeper, "processConnectRequestPacket");
    packetReceiver.registerListener(PacketType::ICEPing, &_gatekeeper, "processICEPingPacket");
    packetReceiver.registerListener(PacketType::ICEPingReply, &_gatekeeper, "processICEPingReplyPacket");
    packetReceiver.registerListener(PacketType::ICEServerPeerInformation, &_gatekeeper, "processICEPeerInformationPacket");
    
    // add whatever static assignments that have been parsed to the queue
    addStaticAssignmentsToQueue();
}

bool DomainServer::didSetupAccountManagerWithAccessToken() {
    if (AccountManager::getInstance().hasValidAccessToken()) {
        // we already gave the account manager a valid access token
        return true;
    }

    return resetAccountManagerAccessToken();
}

const QString ACCESS_TOKEN_KEY_PATH = "metaverse.access_token";

bool DomainServer::resetAccountManagerAccessToken() {
    if (!_oauthProviderURL.isEmpty()) {
        // check for an access-token in our settings, can optionally be overidden by env value
        const QString ENV_ACCESS_TOKEN_KEY = "DOMAIN_SERVER_ACCESS_TOKEN";

        QString accessToken = QProcessEnvironment::systemEnvironment().value(ENV_ACCESS_TOKEN_KEY);

        if (accessToken.isEmpty()) {
            const QVariant* accessTokenVariant = valueForKeyPath(_settingsManager.getSettingsMap(), ACCESS_TOKEN_KEY_PATH);

            if (accessTokenVariant && accessTokenVariant->canConvert(QMetaType::QString)) {
                accessToken = accessTokenVariant->toString();
            } else {
                qDebug() << "A domain-server feature that requires authentication is enabled but no access token is present."
                    << "Set an access token via the web interface, in your user or master config"
                    << "at keypath metaverse.access_token or in your ENV at key DOMAIN_SERVER_ACCESS_TOKEN";
                return false;
            }
        } else {
            qDebug() << "Using access token from DOMAIN_SERVER_ACCESS_TOKEN in env. This overrides any access token present"
                << " in the user or master config.";
        }

        // give this access token to the AccountManager
        AccountManager::getInstance().setAccessTokenForCurrentAuthURL(accessToken);

        return true;

    } else {
        qDebug() << "Missing OAuth provider URL, but a domain-server feature was required that requires authentication." <<
            "domain-server will now quit.";
        QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);

        return false;
    }
}

bool DomainServer::optionallySetupAssignmentPayment() {
    const QString PAY_FOR_ASSIGNMENTS_OPTION = "pay-for-assignments";
    const QVariantMap& settingsMap = _settingsManager.getSettingsMap();

    if (settingsMap.contains(PAY_FOR_ASSIGNMENTS_OPTION) &&
        settingsMap.value(PAY_FOR_ASSIGNMENTS_OPTION).toBool() &&
        didSetupAccountManagerWithAccessToken()) {

        qDebug() << "Assignments will be paid for via" << qPrintable(_oauthProviderURL.toString());

        // assume that the fact we are authing against HF data server means we will pay for assignments
        // setup a timer to send transactions to pay assigned nodes every 30 seconds
        QTimer* creditSetupTimer = new QTimer(this);
        connect(creditSetupTimer, &QTimer::timeout, this, &DomainServer::setupPendingAssignmentCredits);

        const qint64 CREDIT_CHECK_INTERVAL_MSECS = 5 * 1000;
        creditSetupTimer->start(CREDIT_CHECK_INTERVAL_MSECS);

        QTimer* nodePaymentTimer = new QTimer(this);
        connect(nodePaymentTimer, &QTimer::timeout, this, &DomainServer::sendPendingTransactionsToServer);

        const qint64 TRANSACTION_SEND_INTERVAL_MSECS = 30 * 1000;
        nodePaymentTimer->start(TRANSACTION_SEND_INTERVAL_MSECS);
    }

    return true;
}

void DomainServer::setupAutomaticNetworking() {
    auto nodeList = DependencyManager::get<LimitedNodeList>();

    _automaticNetworkingSetting =
        _settingsManager.valueOrDefaultValueForKeyPath(METAVERSE_AUTOMATIC_NETWORKING_KEY_PATH).toString();

    if (_automaticNetworkingSetting == FULL_AUTOMATIC_NETWORKING_VALUE) {
        // call our sendHeartbeatToIceServer immediately anytime a local or public socket changes
        connect(nodeList.data(), &LimitedNodeList::localSockAddrChanged,
                this, &DomainServer::sendHeartbeatToIceServer);
        connect(nodeList.data(), &LimitedNodeList::publicSockAddrChanged,
                this, &DomainServer::sendHeartbeatToIceServer);

        // we need this DS to know what our public IP is - start trying to figure that out now
        nodeList->startSTUNPublicSocketUpdate();

        // setup a timer to heartbeat with the ice-server every so often
        QTimer* iceHeartbeatTimer = new QTimer(this);
        connect(iceHeartbeatTimer, &QTimer::timeout, this, &DomainServer::sendHeartbeatToIceServer);
        iceHeartbeatTimer->start(ICE_HEARBEAT_INTERVAL_MSECS);
    }

    if (!didSetupAccountManagerWithAccessToken()) {
        qDebug() << "Cannot send heartbeat to data server without an access token.";
        qDebug() << "Add an access token to your config file or via the web interface.";

        return;
    }

    if (_automaticNetworkingSetting == IP_ONLY_AUTOMATIC_NETWORKING_VALUE ||
        _automaticNetworkingSetting == FULL_AUTOMATIC_NETWORKING_VALUE) {

        const QUuid& domainID = nodeList->getSessionUUID();

        if (!domainID.isNull()) {
            qDebug() << "domain-server" << _automaticNetworkingSetting << "automatic networking enabled for ID"
                << uuidStringWithoutCurlyBraces(domainID) << "via" << _oauthProviderURL.toString();

            if (_automaticNetworkingSetting == IP_ONLY_AUTOMATIC_NETWORKING_VALUE) {
                // send any public socket changes to the data server so nodes can find us at our new IP
                connect(nodeList.data(), &LimitedNodeList::publicSockAddrChanged,
                        this, &DomainServer::performIPAddressUpdate);

                // have the LNL enable public socket updating via STUN
                nodeList->startSTUNPublicSocketUpdate();
            } else {
                // send our heartbeat to data server so it knows what our network settings are
                sendHeartbeatToDataServer();
            }
        } else {
            qDebug() << "Cannot enable domain-server automatic networking without a domain ID."
            << "Please add an ID to your config file or via the web interface.";

            return;
        }
    } else {
        sendHeartbeatToDataServer();
    }

    qDebug() << "Updating automatic networking setting in domain-server to" << _automaticNetworkingSetting;

    // no matter the auto networking settings we should heartbeat to the data-server every 15s
    const int DOMAIN_SERVER_DATA_WEB_HEARTBEAT_MSECS = 15 * 1000;

    QTimer* dataHeartbeatTimer = new QTimer(this);
    connect(dataHeartbeatTimer, SIGNAL(timeout()), this, SLOT(sendHeartbeatToDataServer()));
    dataHeartbeatTimer->start(DOMAIN_SERVER_DATA_WEB_HEARTBEAT_MSECS);
}

void DomainServer::loginFailed() {
    qDebug() << "Login to data server has failed. domain-server will now quit";
    QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
}

void DomainServer::parseAssignmentConfigs(QSet<Assignment::Type>& excludedTypes) {
    // check for configs from the command line, these take precedence
    const QString ASSIGNMENT_CONFIG_REGEX_STRING = "config-([\\d]+)";
    QRegExp assignmentConfigRegex(ASSIGNMENT_CONFIG_REGEX_STRING);

    const QVariantMap& settingsMap = _settingsManager.getSettingsMap();

    // scan for assignment config keys
    QStringList variantMapKeys = settingsMap.keys();
    int configIndex = variantMapKeys.indexOf(assignmentConfigRegex);

    while (configIndex != -1) {
        // figure out which assignment type this matches
        Assignment::Type assignmentType = (Assignment::Type) assignmentConfigRegex.cap(1).toInt();

        if (assignmentType < Assignment::AllTypes && !excludedTypes.contains(assignmentType)) {
            QVariant mapValue = settingsMap[variantMapKeys[configIndex]];
            QVariantList assignmentList = mapValue.toList();

            if (assignmentType != Assignment::AgentType) {
                createStaticAssignmentsForType(assignmentType, assignmentList);
            }

            excludedTypes.insert(assignmentType);
        }

        configIndex = variantMapKeys.indexOf(assignmentConfigRegex, configIndex + 1);
    }
}

void DomainServer::addStaticAssignmentToAssignmentHash(Assignment* newAssignment) {
    qDebug() << "Inserting assignment" << *newAssignment << "to static assignment hash.";
    newAssignment->setIsStatic(true);
    _allAssignments.insert(newAssignment->getUUID(), SharedAssignmentPointer(newAssignment));
}

void DomainServer::populateStaticScriptedAssignmentsFromSettings() {
    const QString PERSISTENT_SCRIPTS_KEY_PATH = "scripts.persistent_scripts";
    const QVariant* persistentScriptsVariant = valueForKeyPath(_settingsManager.getSettingsMap(), PERSISTENT_SCRIPTS_KEY_PATH);

    if (persistentScriptsVariant) {
        QVariantList persistentScriptsList = persistentScriptsVariant->toList();
        foreach(const QVariant& persistentScriptVariant, persistentScriptsList) {
            QVariantMap persistentScript = persistentScriptVariant.toMap();

            const QString PERSISTENT_SCRIPT_URL_KEY = "url";
            const QString PERSISTENT_SCRIPT_NUM_INSTANCES_KEY = "num_instances";
            const QString PERSISTENT_SCRIPT_POOL_KEY = "pool";

            if (persistentScript.contains(PERSISTENT_SCRIPT_URL_KEY)) {
                // check how many instances of this script to add

                int numInstances = persistentScript[PERSISTENT_SCRIPT_NUM_INSTANCES_KEY].toInt();
                QString scriptURL = persistentScript[PERSISTENT_SCRIPT_URL_KEY].toString();

                QString scriptPool = persistentScript.value(PERSISTENT_SCRIPT_POOL_KEY).toString();

                qDebug() << "Adding" << numInstances << "of persistent script at URL" << scriptURL << "- pool" << scriptPool;

                for (int i = 0; i < numInstances; ++i) {
                    // add a scripted assignment to the queue for this instance
                    Assignment* scriptAssignment = new Assignment(Assignment::CreateCommand,
                                                                  Assignment::AgentType,
                                                                  scriptPool);
                    scriptAssignment->setPayload(scriptURL.toUtf8());

                    // add it to static hash so we know we have to keep giving it back out
                    addStaticAssignmentToAssignmentHash(scriptAssignment);
                }
            }
        }
    }
}

void DomainServer::createStaticAssignmentsForType(Assignment::Type type, const QVariantList &configList) {
    // we have a string for config for this type
    qDebug() << "Parsing config for assignment type" << type;

    int configCounter = 0;

    foreach(const QVariant& configVariant, configList) {
        if (configVariant.canConvert(QMetaType::QVariantMap)) {
            QVariantMap configMap = configVariant.toMap();

            // check the config string for a pool
            const QString ASSIGNMENT_POOL_KEY = "pool";

            QString assignmentPool = configMap.value(ASSIGNMENT_POOL_KEY).toString();
            if (!assignmentPool.isEmpty()) {
                configMap.remove(ASSIGNMENT_POOL_KEY);
            }

            ++configCounter;
            qDebug() << "Type" << type << "config" << configCounter << "=" << configMap;

            Assignment* configAssignment = new Assignment(Assignment::CreateCommand, type, assignmentPool);

            // setup the payload as a semi-colon separated list of key = value
            QStringList payloadStringList;
            foreach(const QString& payloadKey, configMap.keys()) {
                QString dashes = payloadKey.size() == 1 ? "-" : "--";
                payloadStringList << QString("%1%2 %3").arg(dashes).arg(payloadKey).arg(configMap[payloadKey].toString());
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
        if (!excludedTypes.contains(defaultedType)
            && defaultedType != Assignment::UNUSED_0
            && defaultedType != Assignment::UNUSED_1
            && defaultedType != Assignment::AgentType) {
            
            if (defaultedType == Assignment::AssetServerType) {
                // Make sure the asset-server is enabled before adding it here.
                // Initially we do not assign it by default so we can test it in HF domains first
                static const QString ASSET_SERVER_ENABLED_KEYPATH = "asset_server.enabled";
                
                if (!_settingsManager.valueOrDefaultValueForKeyPath(ASSET_SERVER_ENABLED_KEYPATH).toBool()) {
                    // skip to the next iteration if asset-server isn't enabled
                    continue;
                }
            }
            
            // type has not been set from a command line or config file config, use the default
            // by clearing whatever exists and writing a single default assignment with no payload
            Assignment* newAssignment = new Assignment(Assignment::CreateCommand, (Assignment::Type) defaultedType);
            addStaticAssignmentToAssignmentHash(newAssignment);
        }
    }
}

void DomainServer::processListRequestPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    
    QDataStream packetStream(packet.data());
    NodeConnectionData nodeRequestData = NodeConnectionData::fromDataStream(packetStream, packet->getSenderSockAddr(), false);

    // update this node's sockets in case they have changed
    sendingNode->setPublicSocket(nodeRequestData.publicSockAddr);
    sendingNode->setLocalSocket(nodeRequestData.localSockAddr);
    
    // update the NodeInterestSet in case there have been any changes
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(sendingNode->getLinkedData());
    nodeData->setNodeInterestSet(nodeRequestData.interestList.toSet());

    sendDomainListToNode(sendingNode, packet->getSenderSockAddr());
}

unsigned int DomainServer::countConnectedUsers() {
    unsigned int result = 0;
    auto nodeList = DependencyManager::get<LimitedNodeList>();
    nodeList->eachNode([&](const SharedNodePointer& otherNode){
        if (otherNode->getType() == NodeType::Agent) {
            result++;
        }
    });
    return result;
}

QUrl DomainServer::oauthRedirectURL() {
    return QString("https://%1:%2/oauth").arg(_hostname).arg(_httpsManager->serverPort());
}

const QString OAUTH_CLIENT_ID_QUERY_KEY = "client_id";
const QString OAUTH_REDIRECT_URI_QUERY_KEY = "redirect_uri";

QUrl DomainServer::oauthAuthorizationURL(const QUuid& stateUUID) {
    // for now these are all interface clients that have a GUI
    // so just send them back the full authorization URL
    QUrl authorizationURL = _oauthProviderURL;

    const QString OAUTH_AUTHORIZATION_PATH = "/oauth/authorize";
    authorizationURL.setPath(OAUTH_AUTHORIZATION_PATH);

    QUrlQuery authorizationQuery;

    authorizationQuery.addQueryItem(OAUTH_CLIENT_ID_QUERY_KEY, _oauthClientID);

    const QString OAUTH_RESPONSE_TYPE_QUERY_KEY = "response_type";
    const QString OAUTH_REPSONSE_TYPE_QUERY_VALUE = "code";
    authorizationQuery.addQueryItem(OAUTH_RESPONSE_TYPE_QUERY_KEY, OAUTH_REPSONSE_TYPE_QUERY_VALUE);

    const QString OAUTH_STATE_QUERY_KEY = "state";
    // create a new UUID that will be the state parameter for oauth authorization AND the new session UUID for that node
    authorizationQuery.addQueryItem(OAUTH_STATE_QUERY_KEY, uuidStringWithoutCurlyBraces(stateUUID));

    authorizationQuery.addQueryItem(OAUTH_REDIRECT_URI_QUERY_KEY, oauthRedirectURL().toString());

    authorizationURL.setQuery(authorizationQuery);

    return authorizationURL;
}

void DomainServer::handleConnectedNode(SharedNodePointer newNode) {
    
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(newNode->getLinkedData());
    
    // reply back to the user with a PacketType::DomainList
    sendDomainListToNode(newNode, nodeData->getSendingSockAddr());
    
    // send out this node to our other connected nodes
    broadcastNewNode(newNode);
}

void DomainServer::sendDomainListToNode(const SharedNodePointer& node, const HifiSockAddr &senderSockAddr) {
    const int NUM_DOMAIN_LIST_EXTENDED_HEADER_BYTES = NUM_BYTES_RFC4122_UUID + NUM_BYTES_RFC4122_UUID + 2;
    
    // setup the extended header for the domain list packets
    // this data is at the beginning of each of the domain list packets
    QByteArray extendedHeader(NUM_DOMAIN_LIST_EXTENDED_HEADER_BYTES, 0);
    QDataStream extendedHeaderStream(&extendedHeader, QIODevice::WriteOnly);
   
    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
    
    extendedHeaderStream << limitedNodeList->getSessionUUID();
    extendedHeaderStream << node->getUUID();
    extendedHeaderStream << (quint8) node->getCanAdjustLocks();
    extendedHeaderStream << (quint8) node->getCanRez();

    NLPacketList domainListPackets(PacketType::DomainList, extendedHeader);

    // always send the node their own UUID back
    QDataStream domainListStream(&domainListPackets);

    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());

    // store the nodeInterestSet on this DomainServerNodeData, in case it has changed
    auto& nodeInterestSet = nodeData->getNodeInterestSet();

    if (nodeInterestSet.size() > 0) {

        // DTLSServerSession* dtlsSession = _isUsingDTLS ? _dtlsSessions[senderSockAddr] : NULL;
        if (nodeData->isAuthenticated()) {
            // if this authenticated node has any interest types, send back those nodes as well
            limitedNodeList->eachNode([&](const SharedNodePointer& otherNode){
                if (otherNode->getUUID() != node->getUUID() && nodeInterestSet.contains(otherNode->getType())) {
                    
                    // since we're about to add a node to the packet we start a segment
                    domainListPackets.startSegment();

                    // don't send avatar nodes to other avatars, that will come from avatar mixer
                    domainListStream << *otherNode.data();

                    // pack the secret that these two nodes will use to communicate with each other
                    domainListStream << connectionSecretForNodes(node, otherNode);

                    // we've added the node we wanted so end the segment now
                    domainListPackets.endSegment();
                }
            });
        }
    }
    
    // send an empty list to the node, in case there were no other nodes
    domainListPackets.closeCurrentPacket(true);

    // write the PacketList to this node
    limitedNodeList->sendPacketList(domainListPackets, *node);
}

QUuid DomainServer::connectionSecretForNodes(const SharedNodePointer& nodeA, const SharedNodePointer& nodeB) {
    DomainServerNodeData* nodeAData = dynamic_cast<DomainServerNodeData*>(nodeA->getLinkedData());
    DomainServerNodeData* nodeBData = dynamic_cast<DomainServerNodeData*>(nodeB->getLinkedData());

    if (nodeAData && nodeBData) {
        QUuid& secretUUID = nodeAData->getSessionSecretHash()[nodeB->getUUID()];

        if (secretUUID.isNull()) {
            // generate a new secret UUID these two nodes can use
            secretUUID = QUuid::createUuid();

            // set it on the other Node's sessionSecretHash
            reinterpret_cast<DomainServerNodeData*>(nodeBData)->getSessionSecretHash().insert(nodeA->getUUID(), secretUUID);
        }

        return secretUUID;
    }

    return QUuid();
}

void DomainServer::broadcastNewNode(const SharedNodePointer& addedNode) {

    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();

    auto addNodePacket = NLPacket::create(PacketType::DomainServerAddedNode);

    // setup the add packet for this new node
    QDataStream addNodeStream(addNodePacket.get());

    addNodeStream << *addedNode.data();

    int connectionSecretIndex = addNodePacket->pos();

    limitedNodeList->eachMatchingNode(
        [&](const SharedNodePointer& node)->bool {
            if (node->getLinkedData() && node->getActiveSocket() && node != addedNode) {
                // is the added Node in this node's interest list?
                DomainServerNodeData* nodeData = dynamic_cast<DomainServerNodeData*>(node->getLinkedData());
                return nodeData->getNodeInterestSet().contains(addedNode->getType());
            } else {
                return false;
            }
        },
        [&](const SharedNodePointer& node) {
            addNodePacket->seek(connectionSecretIndex);

            QByteArray rfcConnectionSecret = connectionSecretForNodes(node, addedNode).toRfc4122();

            // replace the bytes at the end of the packet for the connection secret between these nodes
            addNodePacket->write(rfcConnectionSecret);

            // send off this packet to the node
            limitedNodeList->sendUnreliablePacket(*addNodePacket, *node);
        }
    );
}

void DomainServer::processRequestAssignmentPacket(QSharedPointer<NLPacket> packet) {
    // construct the requested assignment from the packet data
    Assignment requestAssignment(*packet);

    // Suppress these for Assignment::AgentType to once per 5 seconds
    static QElapsedTimer noisyMessageTimer;
    static bool wasNoisyTimerStarted = false;

    if (!wasNoisyTimerStarted) {
        noisyMessageTimer.start();
        wasNoisyTimerStarted = true;
    }

    const qint64 NOISY_MESSAGE_INTERVAL_MSECS = 5 * 1000;

    if (requestAssignment.getType() != Assignment::AgentType
        || noisyMessageTimer.elapsed() > NOISY_MESSAGE_INTERVAL_MSECS) {
        static QString repeatedMessage = LogHandler::getInstance().addOnlyOnceMessageRegex
            ("Received a request for assignment type [^ ]+ from [^ ]+");
        qDebug() << "Received a request for assignment type" << requestAssignment.getType()
                 << "from" << packet->getSenderSockAddr();
        noisyMessageTimer.restart();
    }

    SharedAssignmentPointer assignmentToDeploy = deployableAssignmentForRequest(requestAssignment);

    if (assignmentToDeploy) {
        qDebug() << "Deploying assignment -" << *assignmentToDeploy.data() << "- to" << packet->getSenderSockAddr();

        // give this assignment out, either the type matches or the requestor said they will take any
        static std::unique_ptr<NLPacket> assignmentPacket;

        if (!assignmentPacket) {
            assignmentPacket = NLPacket::create(PacketType::CreateAssignment);
        }

        // setup a copy of this assignment that will have a unique UUID, for packaging purposes
        Assignment uniqueAssignment(*assignmentToDeploy.data());
        uniqueAssignment.setUUID(QUuid::createUuid());

        // reset the assignmentPacket
        assignmentPacket->reset();

        QDataStream assignmentStream(assignmentPacket.get());

        assignmentStream << uniqueAssignment;

        auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
        limitedNodeList->sendUnreliablePacket(*assignmentPacket, packet->getSenderSockAddr());

        // give the information for that deployed assignment to the gatekeeper so it knows to that that node
        // in when it comes back around
        _gatekeeper.addPendingAssignedNode(uniqueAssignment.getUUID(), assignmentToDeploy->getUUID(),
                                           requestAssignment.getWalletUUID(), requestAssignment.getNodeVersion());
    } else {
        if (requestAssignment.getType() != Assignment::AgentType
            || noisyMessageTimer.elapsed() > NOISY_MESSAGE_INTERVAL_MSECS) {
            static QString repeatedMessage = LogHandler::getInstance().addOnlyOnceMessageRegex
                ("Unable to fulfill assignment request of type [^ ]+ from [^ ]+");
            qDebug() << "Unable to fulfill assignment request of type" << requestAssignment.getType()
                << "from" << packet->getSenderSockAddr();
            noisyMessageTimer.restart();
        }
    }
}

void DomainServer::setupPendingAssignmentCredits() {
    // enumerate the NodeList to find the assigned nodes
    DependencyManager::get<LimitedNodeList>()->eachNode([&](const SharedNodePointer& node){
        DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());

        if (!nodeData->getAssignmentUUID().isNull() && !nodeData->getWalletUUID().isNull()) {
            // check if we have a non-finalized transaction for this node to add this amount to
            TransactionHash::iterator i = _pendingAssignmentCredits.find(nodeData->getWalletUUID());
            WalletTransaction* existingTransaction = NULL;

            while (i != _pendingAssignmentCredits.end() && i.key() == nodeData->getWalletUUID()) {
                if (!i.value()->isFinalized()) {
                    existingTransaction = i.value();
                    break;
                } else {
                    ++i;
                }
            }

            qint64 elapsedMsecsSinceLastPayment = nodeData->getPaymentIntervalTimer().elapsed();
            nodeData->getPaymentIntervalTimer().restart();

            const float CREDITS_PER_HOUR = 0.10f;
            const float CREDITS_PER_MSEC = CREDITS_PER_HOUR / (60 * 60 * 1000);
            const int SATOSHIS_PER_MSEC = CREDITS_PER_MSEC * SATOSHIS_PER_CREDIT;

            float pendingCredits = elapsedMsecsSinceLastPayment * SATOSHIS_PER_MSEC;

            if (existingTransaction) {
                existingTransaction->incrementAmount(pendingCredits);
            } else {
                // create a fresh transaction to pay this node, there is no transaction to append to
                WalletTransaction* freshTransaction = new WalletTransaction(nodeData->getWalletUUID(), pendingCredits);
                _pendingAssignmentCredits.insert(nodeData->getWalletUUID(), freshTransaction);
            }
        }
    });
}

void DomainServer::sendPendingTransactionsToServer() {

    AccountManager& accountManager = AccountManager::getInstance();

    if (accountManager.hasValidAccessToken()) {

        // enumerate the pending transactions and send them to the server to complete payment
        TransactionHash::iterator i = _pendingAssignmentCredits.begin();

        JSONCallbackParameters transactionCallbackParams;

        transactionCallbackParams.jsonCallbackReceiver = this;
        transactionCallbackParams.jsonCallbackMethod = "transactionJSONCallback";

        while (i != _pendingAssignmentCredits.end()) {
            accountManager.sendRequest("api/v1/transactions",
                                       AccountManagerAuth::Required,
                                       QNetworkAccessManager::PostOperation,
                                       transactionCallbackParams, i.value()->postJson().toJson());

            // set this transaction to finalized so we don't add additional credits to it
            i.value()->setIsFinalized(true);

            ++i;
        }
    }
}

void DomainServer::transactionJSONCallback(const QJsonObject& data) {
    // check if this was successful - if so we can remove it from our list of pending
    if (data.value("status").toString() == "success") {
        // create a dummy wallet transaction to unpack the JSON to
        WalletTransaction dummyTransaction;
        dummyTransaction.loadFromJson(data);

        TransactionHash::iterator i = _pendingAssignmentCredits.find(dummyTransaction.getDestinationUUID());

        while (i != _pendingAssignmentCredits.end() && i.key() == dummyTransaction.getDestinationUUID()) {
            if (i.value()->getUUID() == dummyTransaction.getUUID()) {
                // we have a match - we can remove this from the hash of pending credits
                // and delete it for clean up

                WalletTransaction* matchingTransaction = i.value();
                _pendingAssignmentCredits.erase(i);
                delete matchingTransaction;

                break;
            } else {
                ++i;
            }
        }
    }
}

QJsonObject jsonForDomainSocketUpdate(const HifiSockAddr& socket) {
    const QString SOCKET_NETWORK_ADDRESS_KEY = "network_address";
    const QString SOCKET_PORT_KEY = "port";

    QJsonObject socketObject;
    socketObject[SOCKET_NETWORK_ADDRESS_KEY] = socket.getAddress().toString();
    socketObject[SOCKET_PORT_KEY] = socket.getPort();

    return socketObject;
}

const QString DOMAIN_UPDATE_AUTOMATIC_NETWORKING_KEY = "automatic_networking";

void DomainServer::performIPAddressUpdate(const HifiSockAddr& newPublicSockAddr) {
    sendHeartbeatToDataServer(newPublicSockAddr.getAddress().toString());
}

void DomainServer::sendHeartbeatToDataServer(const QString& networkAddress) {
    const QString DOMAIN_UPDATE = "/api/v1/domains/%1";
    auto nodeList = DependencyManager::get<LimitedNodeList>();
    const QUuid& domainID = nodeList->getSessionUUID();

    // setup the domain object to send to the data server
    const QString PUBLIC_NETWORK_ADDRESS_KEY = "network_address";
    const QString AUTOMATIC_NETWORKING_KEY = "automatic_networking";

    QJsonObject domainObject;
    if (!networkAddress.isEmpty()) {
        domainObject[PUBLIC_NETWORK_ADDRESS_KEY] = networkAddress;
    }

    domainObject[AUTOMATIC_NETWORKING_KEY] = _automaticNetworkingSetting;

    // add a flag to indicate if this domain uses restricted access - for now that will exclude it from listings
    const QString RESTRICTED_ACCESS_FLAG = "restricted";

    domainObject[RESTRICTED_ACCESS_FLAG] =
        _settingsManager.valueOrDefaultValueForKeyPath(RESTRICTED_ACCESS_SETTINGS_KEYPATH).toBool();

    // add the number of currently connected agent users
    int numConnectedAuthedUsers = 0;

    nodeList->eachNode([&numConnectedAuthedUsers](const SharedNodePointer& node){
        if (node->getLinkedData() && !static_cast<DomainServerNodeData*>(node->getLinkedData())->getUsername().isEmpty()) {
            ++numConnectedAuthedUsers;
        }
    });

    const QString DOMAIN_HEARTBEAT_KEY = "heartbeat";
    const QString HEARTBEAT_NUM_USERS_KEY = "num_users";

    QJsonObject heartbeatObject;
    heartbeatObject[HEARTBEAT_NUM_USERS_KEY] = numConnectedAuthedUsers;
    domainObject[DOMAIN_HEARTBEAT_KEY] = heartbeatObject;

    QString domainUpdateJSON = QString("{\"domain\": %1 }").arg(QString(QJsonDocument(domainObject).toJson()));

    AccountManager::getInstance().sendRequest(DOMAIN_UPDATE.arg(uuidStringWithoutCurlyBraces(domainID)),
                                              AccountManagerAuth::Required,
                                              QNetworkAccessManager::PutOperation,
                                              JSONCallbackParameters(),
                                              domainUpdateJSON.toUtf8());
}

// TODO: have data-web respond with ice-server hostname to use

void DomainServer::sendHeartbeatToIceServer() {
    DependencyManager::get<LimitedNodeList>()->sendHeartbeatToIceServer(_iceServerSocket);
}

void DomainServer::processNodeJSONStatsPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    auto nodeData = dynamic_cast<DomainServerNodeData*>(sendingNode->getLinkedData());
    if (nodeData) {
        nodeData->processJSONStatsPacket(*packet);
    }
}

QJsonObject DomainServer::jsonForSocket(const HifiSockAddr& socket) {
    QJsonObject socketJSON;

    socketJSON["ip"] = socket.getAddress().toString();
    socketJSON["port"] = socket.getPort();

    return socketJSON;
}

const char JSON_KEY_UUID[] = "uuid";
const char JSON_KEY_TYPE[] = "type";
const char JSON_KEY_PUBLIC_SOCKET[] = "public";
const char JSON_KEY_LOCAL_SOCKET[] = "local";
const char JSON_KEY_POOL[] = "pool";
const char JSON_KEY_PENDING_CREDITS[] = "pending_credits";
const char JSON_KEY_UPTIME[] = "uptime";
const char JSON_KEY_USERNAME[] = "username";
const char JSON_KEY_VERSION[] = "version";
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
    nodeJson[JSON_KEY_UPTIME] = QString::number(double(QDateTime::currentMSecsSinceEpoch() - node->getWakeTimestamp()) / 1000.0);

    // if the node has pool information, add it
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());

    // add the node username, if it exists
    nodeJson[JSON_KEY_USERNAME] = nodeData->getUsername();
    nodeJson[JSON_KEY_VERSION] = nodeData->getNodeVersion();

    SharedAssignmentPointer matchingAssignment = _allAssignments.value(nodeData->getAssignmentUUID());
    if (matchingAssignment) {
        nodeJson[JSON_KEY_POOL] = matchingAssignment->getPool();

        if (!nodeData->getWalletUUID().isNull()) {
            TransactionHash::iterator i = _pendingAssignmentCredits.find(nodeData->getWalletUUID());
            float pendingCreditAmount = 0;

            while (i != _pendingAssignmentCredits.end() && i.key() == nodeData->getWalletUUID()) {
                pendingCreditAmount += i.value()->getAmount() / SATOSHIS_PER_CREDIT;
                ++i;
            }

            nodeJson[JSON_KEY_PENDING_CREDITS] = pendingCreditAmount;
        }
    }

    return nodeJson;
}

const char ASSIGNMENT_SCRIPT_HOST_LOCATION[] = "resources/web/assignment";
QString pathForAssignmentScript(const QUuid& assignmentUUID) {
    QString newPath(ASSIGNMENT_SCRIPT_HOST_LOCATION);
    newPath += "/scripts/";
    // append the UUID for this script as the new filename, remove the curly braces
    newPath += uuidStringWithoutCurlyBraces(assignmentUUID);
    return newPath;
}

const QString URI_OAUTH = "/oauth";
bool DomainServer::handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) {
    const QString JSON_MIME_TYPE = "application/json";

    const QString URI_ASSIGNMENT = "/assignment";
    const QString URI_ASSIGNMENT_SCRIPTS = URI_ASSIGNMENT + "/scripts";
    const QString URI_NODES = "/nodes";
    const QString URI_SETTINGS = "/settings";

    const QString UUID_REGEX_STRING = "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}";

    auto nodeList = DependencyManager::get<LimitedNodeList>();

    // allow sub-handlers to handle requests that do not require authentication
    if (_settingsManager.handlePublicHTTPRequest(connection, url)) {
        return true;
    }

    // check if this is a request for a scripted assignment (with a temp unique UUID)
    const QString ASSIGNMENT_REGEX_STRING = QString("\\%1\\/(%2)\\/?$").arg(URI_ASSIGNMENT).arg(UUID_REGEX_STRING);
    QRegExp assignmentRegex(ASSIGNMENT_REGEX_STRING);

    if (connection->requestOperation() == QNetworkAccessManager::GetOperation
        && assignmentRegex.indexIn(url.path()) != -1) {
        QUuid matchingUUID = QUuid(assignmentRegex.cap(1));

        SharedAssignmentPointer matchingAssignment = _allAssignments.value(matchingUUID);
        if (!matchingAssignment) {
            // check if we have a pending assignment that matches this temp UUID, and it is a scripted assignment
            QUuid assignmentUUID = _gatekeeper.assignmentUUIDForPendingAssignment(matchingUUID);
            if (!assignmentUUID.isNull()) {
                matchingAssignment = _allAssignments.value(assignmentUUID);

                if (matchingAssignment && matchingAssignment->getType() == Assignment::AgentType) {
                    // we have a matching assignment and it is for the right type, have the HTTP manager handle it
                    // via correct URL for the script so the client can download

                    QUrl scriptURL = url;
                    scriptURL.setPath(URI_ASSIGNMENT + "/scripts/"
                                      + uuidStringWithoutCurlyBraces(assignmentUUID));

                    // have the HTTPManager serve the appropriate script file
                    return _httpManager.handleHTTPRequest(connection, scriptURL, true);
                }
            }
        }

        // request not handled
        return false;
    }

    // check if this is a request for our domain ID
    const QString URI_ID = "/id";
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation
        && url.path() == URI_ID) {
        QUuid domainID = nodeList->getSessionUUID();

        connection->respond(HTTPConnection::StatusCode200, uuidStringWithoutCurlyBraces(domainID).toLocal8Bit());
        return true;
    }

    // all requests below require a cookie to prove authentication so check that first
    if (!isAuthenticatedRequest(connection, url)) {
        // this is not an authenticated request
        // return true from the handler since it was handled with a 401 or re-direct to auth
        return true;
    }

    if (connection->requestOperation() == QNetworkAccessManager::GetOperation) {
        if (url.path() == "/assignments.json") {
            // user is asking for json list of assignments

            // setup the JSON
            QJsonObject assignmentJSON;
            QJsonObject assignedNodesJSON;

            // enumerate the NodeList to find the assigned nodes
            nodeList->eachNode([this, &assignedNodesJSON](const SharedNodePointer& node){
                DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());

                if (!nodeData->getAssignmentUUID().isNull()) {
                    // add the node using the UUID as the key
                    QString uuidString = uuidStringWithoutCurlyBraces(nodeData->getAssignmentUUID());
                    assignedNodesJSON[uuidString] = jsonObjectForNode(node);
                }
            });

            assignmentJSON["fulfilled"] = assignedNodesJSON;

            QJsonObject queuedAssignmentsJSON;

            // add the queued but unfilled assignments to the json
            foreach(const SharedAssignmentPointer& assignment, _unfulfilledAssignments) {
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
        } else if (url.path() == "/transactions.json") {
            // enumerate our pending transactions and display them in an array
            QJsonObject rootObject;
            QJsonArray transactionArray;

            TransactionHash::iterator i = _pendingAssignmentCredits.begin();
            while (i != _pendingAssignmentCredits.end()) {
                transactionArray.push_back(i.value()->toJson());
                ++i;
            }

            rootObject["pending_transactions"] = transactionArray;

            // print out the created JSON
            QJsonDocument transactionsDocument(rootObject);
            connection->respond(HTTPConnection::StatusCode200, transactionsDocument.toJson(), qPrintable(JSON_MIME_TYPE));

            return true;
        } else if (url.path() == QString("%1.json").arg(URI_NODES)) {
            // setup the JSON
            QJsonObject rootJSON;
            QJsonArray nodesJSONArray;

            // enumerate the NodeList to find the assigned nodes
            nodeList->eachNode([this, &nodesJSONArray](const SharedNodePointer& node){
                // add the node using the UUID as the key
                nodesJSONArray.append(jsonObjectForNode(node));
            });

            rootJSON["nodes"] = nodesJSONArray;

            // print out the created JSON
            QJsonDocument nodesDocument(rootJSON);

            // send the response
            connection->respond(HTTPConnection::StatusCode200, nodesDocument.toJson(), qPrintable(JSON_MIME_TYPE));

            return true;
        } else {
            // check if this is for json stats for a node
            const QString NODE_JSON_REGEX_STRING = QString("\\%1\\/(%2).json\\/?$").arg(URI_NODES).arg(UUID_REGEX_STRING);
            QRegExp nodeShowRegex(NODE_JSON_REGEX_STRING);

            if (nodeShowRegex.indexIn(url.path()) != -1) {
                QUuid matchingUUID = QUuid(nodeShowRegex.cap(1));

                // see if we have a node that matches this ID
                SharedNodePointer matchingNode = nodeList->nodeWithUUID(matchingUUID);
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

                return false;
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
                if (scriptFile.open(QIODevice::WriteOnly)) {
                    scriptFile.write(formData[0].second);

                    qDebug() << qPrintable(QString("Saved a script for assignment at %1%2")
                                           .arg(newPath).arg(assignmentPool == emptyPool ? "" : " - pool is " + assignmentPool));

                    // add the script assigment to the assignment queue
                    SharedAssignmentPointer sharedScriptedAssignment(scriptAssignment);
                    _unfulfilledAssignments.enqueue(sharedScriptedAssignment);
                    _allAssignments.insert(sharedScriptedAssignment->getUUID(), sharedScriptedAssignment);
                } else {
                    // unable to save script for assignment - we shouldn't be here but debug it out
                    qDebug() << "Unable to save a script for assignment at" << newPath;
                    qDebug() << "Script will not be added to queue";
                }
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

            SharedNodePointer nodeToKill = nodeList->nodeWithUUID(deleteUUID);

            if (nodeToKill) {
                // start with a 200 response
                connection->respond(HTTPConnection::StatusCode200);

                // we have a valid UUID and node - kill the node that has this assignment
                QMetaObject::invokeMethod(nodeList.data(), "killNodeWithUUID", Q_ARG(const QUuid&, deleteUUID));

                // successfully processed request
                return true;
            }

            return true;
        } else if (allNodesDeleteRegex.indexIn(url.path()) != -1) {
            qDebug() << "Received request to kill all nodes.";
            nodeList->eraseAllNodes();

            return true;
        }
    }

    // didn't process the request, let our DomainServerSettingsManager or HTTPManager handle
    return _settingsManager.handleAuthenticatedHTTPRequest(connection, url);
}

const QString HIFI_SESSION_COOKIE_KEY = "DS_WEB_SESSION_UUID";

bool DomainServer::handleHTTPSRequest(HTTPSConnection* connection, const QUrl &url, bool skipSubHandler) {
    qDebug() << "HTTPS request received at" << url.toString();
    if (url.path() == URI_OAUTH) {

        QUrlQuery codeURLQuery(url);

        const QString CODE_QUERY_KEY = "code";
        QString authorizationCode = codeURLQuery.queryItemValue(CODE_QUERY_KEY);

        const QString STATE_QUERY_KEY = "state";
        QUuid stateUUID = QUuid(codeURLQuery.queryItemValue(STATE_QUERY_KEY));

        if (!authorizationCode.isEmpty() && !stateUUID.isNull()) {
            // fire off a request with this code and state to get an access token for the user

            const QString OAUTH_TOKEN_REQUEST_PATH = "/oauth/token";
            QUrl tokenRequestUrl = _oauthProviderURL;
            tokenRequestUrl.setPath(OAUTH_TOKEN_REQUEST_PATH);

            const QString OAUTH_GRANT_TYPE_POST_STRING = "grant_type=authorization_code";
            QString tokenPostBody = OAUTH_GRANT_TYPE_POST_STRING;
            tokenPostBody += QString("&code=%1&redirect_uri=%2&client_id=%3&client_secret=%4")
                .arg(authorizationCode, oauthRedirectURL().toString(), _oauthClientID, _oauthClientSecret);

            QNetworkRequest tokenRequest(tokenRequestUrl);
            tokenRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
            tokenRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

            QNetworkReply* tokenReply = NetworkAccessManager::getInstance().post(tokenRequest, tokenPostBody.toLocal8Bit());

            if (_webAuthenticationStateSet.remove(stateUUID)) {
                // this is a web user who wants to auth to access web interface
                // we hold the response back to them until we get their profile information
                // and can decide if they are let in or not

                QEventLoop loop;
                connect(tokenReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

                // start the loop for the token request
                loop.exec();

                QNetworkReply* profileReply = profileRequestGivenTokenReply(tokenReply);

                // stop the loop once the profileReply is complete
                connect(profileReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

                // restart the loop for the profile request
                loop.exec();

                // call helper method to get cookieHeaders
                Headers cookieHeaders = setupCookieHeadersFromProfileReply(profileReply);

                connection->respond(HTTPConnection::StatusCode302, QByteArray(),
                                    HTTPConnection::DefaultContentType, cookieHeaders);

                delete tokenReply;
                delete profileReply;

                // we've redirected the user back to our homepage
                return true;

            }
        }

        // respond with a 200 code indicating that login is complete
        connection->respond(HTTPConnection::StatusCode200);

        return true;
    } else {
        return false;
    }
}

bool DomainServer::isAuthenticatedRequest(HTTPConnection* connection, const QUrl& url) {

    const QByteArray HTTP_COOKIE_HEADER_KEY = "Cookie";
    const QString ADMIN_USERS_CONFIG_KEY = "admin-users";
    const QString ADMIN_ROLES_CONFIG_KEY = "admin-roles";
    const QString BASIC_AUTH_USERNAME_KEY_PATH = "security.http_username";
    const QString BASIC_AUTH_PASSWORD_KEY_PATH = "security.http_password";

    const QByteArray UNAUTHENTICATED_BODY = "You do not have permission to access this domain-server.";

    QVariantMap& settingsMap = _settingsManager.getSettingsMap();

    if (!_oauthProviderURL.isEmpty()
        && (settingsMap.contains(ADMIN_USERS_CONFIG_KEY) || settingsMap.contains(ADMIN_ROLES_CONFIG_KEY))) {
        QString cookieString = connection->requestHeaders().value(HTTP_COOKIE_HEADER_KEY);

        const QString COOKIE_UUID_REGEX_STRING = HIFI_SESSION_COOKIE_KEY + "=([\\d\\w-]+)($|;)";
        QRegExp cookieUUIDRegex(COOKIE_UUID_REGEX_STRING);

        QUuid cookieUUID;
        if (cookieString.indexOf(cookieUUIDRegex) != -1) {
            cookieUUID = cookieUUIDRegex.cap(1);
        }

        if (valueForKeyPath(settingsMap, BASIC_AUTH_USERNAME_KEY_PATH)) {
            qDebug() << "Config file contains web admin settings for OAuth and basic HTTP authentication."
                << "These cannot be combined - using OAuth for authentication.";
        }

        if (!cookieUUID.isNull() && _cookieSessionHash.contains(cookieUUID)) {
            // pull the QJSONObject for the user with this cookie UUID
            DomainServerWebSessionData sessionData = _cookieSessionHash.value(cookieUUID);
            QString profileUsername = sessionData.getUsername();

            if (settingsMap.value(ADMIN_USERS_CONFIG_KEY).toStringList().contains(profileUsername)) {
                // this is an authenticated user
                return true;
            }

            // loop the roles of this user and see if they are in the admin-roles array
            QStringList adminRolesArray = settingsMap.value(ADMIN_ROLES_CONFIG_KEY).toStringList();

            if (!adminRolesArray.isEmpty()) {
                foreach(const QString& userRole, sessionData.getRoles()) {
                    if (adminRolesArray.contains(userRole)) {
                        // this user has a role that allows them to administer the domain-server
                        return true;
                    }
                }
            }

            connection->respond(HTTPConnection::StatusCode401, UNAUTHENTICATED_BODY);

            // the user does not have allowed username or role, return 401
            return false;
        } else {
            // re-direct this user to OAuth page

            // generate a random state UUID to use
            QUuid stateUUID = QUuid::createUuid();

            // add it to the set so we can handle the callback from the OAuth provider
            _webAuthenticationStateSet.insert(stateUUID);

            QUrl oauthRedirectURL = oauthAuthorizationURL(stateUUID);

            Headers redirectHeaders;
            redirectHeaders.insert("Location", oauthRedirectURL.toEncoded());

            connection->respond(HTTPConnection::StatusCode302,
                                QByteArray(), HTTPConnection::DefaultContentType, redirectHeaders);

            // we don't know about this user yet, so they are not yet authenticated
            return false;
        }
    } else if (valueForKeyPath(settingsMap, BASIC_AUTH_USERNAME_KEY_PATH)) {
        // config file contains username and password combinations for basic auth
        const QByteArray BASIC_AUTH_HEADER_KEY = "Authorization";

        // check if a username and password have been provided with the request
        QString basicAuthString = connection->requestHeaders().value(BASIC_AUTH_HEADER_KEY);

        if (!basicAuthString.isEmpty()) {
            QStringList splitAuthString = basicAuthString.split(' ');
            QString base64String = splitAuthString.size() == 2 ? splitAuthString[1] : "";
            QString credentialString = QByteArray::fromBase64(base64String.toLocal8Bit());

            if (!credentialString.isEmpty()) {
                QStringList credentialList = credentialString.split(':');
                if (credentialList.size() == 2) {
                    QString headerUsername = credentialList[0];
                    QString headerPassword = credentialList[1];

                    // we've pulled a username and password - now check if there is a match in our basic auth hash
                    QString settingsUsername = valueForKeyPath(settingsMap, BASIC_AUTH_USERNAME_KEY_PATH)->toString();
                    const QVariant* settingsPasswordVariant = valueForKeyPath(settingsMap, BASIC_AUTH_PASSWORD_KEY_PATH);
                    QString settingsPassword = settingsPasswordVariant ? settingsPasswordVariant->toString() : "";

                    if (settingsUsername == headerUsername && headerPassword == settingsPassword) {
                        return true;
                    }
                }
            }
        }

        // basic HTTP auth being used but no username and password are present
        // or the username and password are not correct
        // send back a 401 and ask for basic auth

        const QByteArray HTTP_AUTH_REQUEST_HEADER_KEY = "WWW-Authenticate";
        static QString HTTP_AUTH_REALM_STRING = QString("Basic realm='%1 %2'")
            .arg(_hostname.isEmpty() ? "localhost" : _hostname)
            .arg("domain-server");

        Headers basicAuthHeader;
        basicAuthHeader.insert(HTTP_AUTH_REQUEST_HEADER_KEY, HTTP_AUTH_REALM_STRING.toUtf8());

        connection->respond(HTTPConnection::StatusCode401, UNAUTHENTICATED_BODY,
                            HTTPConnection::DefaultContentType, basicAuthHeader);

        // not authenticated, bubble up false
        return false;

    } else {
        // we don't have an OAuth URL + admin roles/usernames, so all users are authenticated
        return true;
    }
}

const QString OAUTH_JSON_ACCESS_TOKEN_KEY = "access_token";
QNetworkReply* DomainServer::profileRequestGivenTokenReply(QNetworkReply* tokenReply) {
    // pull the access token from the returned JSON and store it with the matching session UUID
    QJsonDocument returnedJSON = QJsonDocument::fromJson(tokenReply->readAll());
    QString accessToken = returnedJSON.object()[OAUTH_JSON_ACCESS_TOKEN_KEY].toString();

    // fire off a request to get this user's identity so we can see if we will let them in
    QUrl profileURL = _oauthProviderURL;
    profileURL.setPath("/api/v1/user/profile");
    profileURL.setQuery(QString("%1=%2").arg(OAUTH_JSON_ACCESS_TOKEN_KEY, accessToken));

    QNetworkRequest profileRequest(profileURL);
    profileRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    return NetworkAccessManager::getInstance().get(profileRequest);
}

const QString DS_SETTINGS_SESSIONS_GROUP = "web-sessions";
Headers DomainServer::setupCookieHeadersFromProfileReply(QNetworkReply* profileReply) {
    Headers cookieHeaders;

    // create a UUID for this cookie
    QUuid cookieUUID = QUuid::createUuid();

    QJsonDocument profileDocument = QJsonDocument::fromJson(profileReply->readAll());
    QJsonObject userObject = profileDocument.object()["data"].toObject()["user"].toObject();

    // add the profile to our in-memory data structure so we know who the user is when they send us their cookie
    DomainServerWebSessionData sessionData(userObject);
    _cookieSessionHash.insert(cookieUUID, sessionData);

    // persist the cookie to settings file so we can get it back on DS relaunch
    QStringList path = QStringList() << DS_SETTINGS_SESSIONS_GROUP << cookieUUID.toString();
    Setting::Handle<QVariant>(path).set(QVariant::fromValue(sessionData));

    // setup expiry for cookie to 1 month from today
    QDateTime cookieExpiry = QDateTime::currentDateTimeUtc().addMonths(1);

    QString cookieString = HIFI_SESSION_COOKIE_KEY + "=" + uuidStringWithoutCurlyBraces(cookieUUID.toString());
    cookieString += "; expires=" + cookieExpiry.toString("ddd, dd MMM yyyy HH:mm:ss") + " GMT";
    cookieString += "; domain=" + _hostname + "; path=/";

    cookieHeaders.insert("Set-Cookie", cookieString.toUtf8());

    // redirect the user back to the homepage so they can present their cookie and be authenticated
    QString redirectString = "http://" + _hostname + ":" + QString::number(_httpManager.serverPort());
    cookieHeaders.insert("Location", redirectString.toUtf8());

    return cookieHeaders;
}

void DomainServer::loadExistingSessionsFromSettings() {
    // read data for existing web sessions into memory so existing sessions can be leveraged
    Settings domainServerSettings;
    domainServerSettings.beginGroup(DS_SETTINGS_SESSIONS_GROUP);

    foreach(const QString& uuidKey, domainServerSettings.childKeys()) {
        _cookieSessionHash.insert(QUuid(uuidKey),
                                  domainServerSettings.value(uuidKey).value<DomainServerWebSessionData>());
        qDebug() << "Pulled web session from settings - cookie UUID is" << uuidKey;
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
    _allAssignments.insert(assignment->getUUID(), assignment);
    _unfulfilledAssignments.enqueue(assignment);
}

void DomainServer::nodeAdded(SharedNodePointer node) {
    // we don't use updateNodeWithData, so add the DomainServerNodeData to the node here
    node->setLinkedData(new DomainServerNodeData());
}

void DomainServer::nodeKilled(SharedNodePointer node) {

    // if this peer connected via ICE then remove them from our ICE peers hash
    _gatekeeper.removeICEPeer(node->getUUID());

    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());

    if (nodeData) {
        // if this node's UUID matches a static assignment we need to throw it back in the assignment queue
        if (!nodeData->getAssignmentUUID().isNull()) {
            SharedAssignmentPointer matchedAssignment = _allAssignments.take(nodeData->getAssignmentUUID());

            if (matchedAssignment && matchedAssignment->isStatic()) {
                refreshStaticAssignmentAndAddToQueue(matchedAssignment);
            }
        }

        // If this node was an Agent ask JSONBreakableMarshal to potentially remove the interpolation we stored
        JSONBreakableMarshal::removeInterpolationForKey(USERNAME_UUID_REPLACEMENT_STATS_KEY,
                uuidStringWithoutCurlyBraces(node->getUUID()));

        // cleanup the connection secrets that we set up for this node (on the other nodes)
        foreach (const QUuid& otherNodeSessionUUID, nodeData->getSessionSecretHash().keys()) {
            SharedNodePointer otherNode = DependencyManager::get<LimitedNodeList>()->nodeWithUUID(otherNodeSessionUUID);
            if (otherNode) {
                reinterpret_cast<DomainServerNodeData*>(otherNode->getLinkedData())->getSessionSecretHash().remove(node->getUUID());
            }
        }
    }
}

SharedAssignmentPointer DomainServer::dequeueMatchingAssignment(const QUuid& assignmentUUID, NodeType_t nodeType) {
    QQueue<SharedAssignmentPointer>::iterator i = _unfulfilledAssignments.begin();

    while (i != _unfulfilledAssignments.end()) {
        if (i->data()->getType() == Assignment::typeForNodeType(nodeType)
            && i->data()->getUUID() == assignmentUUID) {
            // we have an unfulfilled assignment to return

            // return the matching assignment
            return _unfulfilledAssignments.takeAt(i - _unfulfilledAssignments.begin());
        } else {
            ++i;
        }
    }

    return SharedAssignmentPointer();
}

SharedAssignmentPointer DomainServer::deployableAssignmentForRequest(const Assignment& requestAssignment) {
    // this is an unassigned client talking to us directly for an assignment
    // go through our queue and see if there are any assignments to give out
    QQueue<SharedAssignmentPointer>::iterator sharedAssignment = _unfulfilledAssignments.begin();

    while (sharedAssignment != _unfulfilledAssignments.end()) {
        Assignment* assignment = sharedAssignment->data();
        bool requestIsAllTypes = requestAssignment.getType() == Assignment::AllTypes;
        bool assignmentTypesMatch = assignment->getType() == requestAssignment.getType();
        bool neitherHasPool = assignment->getPool().isEmpty() && requestAssignment.getPool().isEmpty();
        bool assignmentPoolsMatch = assignment->getPool() == requestAssignment.getPool();

        if ((requestIsAllTypes || assignmentTypesMatch) && (neitherHasPool || assignmentPoolsMatch)) {

            // remove the assignment from the queue
            SharedAssignmentPointer deployableAssignment = _unfulfilledAssignments.takeAt(sharedAssignment
                                                                                          - _unfulfilledAssignments.begin());

            // until we get a connection for this assignment
            // put assignment back in queue but stick it at the back so the others have a chance to go out
            _unfulfilledAssignments.enqueue(deployableAssignment);

            // stop looping, we've handed out an assignment
            return deployableAssignment;
        } else {
            // push forward the iterator to check the next assignment
            ++sharedAssignment;
        }
    }

    return SharedAssignmentPointer();
}

void DomainServer::addStaticAssignmentsToQueue() {

    // if the domain-server has just restarted,
    // check if there are static assignments that we need to throw into the assignment queue
    QHash<QUuid, SharedAssignmentPointer> staticHashCopy = _allAssignments;
    QHash<QUuid, SharedAssignmentPointer>::iterator staticAssignment = staticHashCopy.begin();
    while (staticAssignment != staticHashCopy.end()) {
        // add any of the un-matched static assignments to the queue

        // enumerate the nodes and check if there is one with an attached assignment with matching UUID
        if (!DependencyManager::get<LimitedNodeList>()->nodeWithUUID(staticAssignment->data()->getUUID())) {
            // this assignment has not been fulfilled - reset the UUID and add it to the assignment queue
            refreshStaticAssignmentAndAddToQueue(*staticAssignment);
        }

        ++staticAssignment;
    }
}

void DomainServer::processPathQueryPacket(QSharedPointer<NLPacket> packet) {
    // this is a query for the viewpoint resulting from a path
    // first pull the query path from the packet

    // figure out how many bytes the sender said this path is
    quint16 numPathBytes;
    packet->readPrimitive(&numPathBytes);

    if (numPathBytes <= packet->bytesLeftToRead()) {
        // the number of path bytes makes sense for the sent packet - pull out the path
        QString pathQuery = QString::fromUtf8(packet->getPayload() + packet->pos(), numPathBytes);

        // our settings contain paths that start with a leading slash, so make sure this query has that
        if (!pathQuery.startsWith("/")) {
            pathQuery.prepend("/");
        }

        const QString PATHS_SETTINGS_KEYPATH_FORMAT = "%1.%2";
        const QString PATH_VIEWPOINT_KEY = "viewpoint";

        // check out paths in the _configMap to see if we have a match
        const QVariant* pathMatch = valueForKeyPath(_settingsManager.getSettingsMap(),
                                                    QString(PATHS_SETTINGS_KEYPATH_FORMAT).arg(SETTINGS_PATHS_KEY)
                                                                                          .arg(pathQuery));
        if (pathMatch) {
            // we got a match, respond with the resulting viewpoint
            auto nodeList = DependencyManager::get<LimitedNodeList>();

            QString responseViewpoint = pathMatch->toMap()[PATH_VIEWPOINT_KEY].toString();

            if (!responseViewpoint.isEmpty()) {
                QByteArray viewpointUTF8 = responseViewpoint.toUtf8();

                // prepare a packet for the response
                auto pathResponsePacket = NLPacket::create(PacketType::DomainServerPathResponse);

                // check the number of bytes the viewpoint is
                quint16 numViewpointBytes = viewpointUTF8.size();

                // are we going to be able to fit this response viewpoint in a packet?
                if (numPathBytes + numViewpointBytes + sizeof(numViewpointBytes) + sizeof(numPathBytes)
                        < (unsigned long) pathResponsePacket->bytesAvailableForWrite()) {
                    // append the number of bytes this path is
                    pathResponsePacket->writePrimitive(numPathBytes);

                    // append the path itself
                    pathResponsePacket->write(pathQuery.toUtf8());

                    // append the number of bytes the resulting viewpoint is
                    pathResponsePacket->writePrimitive(numViewpointBytes);

                    // append the viewpoint itself
                    pathResponsePacket->write(viewpointUTF8);

                    qDebug() << "Sending a viewpoint response for path query" << pathQuery << "-" << viewpointUTF8;

                    // send off the packet - see if we can associate this outbound data to a particular node
                    // TODO: does this senderSockAddr always work for a punched DS client?
                    nodeList->sendPacket(std::move(pathResponsePacket), packet->getSenderSockAddr());
                }
            }

        } else {
            // we don't respond if there is no match - this may need to change once this packet
            // query/response is made reliable
            qDebug() << "No match for path query" << pathQuery << "- refusing to respond.";
        }
    }
}
