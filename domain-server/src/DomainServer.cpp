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

#include <memory>
#include <random>

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
#include <BuildInfo.h>
#include <DependencyManager.h>
#include <HifiConfigVariantMap.h>
#include <HTTPConnection.h>
#include <LogUtils.h>
#include <NetworkingConstants.h>
#include <udt/PacketHeaders.h>
#include <SettingHandle.h>
#include <SharedUtil.h>
#include <ShutdownEventListener.h>
#include <UUID.h>
#include <LogHandler.h>
#include <ServerPathUtils.h>

#include "DomainServerNodeData.h"
#include "NodeConnectionData.h"

int const DomainServer::EXIT_CODE_REBOOT = 234923;

const QString ICE_SERVER_DEFAULT_HOSTNAME = "ice.highfidelity.com";

DomainServer::DomainServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _gatekeeper(this),
    _httpManager(QHostAddress::AnyIPv4, DOMAIN_SERVER_HTTP_PORT, QString("%1/resources/web/").arg(QCoreApplication::applicationDirPath()), this),
    _httpsManager(NULL),
    _allAssignments(),
    _unfulfilledAssignments(),
    _isUsingDTLS(false),
    _oauthProviderURL(),
    _oauthClientID(),
    _hostname(),
    _ephemeralACScripts(),
    _webAuthenticationStateSet(),
    _cookieSessionHash(),
    _automaticNetworkingSetting(),
    _settingsManager()
{
    qInstallMessageHandler(LogHandler::verboseMessageHandler);

    LogUtils::init();
    Setting::init();

    connect(this, &QCoreApplication::aboutToQuit, this, &DomainServer::aboutToQuit);

    setOrganizationName(BuildInfo::MODIFIED_ORGANIZATION);
    setOrganizationDomain("highfidelity.io");
    setApplicationName("domain-server");
    setApplicationVersion(BuildInfo::VERSION);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    qDebug() << "Setting up domain-server";

    // make sure we have a fresh AccountManager instance
    // (need this since domain-server can restart itself and maintain static variables)
    DependencyManager::set<AccountManager>();

    auto args = arguments();

    _settingsManager.setupConfigMap(args);

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

    // if a connected node loses connection privileges, hang up on it
    connect(&_gatekeeper, &DomainGatekeeper::killNode, this, &DomainServer::handleKillNode);

    // if permissions are updated, relay the changes to the Node datastructures
    connect(&_settingsManager, &DomainServerSettingsManager::updateNodePermissions,
            &_gatekeeper, &DomainGatekeeper::updateNodePermissions);

    // if we were given a certificate/private key or oauth credentials they must succeed
    if (!(optionallyReadX509KeyAndCertificate() && optionallySetupOAuth())) {
        return;
    }

    setupNodeListAndAssignments();
    setupAutomaticNetworking();
    if (!getID().isNull()) {
        setupHeartbeatToMetaverse();
        // send the first heartbeat immediately
        sendHeartbeatToMetaverse();
    }

    // check for the temporary name parameter
    const QString GET_TEMPORARY_NAME_SWITCH = "--get-temp-name";
    if (args.contains(GET_TEMPORARY_NAME_SWITCH)) {
        getTemporaryName();
    }

    _gatekeeper.preloadAllowedUserPublicKeys(); // so they can connect on first request

    _metadata = new DomainMetadata(this);


    qDebug() << "domain-server is running";
}

DomainServer::~DomainServer() {
    // destroy the LimitedNodeList before the DomainServer QCoreApplication is down
    DependencyManager::destroy<LimitedNodeList>();
}

void DomainServer::queuedQuit(QString quitMessage, int exitCode) {
    if (!quitMessage.isEmpty()) {
        qCritical() << qPrintable(quitMessage);
    }

    QCoreApplication::exit(exitCode);
}

void DomainServer::aboutToQuit() {

    // clear the log handler so that Qt doesn't call the destructor on LogHandler
    qInstallMessageHandler(0);
}

void DomainServer::restart() {
    qDebug() << "domain-server is restarting.";

    exit(DomainServer::EXIT_CODE_REBOOT);
}

const QUuid& DomainServer::getID() {
    return DependencyManager::get<LimitedNodeList>()->getSessionUUID();
}

bool DomainServer::optionallyReadX509KeyAndCertificate() {
    const QString X509_CERTIFICATE_OPTION = "cert";
    const QString X509_PRIVATE_KEY_OPTION = "key";
    const QString X509_KEY_PASSPHRASE_ENV = "DOMAIN_SERVER_KEY_PASSPHRASE";

    QString certPath = _settingsManager.getSettingsMap().value(X509_CERTIFICATE_OPTION).toString();
    QString keyPath = _settingsManager.getSettingsMap().value(X509_PRIVATE_KEY_OPTION).toString();

    if (!certPath.isEmpty() && !keyPath.isEmpty()) {
        // the user wants to use the following cert and key for HTTPS
        // this is used for Oauth callbacks when authorizing users against a data server
        // let's make sure we can load the key and certificate

        QString keyPassphraseString = QProcessEnvironment::systemEnvironment().value(X509_KEY_PASSPHRASE_ENV);

        qDebug() << "Reading certificate file at" << certPath << "for HTTPS.";
        qDebug() << "Reading key file at" << keyPath << "for HTTPS.";    

        QFile certFile(certPath);
        certFile.open(QIODevice::ReadOnly);

        QFile keyFile(keyPath);
        keyFile.open(QIODevice::ReadOnly);

        QSslCertificate sslCertificate(&certFile);
        QSslKey privateKey(&keyFile, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, keyPassphraseString.toUtf8());

        _httpsManager = new HTTPSManager(QHostAddress::AnyIPv4, DOMAIN_SERVER_HTTPS_PORT, sslCertificate, privateKey, QString(), this, this);

        qDebug() << "TCP server listening for HTTPS connections on" << DOMAIN_SERVER_HTTPS_PORT;

    } else if (!certPath.isEmpty() || !keyPath.isEmpty()) {
        static const QString MISSING_CERT_ERROR_MSG = "Missing certificate or private key. domain-server will now quit.";
        static const int MISSING_CERT_ERROR_CODE = 3;

        QMetaObject::invokeMethod(this, "queuedQuit", Qt::QueuedConnection,
                                  Q_ARG(QString, MISSING_CERT_ERROR_MSG), Q_ARG(int, MISSING_CERT_ERROR_CODE));
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

    auto accountManager = DependencyManager::get<AccountManager>();
    accountManager->setAuthURL(_oauthProviderURL);

    _oauthClientID = settingsMap.value(OAUTH_CLIENT_ID_OPTION).toString();
    _oauthClientSecret = QProcessEnvironment::systemEnvironment().value(OAUTH_CLIENT_SECRET_ENV);
    _hostname = settingsMap.value(REDIRECT_HOSTNAME_OPTION).toString();

    if (!_oauthClientID.isEmpty()) {
        if (_oauthProviderURL.isEmpty()
            || _hostname.isEmpty()
            || _oauthClientID.isEmpty()
            || _oauthClientSecret.isEmpty()) {
            static const QString MISSING_OAUTH_INFO_MSG = "Missing OAuth provider URL, hostname, client ID, or client secret. domain-server will now quit.";
            static const int MISSING_OAUTH_INFO_ERROR_CODE = 4;
            QMetaObject::invokeMethod(this, "queuedQuit", Qt::QueuedConnection,
                                      Q_ARG(QString, MISSING_OAUTH_INFO_MSG), Q_ARG(int, MISSING_OAUTH_INFO_ERROR_CODE));
            return false;
        } else {
            qDebug() << "OAuth will be used to identify clients using provider at" << _oauthProviderURL.toString();
            qDebug() << "OAuth Client ID is" << _oauthClientID;
        }
    }

    return true;
}

static const QString METAVERSE_DOMAIN_ID_KEY_PATH = "metaverse.id";

void DomainServer::getTemporaryName(bool force) {
    // check if we already have a domain ID
    const QVariant* idValueVariant = valueForKeyPath(_settingsManager.getSettingsMap(), METAVERSE_DOMAIN_ID_KEY_PATH);

    qInfo() << "Requesting temporary domain name";
    if (idValueVariant) {
        qDebug() << "A domain ID is already present in domain-server settings:" << idValueVariant->toString();
        if (force) {
            qDebug() << "Requesting temporary domain name to replace current ID:" << getID();
        } else {
            qInfo() << "Abandoning request of temporary domain name.";
            return;
        }
    }

    // request a temporary name from the metaverse
    auto accountManager = DependencyManager::get<AccountManager>();
    JSONCallbackParameters callbackParameters { this, "handleTempDomainSuccess", this, "handleTempDomainError" };
    accountManager->sendRequest("/api/v1/domains/temporary", AccountManagerAuth::None,
                                QNetworkAccessManager::PostOperation, callbackParameters);
}

void DomainServer::handleTempDomainSuccess(QNetworkReply& requestReply) {
    QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();

    // grab the information for the new domain
    static const QString DATA_KEY = "data";
    static const QString DOMAIN_KEY = "domain";
    static const QString ID_KEY = "id";
    static const QString NAME_KEY = "name";
    static const QString KEY_KEY = "api_key";

    auto domainObject = jsonObject[DATA_KEY].toObject()[DOMAIN_KEY].toObject();
    if (!domainObject.isEmpty()) {
        auto id = domainObject[ID_KEY].toString();
        auto name = domainObject[NAME_KEY].toString();
        auto key = domainObject[KEY_KEY].toString();

        qInfo() << "Received new temporary domain name" << name;
        qDebug() << "The temporary domain ID is" << id;

        // store the new domain ID and auto network setting immediately
        QString newSettingsJSON = QString("{\"metaverse\": { \"id\": \"%1\", \"automatic_networking\": \"full\"}}").arg(id);
        auto settingsDocument = QJsonDocument::fromJson(newSettingsJSON.toUtf8());
        _settingsManager.recurseJSONObjectAndOverwriteSettings(settingsDocument.object());

        // store the new ID and auto networking setting on disk
        _settingsManager.persistToFile();

        // change our domain ID immediately
        DependencyManager::get<LimitedNodeList>()->setSessionUUID(QUuid { id });

        // store the new token to the account info
        auto accountManager = DependencyManager::get<AccountManager>();
        accountManager->setTemporaryDomain(id, key);

        // update our heartbeats to use the correct id
        setupICEHeartbeatForFullNetworking();
        setupHeartbeatToMetaverse();
    } else {
        qWarning() << "There were problems parsing the API response containing a temporary domain name. Please try again"
            << "via domain-server relaunch or from the domain-server settings.";
    }
}

void DomainServer::handleTempDomainError(QNetworkReply& requestReply) {
    qWarning() << "A temporary name was requested but there was an error creating one. Please try again via domain-server relaunch"
        << "or from the domain-server settings.";
}

const QString DOMAIN_CONFIG_ID_KEY = "id";

const QString METAVERSE_AUTOMATIC_NETWORKING_KEY_PATH = "metaverse.automatic_networking";
const QString FULL_AUTOMATIC_NETWORKING_VALUE = "full";
const QString IP_ONLY_AUTOMATIC_NETWORKING_VALUE = "ip";
const QString DISABLED_AUTOMATIC_NETWORKING_VALUE = "disabled";



bool DomainServer::packetVersionMatch(const udt::Packet& packet) {
    PacketType headerType = NLPacket::typeInHeader(packet);
    PacketVersion headerVersion = NLPacket::versionInHeader(packet);

    auto nodeList = DependencyManager::get<LimitedNodeList>();

    // if this is a mismatching connect packet, we can't simply drop it on the floor
    // send back a packet to the interface that tells them we refuse connection for a mismatch
    if (headerType == PacketType::DomainConnectRequest
        && headerVersion != versionForPacketType(PacketType::DomainConnectRequest)) {
        DomainGatekeeper::sendProtocolMismatchConnectionDenial(packet.getSenderSockAddr());
    }

    // let the normal nodeList implementation handle all other packets.
    return nodeList->isPacketVerified(packet);
}


void DomainServer::setupNodeListAndAssignments() {
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
    const QVariant* idValueVariant = valueForKeyPath(settingsMap, METAVERSE_DOMAIN_ID_KEY_PATH);
    if (idValueVariant) {
        nodeList->setSessionUUID(idValueVariant->toString());
    } else {
        nodeList->setSessionUUID(QUuid::createUuid()); // Use random UUID
    }

    connect(nodeList.data(), &LimitedNodeList::nodeAdded, this, &DomainServer::nodeAdded);
    connect(nodeList.data(), &LimitedNodeList::nodeKilled, this, &DomainServer::nodeKilled);

    // register as the packet receiver for the types we want
    PacketReceiver& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::RequestAssignment, this, "processRequestAssignmentPacket");
    packetReceiver.registerListener(PacketType::DomainListRequest, this, "processListRequestPacket");
    packetReceiver.registerListener(PacketType::DomainServerPathQuery, this, "processPathQueryPacket");
    packetReceiver.registerListener(PacketType::NodeJsonStats, this, "processNodeJSONStatsPacket");
    packetReceiver.registerListener(PacketType::DomainDisconnectRequest, this, "processNodeDisconnectRequestPacket");
    
    // NodeList won't be available to the settings manager when it is created, so call registerListener here
    packetReceiver.registerListener(PacketType::DomainSettingsRequest, &_settingsManager, "processSettingsRequestPacket");
    
    // register the gatekeeper for the packets it needs to receive
    packetReceiver.registerListener(PacketType::DomainConnectRequest, &_gatekeeper, "processConnectRequestPacket");
    packetReceiver.registerListener(PacketType::ICEPing, &_gatekeeper, "processICEPingPacket");
    packetReceiver.registerListener(PacketType::ICEPingReply, &_gatekeeper, "processICEPingReplyPacket");
    packetReceiver.registerListener(PacketType::ICEServerPeerInformation, &_gatekeeper, "processICEPeerInformationPacket");

    packetReceiver.registerListener(PacketType::ICEServerHeartbeatDenied, this, "processICEServerHeartbeatDenialPacket");
    packetReceiver.registerListener(PacketType::ICEServerHeartbeatACK, this, "processICEServerHeartbeatACK");
    
    // add whatever static assignments that have been parsed to the queue
    addStaticAssignmentsToQueue();

    // set a custum packetVersionMatch as the verify packet operator for the udt::Socket
    nodeList->setPacketFilterOperator(&DomainServer::packetVersionMatch);
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
                qDebug() << "A domain-server feature that requires authentication is enabled but no access token is present.";
                qDebug() << "Set an access token via the web interface, in your user or master config"
                    << "at keypath metaverse.access_token or in your ENV at key DOMAIN_SERVER_ACCESS_TOKEN";

                // clear any existing access token from AccountManager
                DependencyManager::get<AccountManager>()->setAccessTokenForCurrentAuthURL(QString());

                return false;
            }
        } else {
            qDebug() << "Using access token from DOMAIN_SERVER_ACCESS_TOKEN in env. This overrides any access token present"
                << " in the user or master config.";
        }

        // give this access token to the AccountManager
        DependencyManager::get<AccountManager>()->setAccessTokenForCurrentAuthURL(accessToken);

        return true;

    } else {
        static const QString MISSING_OAUTH_PROVIDER_MSG =
            QString("Missing OAuth provider URL, but a domain-server feature was required that requires authentication.") +
            QString("domain-server will now quit.");
        static const int MISSING_OAUTH_PROVIDER_ERROR_CODE = 5;
        QMetaObject::invokeMethod(this, "queuedQuit", Qt::QueuedConnection,
                                  Q_ARG(QString, MISSING_OAUTH_PROVIDER_MSG),
                                  Q_ARG(int, MISSING_OAUTH_PROVIDER_ERROR_CODE));

        return false;
    }
}

void DomainServer::setupAutomaticNetworking() {
    qDebug() << "Updating automatic networking setting in domain-server to" << _automaticNetworkingSetting;

    resetAccountManagerAccessToken();

    _automaticNetworkingSetting =
        _settingsManager.valueOrDefaultValueForKeyPath(METAVERSE_AUTOMATIC_NETWORKING_KEY_PATH).toString();

    auto nodeList = DependencyManager::get<LimitedNodeList>();
    const QUuid& domainID = getID();

    if (_automaticNetworkingSetting == FULL_AUTOMATIC_NETWORKING_VALUE) {
        setupICEHeartbeatForFullNetworking();
    }

    if (_automaticNetworkingSetting == IP_ONLY_AUTOMATIC_NETWORKING_VALUE ||
        _automaticNetworkingSetting == FULL_AUTOMATIC_NETWORKING_VALUE) {

        if (!domainID.isNull()) {
            qDebug() << "domain-server" << _automaticNetworkingSetting << "automatic networking enabled for ID"
                << uuidStringWithoutCurlyBraces(domainID) << "via" << _oauthProviderURL.toString();

            if (_automaticNetworkingSetting == IP_ONLY_AUTOMATIC_NETWORKING_VALUE) {
                // send any public socket changes to the data server so nodes can find us at our new IP
                connect(nodeList.data(), &LimitedNodeList::publicSockAddrChanged,
                        this, &DomainServer::performIPAddressUpdate);

                // have the LNL enable public socket updating via STUN
                nodeList->startSTUNPublicSocketUpdate();
            }
        } else {
            qDebug() << "Cannot enable domain-server automatic networking without a domain ID."
            << "Please add an ID to your config file or via the web interface.";

            return;
        }
    }
}

void DomainServer::setupHeartbeatToMetaverse() {
    // heartbeat to the data-server every 15s
    const int DOMAIN_SERVER_DATA_WEB_HEARTBEAT_MSECS = 15 * 1000;

    if (!_metaverseHeartbeatTimer) {
        // setup a timer to heartbeat with the metaverse-server
        _metaverseHeartbeatTimer = new QTimer { this };
        connect(_metaverseHeartbeatTimer, SIGNAL(timeout()), this, SLOT(sendHeartbeatToMetaverse()));
        // do not send a heartbeat immediately - this avoids flooding if the heartbeat fails with a 401
        _metaverseHeartbeatTimer->start(DOMAIN_SERVER_DATA_WEB_HEARTBEAT_MSECS);
    }
}

void DomainServer::setupICEHeartbeatForFullNetworking() {
    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();

    // lookup the available ice-server hosts now
    updateICEServerAddresses();

    // call our sendHeartbeatToIceServer immediately anytime a local or public socket changes
    connect(limitedNodeList.data(), &LimitedNodeList::localSockAddrChanged,
            this, &DomainServer::sendHeartbeatToIceServer);
    connect(limitedNodeList.data(), &LimitedNodeList::publicSockAddrChanged,
            this, &DomainServer::sendHeartbeatToIceServer);

    // we need this DS to know what our public IP is - start trying to figure that out now
    limitedNodeList->startSTUNPublicSocketUpdate();

    // to send ICE heartbeats we'd better have a private key locally with an uploaded public key
    // if we have an access token and we don't have a private key or the current domain ID has changed
    // we should generate a new keypair
    auto accountManager = DependencyManager::get<AccountManager>();
    if (!accountManager->getAccountInfo().hasPrivateKey() || accountManager->getAccountInfo().getDomainID() != getID()) {
        accountManager->generateNewDomainKeypair(getID());
    }

    // hookup to the signal from account manager that tells us when keypair is available
    connect(accountManager.data(), &AccountManager::newKeypair, this, &DomainServer::handleKeypairChange);

    if (!_iceHeartbeatTimer) {
        // setup a timer to heartbeat with the ice-server
        _iceHeartbeatTimer = new QTimer { this };
        connect(_iceHeartbeatTimer, &QTimer::timeout, this, &DomainServer::sendHeartbeatToIceServer);
        sendHeartbeatToIceServer();
        _iceHeartbeatTimer->start(ICE_HEARBEAT_INTERVAL_MSECS);
    }
}

void DomainServer::updateICEServerAddresses() {
    if (_iceAddressLookupID == -1) {
        _iceAddressLookupID = QHostInfo::lookupHost(ICE_SERVER_DEFAULT_HOSTNAME, this, SLOT(handleICEHostInfo(QHostInfo)));
    }
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

void DomainServer::processListRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {

    QDataStream packetStream(message->getMessage());
    NodeConnectionData nodeRequestData = NodeConnectionData::fromDataStream(packetStream, message->getSenderSockAddr(), false);

    // update this node's sockets in case they have changed
    sendingNode->setPublicSocket(nodeRequestData.publicSockAddr);
    sendingNode->setLocalSocket(nodeRequestData.localSockAddr);
    
    // update the NodeInterestSet in case there have been any changes
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(sendingNode->getLinkedData());
    nodeData->setNodeInterestSet(nodeRequestData.interestList.toSet());

    // update the connecting hostname in case it has changed
    nodeData->setPlaceName(nodeRequestData.placeName);

    sendDomainListToNode(sendingNode, message->getSenderSockAddr());
}

unsigned int DomainServer::countConnectedUsers() {
    unsigned int result = 0;
    auto nodeList = DependencyManager::get<LimitedNodeList>();
    nodeList->eachNode([&](const SharedNodePointer& node){
        // only count unassigned agents (i.e., users)
        if (node->getType() == NodeType::Agent) {
            auto nodeData = static_cast<DomainServerNodeData*>(node->getLinkedData());
            if (nodeData && !nodeData->wasAssigned()) {
                result++;
            }
        }
    });
    return result;
}

QUrl DomainServer::oauthRedirectURL() {
    if (_httpsManager) {
        return QString("https://%1:%2/oauth").arg(_hostname).arg(_httpsManager->serverPort());
    } else {
        qWarning() << "Attempting to determine OAuth re-direct URL with no HTTPS server configured.";
        return QUrl();
    }
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
    DomainServerNodeData* nodeData = static_cast<DomainServerNodeData*>(newNode->getLinkedData());

    // reply back to the user with a PacketType::DomainList
    sendDomainListToNode(newNode, nodeData->getSendingSockAddr());

    // if this node is a user (unassigned Agent), signal
    if (newNode->getType() == NodeType::Agent && !nodeData->wasAssigned()) {
        emit userConnected();
    }

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
    extendedHeaderStream << node->getPermissions();

    auto domainListPackets = NLPacketList::create(PacketType::DomainList, extendedHeader);

    // always send the node their own UUID back
    QDataStream domainListStream(domainListPackets.get());

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
                    domainListPackets->startSegment();

                    // don't send avatar nodes to other avatars, that will come from avatar mixer
                    domainListStream << *otherNode.data();

                    // pack the secret that these two nodes will use to communicate with each other
                    domainListStream << connectionSecretForNodes(node, otherNode);

                    // we've added the node we wanted so end the segment now
                    domainListPackets->endSegment();
                }
            });
        }
    }
    
    // send an empty list to the node, in case there were no other nodes
    domainListPackets->closeCurrentPacket(true);

    // write the PacketList to this node
    limitedNodeList->sendPacketList(std::move(domainListPackets), *node);
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

void DomainServer::processRequestAssignmentPacket(QSharedPointer<ReceivedMessage> message) {
    // construct the requested assignment from the packet data
    Assignment requestAssignment(*message);

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
                 << "from" << message->getSenderSockAddr();
        noisyMessageTimer.restart();
    }

    SharedAssignmentPointer assignmentToDeploy = deployableAssignmentForRequest(requestAssignment);

    if (assignmentToDeploy) {
        qDebug() << "Deploying assignment -" << *assignmentToDeploy.data() << "- to" << message->getSenderSockAddr();

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
        limitedNodeList->sendUnreliablePacket(*assignmentPacket, message->getSenderSockAddr());

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
                << "from" << message->getSenderSockAddr();
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

    auto accountManager = DependencyManager::get<AccountManager>();

    if (accountManager->hasValidAccessToken()) {

        // enumerate the pending transactions and send them to the server to complete payment
        TransactionHash::iterator i = _pendingAssignmentCredits.begin();

        JSONCallbackParameters transactionCallbackParams;

        transactionCallbackParams.jsonCallbackReceiver = this;
        transactionCallbackParams.jsonCallbackMethod = "transactionJSONCallback";

        while (i != _pendingAssignmentCredits.end()) {
            accountManager->sendRequest("api/v1/transactions",
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
    sendHeartbeatToMetaverse(newPublicSockAddr.getAddress().toString());
}

void DomainServer::sendHeartbeatToMetaverse(const QString& networkAddress) {
    // Setup the domain object to send to the data server
    QJsonObject domainObject;

    // add the versions
    static const QString VERSION_KEY = "version";
    domainObject[VERSION_KEY] = BuildInfo::VERSION;
    static const QString PROTOCOL_KEY = "protocol";
    domainObject[PROTOCOL_KEY] = protocolVersionsSignatureBase64();

    // add networking
    if (!networkAddress.isEmpty()) {
        static const QString PUBLIC_NETWORK_ADDRESS_KEY = "network_address";
        domainObject[PUBLIC_NETWORK_ADDRESS_KEY] = networkAddress;
    }

    static const QString AUTOMATIC_NETWORKING_KEY = "automatic_networking";
    domainObject[AUTOMATIC_NETWORKING_KEY] = _automaticNetworkingSetting;


    // add access level for anonymous connections
    // consider the domain to be "restricted" if anonymous connections are disallowed
    static const QString RESTRICTED_ACCESS_FLAG = "restricted";
    NodePermissions anonymousPermissions = _settingsManager.getPermissionsForName(NodePermissions::standardNameAnonymous);
    domainObject[RESTRICTED_ACCESS_FLAG] = !anonymousPermissions.canConnectToDomain;

    const auto& temporaryDomainKey = DependencyManager::get<AccountManager>()->getTemporaryDomainKey(getID());
    if (!temporaryDomainKey.isEmpty()) {
        // add the temporary domain token
        const QString KEY_KEY = "api_key";
        domainObject[KEY_KEY] = temporaryDomainKey;
    }

    if (_metadata) {
        // Add the metadata to the heartbeat
        static const QString DOMAIN_HEARTBEAT_KEY = "heartbeat";
        domainObject[DOMAIN_HEARTBEAT_KEY] = _metadata->get(DomainMetadata::USERS);
    }

    QString domainUpdateJSON = QString("{\"domain\":%1}").arg(QString(QJsonDocument(domainObject).toJson(QJsonDocument::Compact)));

    static const QString DOMAIN_UPDATE = "/api/v1/domains/%1";
    DependencyManager::get<AccountManager>()->sendRequest(DOMAIN_UPDATE.arg(uuidStringWithoutCurlyBraces(getID())),
                                              AccountManagerAuth::Optional,
                                              QNetworkAccessManager::PutOperation,
                                              JSONCallbackParameters(nullptr, QString(), this, "handleMetaverseHeartbeatError"),
                                              domainUpdateJSON.toUtf8());
}

void DomainServer::handleMetaverseHeartbeatError(QNetworkReply& requestReply) {
    if (!_metaverseHeartbeatTimer) {
        // avoid rehandling errors from the same issue
        return;
    }

    // check if we need to force a new temporary domain name
    switch (requestReply.error()) {
        // if we have a temporary domain with a bad token, we get a 401
        case QNetworkReply::NetworkError::AuthenticationRequiredError: {
            static const QString DATA_KEY = "data";
            static const QString TOKEN_KEY = "api_key";

            QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();
            auto tokenFailure = jsonObject[DATA_KEY].toObject()[TOKEN_KEY];

            if (!tokenFailure.isNull()) {
                qWarning() << "Temporary domain name lacks a valid API key, and is being reset.";
            }
            break;
        }
        // if the domain does not (or no longer) exists, we get a 404
        case QNetworkReply::NetworkError::ContentNotFoundError:
            qWarning() << "Domain not found, getting a new temporary domain.";
            break;
        // otherwise, we erred on something else, and should not force a temporary domain
        default:
            return;
    }

    // halt heartbeats until we have a token
    _metaverseHeartbeatTimer->deleteLater();
    _metaverseHeartbeatTimer = nullptr;

    // give up eventually to avoid flooding traffic
    static const int MAX_ATTEMPTS = 5;
    static int attempt = 0;
    if (++attempt < MAX_ATTEMPTS) {
        // get a new temporary name and token
        getTemporaryName(true);
    } else {
        qWarning() << "Already attempted too many temporary domain requests. Please set a domain ID manually or restart.";
    }
}

void DomainServer::sendICEServerAddressToMetaverseAPI() {
    if (!_iceServerSocket.isNull()) {
        const QString ICE_SERVER_ADDRESS = "ice_server_address";

        QJsonObject domainObject;

        // we're using full automatic networking and we have a current ice-server socket, use that now
        domainObject[ICE_SERVER_ADDRESS] = _iceServerSocket.getAddress().toString();

        const auto& temporaryDomainKey = DependencyManager::get<AccountManager>()->getTemporaryDomainKey(getID());
        if (!temporaryDomainKey.isEmpty()) {
            // add the temporary domain token
            const QString KEY_KEY = "api_key";
            domainObject[KEY_KEY] = temporaryDomainKey;
        }

        QString domainUpdateJSON = QString("{\"domain\": %1 }").arg(QString(QJsonDocument(domainObject).toJson()));

        // make sure we hear about failure so we can retry
        JSONCallbackParameters callbackParameters;
        callbackParameters.errorCallbackReceiver = this;
        callbackParameters.errorCallbackMethod = "handleFailedICEServerAddressUpdate";

        qDebug() << "Updating ice-server address in High Fidelity Metaverse API to" << _iceServerSocket.getAddress().toString();

        static const QString DOMAIN_ICE_ADDRESS_UPDATE = "/api/v1/domains/%1/ice_server_address";

        DependencyManager::get<AccountManager>()->sendRequest(DOMAIN_ICE_ADDRESS_UPDATE.arg(uuidStringWithoutCurlyBraces(getID())),
                                                  AccountManagerAuth::Optional,
                                                  QNetworkAccessManager::PutOperation,
                                                  callbackParameters,
                                                  domainUpdateJSON.toUtf8());
    }
}

void DomainServer::handleFailedICEServerAddressUpdate(QNetworkReply& requestReply) {
    const int ICE_SERVER_UPDATE_RETRY_MS = 2 * 1000;

    qWarning() << "Failed to update ice-server address with High Fidelity Metaverse - error was" << requestReply.errorString();
    qWarning() << "\tRe-attempting in" << ICE_SERVER_UPDATE_RETRY_MS / 1000 << "seconds";

    QTimer::singleShot(ICE_SERVER_UPDATE_RETRY_MS, this, SLOT(sendICEServerAddressToMetaverseAPI()));
}

void DomainServer::sendHeartbeatToIceServer() {
    if (!_iceServerSocket.getAddress().isNull()) {

        auto accountManager = DependencyManager::get<AccountManager>();
        auto limitedNodeList = DependencyManager::get<LimitedNodeList>();

        if (!accountManager->getAccountInfo().hasPrivateKey()) {
            qWarning() << "Cannot send an ice-server heartbeat without a private key for signature.";
            qWarning() << "Waiting for keypair generation to complete before sending ICE heartbeat.";

            if (!limitedNodeList->getSessionUUID().isNull()) {
                accountManager->generateNewDomainKeypair(limitedNodeList->getSessionUUID());
            } else {
                qWarning() << "Attempting to send ICE server heartbeat with no domain ID. This is not supported";
            }

            return;
        }

        const int FAILOVER_NO_REPLY_ICE_HEARTBEATS { 3 };

        // increase the count of no reply ICE heartbeats and check the current value
        ++_noReplyICEHeartbeats;

        if (_noReplyICEHeartbeats > FAILOVER_NO_REPLY_ICE_HEARTBEATS) {
            qWarning() << "There have been" << _noReplyICEHeartbeats - 1 << "heartbeats sent with no reply from the ice-server";
            qWarning() << "Clearing the current ice-server socket and selecting a new candidate ice-server";

            // add the current address to our list of failed addresses
            _failedIceServerAddresses << _iceServerSocket.getAddress();

            // if we've failed to hear back for three heartbeats, we clear the current ice-server socket and attempt
            // to randomize a new one
            _iceServerSocket.clear();

            // reset the number of no reply ICE hearbeats
            _noReplyICEHeartbeats = 0;

            // reset the connection flag for ICE server
            _connectedToICEServer = false;

            // randomize our ice-server address (and simultaneously look up any new hostnames for available ice-servers)
            randomizeICEServerAddress(true);
        }

        // NOTE: I'd love to specify the correct size for the packet here, but it's a little trickey with
        // QDataStream and the possibility of IPv6 address for the sockets.
        if (!_iceServerHeartbeatPacket) {
            _iceServerHeartbeatPacket = NLPacket::create(PacketType::ICEServerHeartbeat);
        }

        bool shouldRecreatePacket = false;

        if (_iceServerHeartbeatPacket->getPayloadSize() > 0) {
            // if either of our sockets have changed we need to re-sign the heartbeat
            // first read the sockets out from the current packet
            _iceServerHeartbeatPacket->seek(0);
            QDataStream heartbeatStream(_iceServerHeartbeatPacket.get());

            QUuid senderUUID;
            HifiSockAddr publicSocket, localSocket;
            heartbeatStream >> senderUUID >> publicSocket >> localSocket;

            if (senderUUID != limitedNodeList->getSessionUUID()
                || publicSocket != limitedNodeList->getPublicSockAddr()
                || localSocket != limitedNodeList->getLocalSockAddr()) {
                shouldRecreatePacket = true;
            }
        } else {
            shouldRecreatePacket = true;
        }

        if (shouldRecreatePacket) {
            // either we don't have a heartbeat packet yet or some combination of sockets, ID and keypair have changed
            // and we need to make a new one

            // reset the position in the packet before writing
            _iceServerHeartbeatPacket->reset();

            // write our plaintext data to the packet
            QDataStream heartbeatDataStream(_iceServerHeartbeatPacket.get());
            heartbeatDataStream << limitedNodeList->getSessionUUID()
                << limitedNodeList->getPublicSockAddr() << limitedNodeList->getLocalSockAddr();

            // setup a QByteArray that points to the plaintext data
            auto plaintext = QByteArray::fromRawData(_iceServerHeartbeatPacket->getPayload(), _iceServerHeartbeatPacket->getPayloadSize());

            // generate a signature for the plaintext data in the packet
            auto signature = accountManager->getAccountInfo().signPlaintext(plaintext);

            // pack the signature with the data
            heartbeatDataStream << signature;
        }

        // send the heartbeat packet to the ice server now
        limitedNodeList->sendUnreliablePacket(*_iceServerHeartbeatPacket, _iceServerSocket);

    } else {
        qDebug() << "Not sending ice-server heartbeat since there is no selected ice-server.";
        qDebug() << "Waiting for" << ICE_SERVER_DEFAULT_HOSTNAME << "host lookup response";

    }
}

void DomainServer::processNodeJSONStatsPacket(QSharedPointer<ReceivedMessage> packetList, SharedNodePointer sendingNode) {
    auto nodeData = dynamic_cast<DomainServerNodeData*>(sendingNode->getLinkedData());
    if (nodeData) {
        nodeData->updateJSONStats(packetList->getMessage());
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

QDir pathForAssignmentScriptsDirectory() {
    static const QString SCRIPTS_DIRECTORY_NAME = "/scripts/";

    QDir directory(ServerPathUtils::getDataDirectory() + SCRIPTS_DIRECTORY_NAME);
    if (!directory.exists()) {
        directory.mkpath(".");
        qInfo() << "Created path to " << directory.path();
    }

    return directory;
}

QString pathForAssignmentScript(const QUuid& assignmentUUID) {
    QDir directory = pathForAssignmentScriptsDirectory();
    // append the UUID for this script as the new filename, remove the curly braces
    return directory.absoluteFilePath(uuidStringWithoutCurlyBraces(assignmentUUID));
}

const QString URI_OAUTH = "/oauth";
bool DomainServer::handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) {
    const QString JSON_MIME_TYPE = "application/json";

    const QString URI_ASSIGNMENT = "/assignment";
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
        QUuid nodeUUID = QUuid(assignmentRegex.cap(1));
        
        auto matchingNode = nodeList->nodeWithUUID(nodeUUID);
        
        // don't handle if we don't have a matching node
        if (!matchingNode) {
            return false;
        }
        
        auto nodeData = dynamic_cast<DomainServerNodeData*>(matchingNode->getLinkedData());
        
        // don't handle if we don't have node data for this node
        if (!nodeData) {
            return false;
        }
        
        SharedAssignmentPointer matchingAssignment = _allAssignments.value(nodeData->getAssignmentUUID());
        
        // check if we have an assignment that matches this temp UUID, and it is a scripted assignment
        if (matchingAssignment && matchingAssignment->getType() == Assignment::AgentType) {
            // we have a matching assignment and it is for the right type, have the HTTP manager handle it
            // via correct URL for the script so the client can download
            const auto it = _ephemeralACScripts.find(matchingAssignment->getUUID());

            if (it != _ephemeralACScripts.end()) {
                connection->respond(HTTPConnection::StatusCode200, it->second, "application/javascript");
            } else {
                connection->respond(HTTPConnection::StatusCode404, "Resource not found.");
            }

            return true;
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

                _ephemeralACScripts[scriptAssignment->getUUID()] = formData[0].second;

                // add the script assigment to the assignment queue
                SharedAssignmentPointer sharedScriptedAssignment(scriptAssignment);
                _unfulfilledAssignments.enqueue(sharedScriptedAssignment);
                _allAssignments.insert(sharedScriptedAssignment->getUUID(), sharedScriptedAssignment);
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

            QUrl authURL = oauthAuthorizationURL(stateUUID);

            Headers redirectHeaders;

            redirectHeaders.insert("Location", authURL.toEncoded());

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
                    QString hexHeaderPassword = QCryptographicHash::hash(headerPassword.toUtf8(), QCryptographicHash::Sha256).toHex();

                    if (settingsUsername == headerUsername
                        && (settingsPassword.isEmpty() || hexHeaderPassword == settingsPassword)) {
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

Headers DomainServer::setupCookieHeadersFromProfileReply(QNetworkReply* profileReply) {
    Headers cookieHeaders;

    // create a UUID for this cookie
    QUuid cookieUUID = QUuid::createUuid();

    QJsonDocument profileDocument = QJsonDocument::fromJson(profileReply->readAll());
    QJsonObject userObject = profileDocument.object()["data"].toObject()["user"].toObject();

    // add the profile to our in-memory data structure so we know who the user is when they send us their cookie
    DomainServerWebSessionData sessionData(userObject);
    _cookieSessionHash.insert(cookieUUID, sessionData);

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
    node->setLinkedData(std::unique_ptr<DomainServerNodeData> { new DomainServerNodeData() });
}

void DomainServer::nodeKilled(SharedNodePointer node) {
    // if this peer connected via ICE then remove them from our ICE peers hash
    _gatekeeper.removeICEPeer(node->getUUID());

    DomainServerNodeData* nodeData = static_cast<DomainServerNodeData*>(node->getLinkedData());

    if (nodeData) {
        // if this node's UUID matches a static assignment we need to throw it back in the assignment queue
        if (!nodeData->getAssignmentUUID().isNull()) {
            SharedAssignmentPointer matchedAssignment = _allAssignments.take(nodeData->getAssignmentUUID());

            if (matchedAssignment && matchedAssignment->isStatic()) {
                refreshStaticAssignmentAndAddToQueue(matchedAssignment);
            }
        }

        // cleanup the connection secrets that we set up for this node (on the other nodes)
        foreach (const QUuid& otherNodeSessionUUID, nodeData->getSessionSecretHash().keys()) {
            SharedNodePointer otherNode = DependencyManager::get<LimitedNodeList>()->nodeWithUUID(otherNodeSessionUUID);
            if (otherNode) {
                static_cast<DomainServerNodeData*>(otherNode->getLinkedData())->getSessionSecretHash().remove(node->getUUID());
            }
        }

        if (node->getType() == NodeType::Agent) {
            // if this node was an Agent ask DomainServerNodeData to remove the interpolation we potentially stored
            nodeData->removeOverrideForKey(USERNAME_UUID_REPLACEMENT_STATS_KEY,
                    uuidStringWithoutCurlyBraces(node->getUUID()));

            // if this node is a user (unassigned Agent), signal
            if (!nodeData->wasAssigned()) {
                emit userDisconnected();
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
    auto sharedAssignments = _allAssignments.values();
    
    // sort the assignments to put the server/mixer assignments first
    qSort(sharedAssignments.begin(), sharedAssignments.end(), [](SharedAssignmentPointer a, SharedAssignmentPointer b){
        if (a->getType() == b->getType()) {
            return true;
        } else if (a->getType() != Assignment::AgentType && b->getType() != Assignment::AgentType) {
            return a->getType() < b->getType();
        } else {
            return a->getType() != Assignment::AgentType;
        }
    });
    
    auto staticAssignment = sharedAssignments.begin();
    
    while (staticAssignment != sharedAssignments.end()) {
        // add any of the un-matched static assignments to the queue

        // enumerate the nodes and check if there is one with an attached assignment with matching UUID
        if (!DependencyManager::get<LimitedNodeList>()->nodeWithUUID((*staticAssignment)->getUUID())) {
            // this assignment has not been fulfilled - reset the UUID and add it to the assignment queue
            refreshStaticAssignmentAndAddToQueue(*staticAssignment);
        }

        ++staticAssignment;
    }
}

void DomainServer::processPathQueryPacket(QSharedPointer<ReceivedMessage> message) {
    // this is a query for the viewpoint resulting from a path
    // first pull the query path from the packet

    // figure out how many bytes the sender said this path is
    quint16 numPathBytes;
    message->readPrimitive(&numPathBytes);

    if (numPathBytes <= message->getBytesLeftToRead()) {
        // the number of path bytes makes sense for the sent packet - pull out the path
        QString pathQuery = QString::fromUtf8(message->getRawMessage() + message->getPosition(), numPathBytes);

        // our settings contain paths that start with a leading slash, so make sure this query has that
        if (!pathQuery.startsWith("/")) {
            pathQuery.prepend("/");
        }

        const QString PATHS_SETTINGS_KEYPATH_FORMAT = "%1.%2";
        const QString PATH_VIEWPOINT_KEY = "viewpoint";
        const QString INDEX_PATH = "/";

        // check out paths in the _configMap to see if we have a match
        auto keypath = QString(PATHS_SETTINGS_KEYPATH_FORMAT).arg(SETTINGS_PATHS_KEY).arg(pathQuery);
        const QVariant* pathMatch = valueForKeyPath(_settingsManager.getSettingsMap(), keypath);

        if (pathMatch || pathQuery == INDEX_PATH) {
            // we got a match, respond with the resulting viewpoint
            auto nodeList = DependencyManager::get<LimitedNodeList>();

            QString responseViewpoint;

            // if we didn't match the path BUT this is for the index path then send back our default
            if (pathMatch) {
                responseViewpoint = pathMatch->toMap()[PATH_VIEWPOINT_KEY].toString();
            } else {
                const QString DEFAULT_INDEX_PATH = "/0,0,0/0,0,0,1";
                responseViewpoint = DEFAULT_INDEX_PATH;
            }

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
                    nodeList->sendPacket(std::move(pathResponsePacket), message->getSenderSockAddr());
                }
            }

        } else {
            // we don't respond if there is no match - this may need to change once this packet
            // query/response is made reliable
            qDebug() << "No match for path query" << pathQuery << "- refusing to respond.";
        }
    }
}

void DomainServer::processNodeDisconnectRequestPacket(QSharedPointer<ReceivedMessage> message) {
    // This packet has been matched to a source node and they're asking not to be in the domain anymore
    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();

    const QUuid& nodeUUID = message->getSourceID();

    qDebug() << "Received a disconnect request from node with UUID" << nodeUUID;

    // we want to check what type this node was before going to kill it so that we can avoid sending the RemovedNode
    // packet to nodes that don't care about this type
    auto nodeToKill = limitedNodeList->nodeWithUUID(nodeUUID);

    if (nodeToKill) {
        handleKillNode(nodeToKill);
    }
}

void DomainServer::handleKillNode(SharedNodePointer nodeToKill) {
    auto nodeType = nodeToKill->getType();
    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
    const QUuid& nodeUUID = nodeToKill->getUUID();

    limitedNodeList->killNodeWithUUID(nodeUUID);

    static auto removedNodePacket = NLPacket::create(PacketType::DomainServerRemovedNode, NUM_BYTES_RFC4122_UUID);

    removedNodePacket->reset();
    removedNodePacket->write(nodeUUID.toRfc4122());

    // broadcast out the DomainServerRemovedNode message
    limitedNodeList->eachMatchingNode([&nodeType](const SharedNodePointer& otherNode) -> bool {
        // only send the removed node packet to nodes that care about the type of node this was
        auto nodeLinkedData = dynamic_cast<DomainServerNodeData*>(otherNode->getLinkedData());
        return (nodeLinkedData != nullptr) && nodeLinkedData->getNodeInterestSet().contains(nodeType);
    }, [&limitedNodeList](const SharedNodePointer& otherNode){
        limitedNodeList->sendUnreliablePacket(*removedNodePacket, *otherNode);
    });
}

void DomainServer::processICEServerHeartbeatDenialPacket(QSharedPointer<ReceivedMessage> message) {
    static const int NUM_HEARTBEAT_DENIALS_FOR_KEYPAIR_REGEN = 3;

    if (++_numHeartbeatDenials > NUM_HEARTBEAT_DENIALS_FOR_KEYPAIR_REGEN) {
        qDebug() << "Received" << NUM_HEARTBEAT_DENIALS_FOR_KEYPAIR_REGEN << "heartbeat denials from ice-server"
            << "- re-generating keypair now";

        // we've hit our threshold of heartbeat denials, trigger a keypair re-generation
        auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
        DependencyManager::get<AccountManager>()->generateNewDomainKeypair(limitedNodeList->getSessionUUID());

        // reset our number of heartbeat denials
        _numHeartbeatDenials = 0;
    }

    // even though we can't get into this ice-server it is responding to us, so we reset our number of no-reply heartbeats
    _noReplyICEHeartbeats = 0;
}

void DomainServer::processICEServerHeartbeatACK(QSharedPointer<ReceivedMessage> message) {
    // we don't do anything with this ACK other than use it to tell us to keep talking to the same ice-server
    _noReplyICEHeartbeats = 0;

    if (!_connectedToICEServer) {
        _connectedToICEServer = true;
        qInfo() << "Connected to ice-server at" << _iceServerSocket;
    }
}

void DomainServer::handleKeypairChange() {
    if (_iceServerHeartbeatPacket) {
        // reset the payload size of the ice-server heartbeat packet - this causes the packet to be re-generated
        // the next time we go to send an ice-server heartbeat
        _iceServerHeartbeatPacket->setPayloadSize(0);

        // send a heartbeat to the ice server immediately
        sendHeartbeatToIceServer();
    }
}

void DomainServer::handleICEHostInfo(const QHostInfo& hostInfo) {
    // clear the ICE address lookup ID so that it can fire again
    _iceAddressLookupID = -1;

    if (hostInfo.error() != QHostInfo::NoError) {
        qWarning() << "IP address lookup failed for" << ICE_SERVER_DEFAULT_HOSTNAME << ":" << hostInfo.errorString();

        // if we don't have an ICE server to use yet, trigger a retry
        if (_iceServerSocket.isNull()) {
            const int ICE_ADDRESS_LOOKUP_RETRY_MS = 1000;

            QTimer::singleShot(ICE_ADDRESS_LOOKUP_RETRY_MS, this, SLOT(updateICEServerAddresses()));
        }

    } else {
        int countBefore = _iceServerAddresses.count();

        _iceServerAddresses = hostInfo.addresses();

        if (countBefore == 0) {
            qInfo() << "Found" << _iceServerAddresses.count() << "ice-server IP addresses for" << ICE_SERVER_DEFAULT_HOSTNAME;
        }

        if (_iceServerSocket.isNull()) {
            // we don't have a candidate ice-server yet, pick now (without triggering a host lookup since we just did one)
            randomizeICEServerAddress(false);
        }
    }
}

void DomainServer::randomizeICEServerAddress(bool shouldTriggerHostLookup) {
    if (shouldTriggerHostLookup) {
        updateICEServerAddresses();
    }

    // create a list by removing the already failed ice-server addresses
    auto candidateICEAddresses = _iceServerAddresses;

    auto it = candidateICEAddresses.begin();

    while (it != candidateICEAddresses.end()) {
        if (_failedIceServerAddresses.contains(*it)) {
            // we already tried this address and it failed, remove it from list of candidates
            it = candidateICEAddresses.erase(it);
        } else {
            // keep this candidate, it hasn't failed yet
            ++it;
        }
    }

    if (candidateICEAddresses.empty()) {
        // we ended up with an empty list since everything we've tried has failed
        // so clear the set of failed addresses and start going through them again

        qWarning() << "All current ice-server addresses have failed - re-attempting all current addresses for"
            << ICE_SERVER_DEFAULT_HOSTNAME;

        _failedIceServerAddresses.clear();
        candidateICEAddresses = _iceServerAddresses;
    }

    // of the list of available addresses that we haven't tried, pick a random one
    int maxIndex = candidateICEAddresses.size() - 1;
    int indexToTry = 0;

    if (maxIndex > 0) {
        static std::random_device randomDevice;
        static std::mt19937 generator(randomDevice());
        std::uniform_int_distribution<> distribution(0, maxIndex);

        indexToTry = distribution(generator);
    }

    _iceServerSocket = HifiSockAddr { candidateICEAddresses[indexToTry], ICE_SERVER_DEFAULT_PORT };
    qInfo() << "Set candidate ice-server socket to" << _iceServerSocket;

    // clear our number of hearbeat denials, this should be re-set on ice-server change
    _numHeartbeatDenials = 0;

    // immediately fire an ICE heartbeat once we've picked a candidate ice-server
    sendHeartbeatToIceServer();

    // immediately send an update to the metaverse API when our ice-server changes
    sendICEServerAddressToMetaverseAPI();
}
