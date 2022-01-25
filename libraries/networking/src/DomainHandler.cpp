//
//  DomainHandler.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainHandler.h"

#include <math.h>

#include <PathUtils.h>

#include <shared/QtHelpers.h>

#include <QThread>

#include <QtCore/QJsonDocument>
#include <QtCore/QDataStream>

#include "AddressManager.h"
#include "Assignment.h"
#include "DomainAccountManager.h"
#include "SockAddr.h"
#include "NodeList.h"
#include "udt/Packet.h"
#include "udt/PacketHeaders.h"
#include "NLPacket.h"
#include "SharedUtil.h"
#include "UserActivityLogger.h"
#include "NetworkLogging.h"

DomainHandler::DomainHandler(QObject* parent) :
    QObject(parent),
    _sockAddr(SockAddr(SocketType::UDP, QHostAddress::Null, DEFAULT_DOMAIN_SERVER_PORT)),
    _icePeer(this),
    _settingsTimer(this),
    _apiRefreshTimer(this)
{
    _sockAddr.setObjectName("DomainServer");

    // if we get a socket that make sure our NetworkPeer ping timer stops
    connect(this, &DomainHandler::completedSocketDiscovery, &_icePeer, &NetworkPeer::stopPingTimer);

    // setup a timeout for failure on settings requests
    static const int DOMAIN_SETTINGS_TIMEOUT_MS = 5000;
    _settingsTimer.setInterval(DOMAIN_SETTINGS_TIMEOUT_MS); // 5s, Qt::CoarseTimer acceptable
    connect(&_settingsTimer, &QTimer::timeout, this, &DomainHandler::settingsReceiveFail);

    // setup the API refresh timer for auto connection information refresh from API when failing to connect
    const int API_REFRESH_TIMEOUT_MSEC = 2500;
    _apiRefreshTimer.setInterval(API_REFRESH_TIMEOUT_MSEC); // 2.5s, Qt::CoarseTimer acceptable

    auto addressManager = DependencyManager::get<AddressManager>();
    connect(&_apiRefreshTimer, &QTimer::timeout, addressManager.data(), &AddressManager::refreshPreviousLookup);

    // stop the refresh timer if we connect to a domain
    connect(this, &DomainHandler::connectedToDomain, &_apiRefreshTimer, &QTimer::stop);

    // stop the refresh timer if redirected to the error domain
    connect(this, &DomainHandler::redirectToErrorDomainURL, &_apiRefreshTimer, &QTimer::stop);
}

void DomainHandler::disconnect(QString reason) {
    // if we're currently connected to a domain, send a disconnect packet on our way out
    if (_isConnected) {
        sendDisconnectPacket();
    }

    // clear member variables that hold the connection state to a domain
    _uuid = QUuid();
    _connectionToken = QUuid();

    _icePeer.reset();

    if (requiresICE()) {
        // if we connected to this domain with ICE, re-set the socket so we reconnect through the ice-server
        _sockAddr.clear();
    }

    qCDebug(networking_ice) << "Disconnecting from domain server.";
    qCDebug(networking_ice) << "REASON:" << reason;
    setIsConnected(false);
}

void DomainHandler::sendDisconnectPacket() {
    // The DomainDisconnect packet is not verified - we're relying on the eventual addition of DTLS to the
    // domain-server connection to stop greifing here

    // construct the disconnect packet once (an empty packet but sourced with our current session UUID)
    static auto disconnectPacket = NLPacket::create(PacketType::DomainDisconnectRequest, 0);

    // send the disconnect packet to the current domain server
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendUnreliablePacket(*disconnectPacket, _sockAddr);
}

void DomainHandler::clearSettings() {
    _settingsObject = QJsonObject();
}

void DomainHandler::softReset(QString reason) {
    qCDebug(networking) << "Resetting current domain connection information.";
    disconnect(reason);

    clearSettings();

    _connectionDenialsSinceKeypairRegen = 0;
    _checkInPacketsSinceLastReply = 0;

    // cancel the failure timeout for any pending requests for settings
    QMetaObject::invokeMethod(&_settingsTimer, "stop");

    // restart the API refresh timer in case we fail to connect and need to refresh information
    if (!_isInErrorState) {
        QMetaObject::invokeMethod(&_apiRefreshTimer, "start");
    }
}

void DomainHandler::hardReset(QString reason) {
    emit resetting();

    softReset(reason);
    _haveAskedConnectWithoutAvatarEntities = false;
    _canConnectWithoutAvatarEntities = false;
    _isInErrorState = false;
    emit redirectErrorStateChanged(_isInErrorState);

    qCDebug(networking) << "Hard reset in NodeList DomainHandler.";
    _pendingDomainID = QUuid();
    _iceServerSockAddr = SockAddr();
    _sockAddr.clear();
    _domainURL = QUrl();

    _domainConnectionRefusals.clear();

    _hasCheckedForAccessToken = false;

    // clear any pending path we may have wanted to ask the previous DS about
    _pendingPath.clear();
}

bool DomainHandler::isHardRefusal(int reasonCode) {
    return (reasonCode == (int)ConnectionRefusedReason::ProtocolMismatch ||
            reasonCode == (int)ConnectionRefusedReason::TooManyUsers ||
            reasonCode == (int)ConnectionRefusedReason::NotAuthorizedMetaverse ||
            reasonCode == (int)ConnectionRefusedReason::NotAuthorizedDomain ||
            reasonCode == (int)ConnectionRefusedReason::TimedOut);
}

bool DomainHandler::getInterstitialModeEnabled() const {
    return _interstitialModeSettingLock.resultWithReadLock<bool>([&] {
        return _enableInterstitialMode.get();
    });
}

void DomainHandler::setInterstitialModeEnabled(bool enableInterstitialMode) {
    _interstitialModeSettingLock.withWriteLock([&] {
        _enableInterstitialMode.set(enableInterstitialMode);
    });
}

void DomainHandler::setErrorDomainURL(const QUrl& url) {
    _errorDomainURL = url;
    return;
}

void DomainHandler::setSockAddr(const SockAddr& sockAddr, const QString& hostname) {
    if (_sockAddr != sockAddr) {
        // we should reset on a sockAddr change
        hardReset("Changing domain sockAddr");
        // change the sockAddr
        _sockAddr = sockAddr;
    }

    if (!_sockAddr.isNull()) {
        DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    }

    // some callers may pass a hostname, this is not to be used for lookup but for DTLS certificate verification
    _domainURL = QUrl();
    _domainURL.setScheme(URL_SCHEME_VIRCADIA);
    _domainURL.setHost(hostname);
    _domainURL.setPort(_sockAddr.getPort());
}

void DomainHandler::setUUID(const QUuid& uuid) {
    if (uuid != _uuid) {
        _uuid = uuid;
        qCDebug(networking) << "Domain ID changed to" << uuidStringWithoutCurlyBraces(_uuid);
    }
}

void DomainHandler::setURLAndID(QUrl domainURL, QUuid domainID) {
    _pendingDomainID = domainID;

    if (domainURL.scheme() != URL_SCHEME_VIRCADIA) {
        _sockAddr.clear();

        // if this is a file URL we need to see if it has a ~ for us to expand
        if (domainURL.scheme() == HIFI_URL_SCHEME_FILE) {
            domainURL = PathUtils::expandToLocalDataAbsolutePath(domainURL);
        }
    }

    auto domainPort = domainURL.port();
    if (domainPort == -1) {
        domainPort = DEFAULT_DOMAIN_SERVER_PORT;
    }

    // if it's in the error state, reset and try again.
    if (_domainURL != domainURL 
        || (_sockAddr.getPort() != domainPort && domainURL.scheme() == URL_SCHEME_VIRCADIA)
        || isServerless() // For reloading content in serverless domain.
        || _isInErrorState) {
        // re-set the domain info so that auth information is reloaded
        hardReset("Changing domain URL");

        QString previousHost = _domainURL.host();
        _domainURL = domainURL;

        _hasCheckedForDomainAccessToken = false;

        if (previousHost != domainURL.host()) {
            qCDebug(networking) << "Updated domain hostname to" << domainURL.host();

            if (!domainURL.host().isEmpty()) {
                if (domainURL.scheme() == URL_SCHEME_VIRCADIA) {
                    // re-set the sock addr to null and fire off a lookup of the IP address for this domain-server's hostname
                    qCDebug(networking, "Looking up DS hostname %s.", domainURL.host().toLocal8Bit().constData());
                    QHostInfo::lookupHost(domainURL.host(), this, SLOT(completedHostnameLookup(const QHostInfo&)));
                }

                DependencyManager::get<NodeList>()->flagTimeForConnectionStep(
                    LimitedNodeList::ConnectionStep::SetDomainHostname);

                UserActivityLogger::getInstance().changedDomain(domainURL.host());
            }
        }

        DependencyManager::get<DomainAccountManager>()->setDomainURL(_domainURL);

        emit domainURLChanged(_domainURL);

        if (_sockAddr.getPort() != domainPort) {
            qCDebug(networking) << "Updated domain port to" << domainPort;
            _sockAddr.setPort(domainPort);
        }
    }
}

void DomainHandler::setIceServerHostnameAndID(const QString& iceServerHostname, const QUuid& id) {

    auto newIceServer = _iceServerSockAddr.getAddress().toString() != iceServerHostname;
    auto newDomainID = id != _pendingDomainID;

    // if it's in the error state, reset and try again.
    if (newIceServer || newDomainID || _isInErrorState) {
        QString reason;
        if (newIceServer) {
            reason += "New ICE server;";
        }
        if (newDomainID) {
            reason += "New domain ID;";
        }
        if (_isInErrorState) {
            reason += "Domain in error state;";
        }

        // re-set the domain info to connect to new domain
        hardReset(reason);

        // refresh our ICE client UUID to something new
        _iceClientID = QUuid::createUuid();

        _pendingDomainID = id;

        SockAddr* replaceableSockAddr = &_iceServerSockAddr;
        replaceableSockAddr->~SockAddr();
        replaceableSockAddr = new (replaceableSockAddr) SockAddr(SocketType::UDP, iceServerHostname, ICE_SERVER_DEFAULT_PORT);
        _iceServerSockAddr.setObjectName("IceServer");

        auto nodeList = DependencyManager::get<NodeList>();

        nodeList->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetICEServerHostname);

        if (_iceServerSockAddr.getAddress().isNull()) {
            // connect to lookup completed for ice-server socket so we can request a heartbeat once hostname is looked up
            connect(&_iceServerSockAddr, &SockAddr::lookupCompleted, this, &DomainHandler::completedIceServerHostnameLookup);
        } else {
            completedIceServerHostnameLookup();
        }

        qCDebug(networking_ice) << "ICE required to connect to domain via ice server at" << iceServerHostname;
    }
}

void DomainHandler::activateICELocalSocket() {
    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    _sockAddr = _icePeer.getLocalSocket();
    _domainURL.setScheme(URL_SCHEME_VIRCADIA);
    _domainURL.setHost(_sockAddr.getAddress().toString());
    emit domainURLChanged(_domainURL);
    emit completedSocketDiscovery();
}

void DomainHandler::activateICEPublicSocket() {
    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    _sockAddr = _icePeer.getPublicSocket();
    _domainURL.setScheme(URL_SCHEME_VIRCADIA);
    _domainURL.setHost(_sockAddr.getAddress().toString());
    emit domainURLChanged(_domainURL);
    emit completedSocketDiscovery();
}

QString DomainHandler::getViewPointFromNamedPath(QString namedPath) {
    auto lookup = _namedPaths.find(namedPath);
    if (lookup != _namedPaths.end()) {
        return lookup->second;
    }
    if (namedPath == DEFAULT_NAMED_PATH) {
        return DOMAIN_SPAWNING_POINT;
    }
    return "";
}

void DomainHandler::completedHostnameLookup(const QHostInfo& hostInfo) {
    for (int i = 0; i < hostInfo.addresses().size(); i++) {
        if (hostInfo.addresses()[i].protocol() == QAbstractSocket::IPv4Protocol) {
            _sockAddr.setAddress(hostInfo.addresses()[i]);

            DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);

            qCDebug(networking, "DS at %s is at %s", _domainURL.host().toLocal8Bit().constData(),
                   _sockAddr.getAddress().toString().toLocal8Bit().constData());

            emit completedSocketDiscovery();

            return;
        }
    }

    // if we got here then we failed to lookup the address
    qCDebug(networking, "Failed domain server lookup");
}

void DomainHandler::completedIceServerHostnameLookup() {
    qCDebug(networking_ice) << "ICE server socket is at" << _iceServerSockAddr;

    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetICEServerSocket);

    // emit our signal so we can send a heartbeat to ice-server immediately
    emit iceSocketAndIDReceived();
}

void DomainHandler::setIsConnected(bool isConnected) {
    if (_isConnected != isConnected) {
        _isConnected = isConnected;

        if (_isConnected) {
            _lastDomainConnectionError = -1;
            emit connectedToDomain(_domainURL);

            // FIXME: Reinstate the requestDomainSettings() call here in version 2021.2.0 instead of having it in 
            // NodeList::processDomainList().
            /*
            if (_domainURL.scheme() == URL_SCHEME_VIRCADIA && !_domainURL.host().isEmpty()) {
                // we've connected to new domain - time to ask it for global settings
                requestDomainSettings();
            }
            */

        } else {
            emit disconnectedFromDomain();
        }
    }
}

void DomainHandler::setCanConnectWithoutAvatarEntities(bool canConnect) {
    _canConnectWithoutAvatarEntities = canConnect;
    _haveAskedConnectWithoutAvatarEntities = true;
}

bool DomainHandler::canConnectWithoutAvatarEntities() {
    if (!_canConnectWithoutAvatarEntities && !_haveAskedConnectWithoutAvatarEntities) {
        if (_isConnected) {
            // Already connected so don't ask. (Permission removed from user while in the domain.)
            setCanConnectWithoutAvatarEntities(true);
        } else {
            // Ask whether to connect to the domain.
            emit confirmConnectWithoutAvatarEntities();
        }
    }
    return _canConnectWithoutAvatarEntities;
}

void DomainHandler::connectedToServerless(std::map<QString, QString> namedPaths) {
    _namedPaths = namedPaths;
    setIsConnected(true);
}

void DomainHandler::loadedErrorDomain(std::map<QString, QString> namedPaths) {
    auto lookup = namedPaths.find("/");
    QString viewpoint;
    if (lookup != namedPaths.end()) {
        viewpoint = lookup->second;
    } else {
        viewpoint = DOMAIN_SPAWNING_POINT;
    }
    DependencyManager::get<AddressManager>()->goToViewpointForPath(viewpoint, QString());
}

void DomainHandler::setRedirectErrorState(QUrl errorUrl, QString reasonMessage, int reasonCode, const QString& extraInfo) {
    _lastDomainConnectionError = reasonCode;
    if (getInterstitialModeEnabled() && isHardRefusal(reasonCode)) {
        _errorDomainURL = errorUrl;
        _isInErrorState = true;
        qCDebug(networking) << "Error connecting to domain: " << reasonMessage;
        emit redirectErrorStateChanged(_isInErrorState);
        emit redirectToErrorDomainURL(_errorDomainURL);
    } else {
        emit domainConnectionRefused(reasonMessage, reasonCode, extraInfo);
    }
}

void DomainHandler::requestDomainSettings() {
    qCDebug(networking) << "Requesting settings from domain server";

    Assignment::Type assignmentType = Assignment::typeForNodeType(DependencyManager::get<NodeList>()->getOwnerType());

    auto packet = NLPacket::create(PacketType::DomainSettingsRequest, sizeof(assignmentType), true, false);
    packet->writePrimitive(assignmentType);

    auto nodeList = DependencyManager::get<LimitedNodeList>();
    nodeList->sendPacket(std::move(packet), _sockAddr);

    _settingsTimer.start();
}

void DomainHandler::processSettingsPacketList(QSharedPointer<ReceivedMessage> packetList) {
    // stop our settings timer since we successfully requested the settings we need
    _settingsTimer.stop();
    
    auto data = packetList->getMessage();

    _settingsObject = QJsonDocument::fromJson(data).object();
    
    if (!_settingsObject.isEmpty()) {
        qCDebug(networking) << "Received domain settings: \n" << _settingsObject;
    }

    emit settingsReceived(_settingsObject);
}

void DomainHandler::processICEPingReplyPacket(QSharedPointer<ReceivedMessage> message) {
    const SockAddr& senderSockAddr = message->getSenderSockAddr();
    qCDebug(networking_ice) << "Received reply from domain-server on" << senderSockAddr;

    if (getIP().isNull()) {
        // we're hearing back from this domain-server, no need to refresh API information
        _apiRefreshTimer.stop();

        // for now we're unsafely assuming this came back from the domain
        if (senderSockAddr == _icePeer.getLocalSocket()) {
            qCDebug(networking_ice) << "Connecting to domain using local socket";
            activateICELocalSocket();
        } else if (senderSockAddr == _icePeer.getPublicSocket()) {
            qCDebug(networking_ice) << "Conecting to domain using public socket";
            activateICEPublicSocket();
        } else {
            qCDebug(networking_ice) << "Reply does not match either local or public socket for domain. Will not connect.";
        }
    }
}

void DomainHandler::processDTLSRequirementPacket(QSharedPointer<ReceivedMessage> message) {
    // figure out the port that the DS wants us to use for us to talk to them with DTLS
    unsigned short dtlsPort;
    message->readPrimitive(&dtlsPort);

    qCDebug(networking) << "domain-server DTLS port changed to" << dtlsPort << "- Enabling DTLS.";

    _sockAddr.setPort(dtlsPort);

//    initializeDTLSSession();
}

void DomainHandler::processICEResponsePacket(QSharedPointer<ReceivedMessage> message) {
    if (_icePeer.hasSockets()) {
        qCDebug(networking_ice) << "Received an ICE peer packet for domain-server but we already have sockets. Not processing.";
        // bail on processing this packet if our ice peer already has sockets
        return;
    }

    // start or restart the API refresh timer now that we have new information
    _apiRefreshTimer.start();

    QDataStream iceResponseStream(message->getMessage());

    iceResponseStream >> _icePeer;

    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::ReceiveDSPeerInformation);

    if (_icePeer.getUUID() != _pendingDomainID) {
        qCDebug(networking_ice) << "Received a network peer with ID that does not match current domain. Will not attempt connection.";
        _icePeer.reset();
    } else {
        qCDebug(networking_ice) << "Received network peer object for domain -" << _icePeer;

        // ask the peer object to start its ping timer
        _icePeer.startPingTimer();

        // emit our signal so the NodeList knows to send a ping immediately
        emit icePeerSocketsReceived();
    }
}

bool DomainHandler::reasonSuggestsMetaverseLogin(ConnectionRefusedReason reasonCode) {
    switch (reasonCode) {
        case ConnectionRefusedReason::LoginErrorMetaverse:
        case ConnectionRefusedReason::NotAuthorizedMetaverse:
            return true;

        default:
        case ConnectionRefusedReason::Unknown:
        case ConnectionRefusedReason::ProtocolMismatch:
        case ConnectionRefusedReason::TooManyUsers:
        case ConnectionRefusedReason::NotAuthorizedDomain:
            return false;
    }
    return false;
}

bool DomainHandler::reasonSuggestsDomainLogin(ConnectionRefusedReason reasonCode) {
    switch (reasonCode) {
        case ConnectionRefusedReason::LoginErrorDomain:
        case ConnectionRefusedReason::NotAuthorizedDomain:
            return true;

        default:
        case ConnectionRefusedReason::Unknown:
        case ConnectionRefusedReason::ProtocolMismatch:
        case ConnectionRefusedReason::TooManyUsers:
        case ConnectionRefusedReason::NotAuthorizedMetaverse:
            return false;
    }
    return false;
}

void DomainHandler::processDomainServerConnectionDeniedPacket(QSharedPointer<ReceivedMessage> message) {

    // Ignore any residual packets from previous domain.
    if (!message->getSenderSockAddr().getAddress().isEqual(_sockAddr.getAddress())) {
        return;
    }

    // we're hearing from this domain-server, don't need to refresh API info
    _apiRefreshTimer.stop();

    // this counts as a reply from the DS after a check in or connect packet, so reset that counter now
    _checkInPacketsSinceLastReply = 0;

    // Read deny reason from packet
    uint8_t reasonCodeWire;

    message->readPrimitive(&reasonCodeWire);
    ConnectionRefusedReason reasonCode = static_cast<ConnectionRefusedReason>(reasonCodeWire);
    quint16 reasonSize;
    message->readPrimitive(&reasonSize);
    auto reasonText = message->readWithoutCopy(reasonSize);
    QString reasonMessage = QString::fromUtf8(reasonText);

    quint16 extraInfoSize;
    message->readPrimitive(&extraInfoSize);
    auto extraInfoUtf8= message->readWithoutCopy(extraInfoSize);
    QString extraInfo = QString::fromUtf8(extraInfoUtf8);

    // output to the log so the user knows they got a denied connection request
    // and check and signal for an access token so that we can make sure they are logged in
    QString sanitizedExtraInfo = extraInfo.toLower().startsWith("http") ? "" : extraInfo;  // Don't log URLs.
    qCWarning(networking) << "The domain-server denied a connection request: " << reasonMessage 
        << " extraInfo:" << sanitizedExtraInfo;

    if (!_domainConnectionRefusals.contains(reasonMessage)) {
        _domainConnectionRefusals.insert(reasonMessage);
#if defined(Q_OS_ANDROID)
        emit domainConnectionRefused(reasonMessage, (int)reasonCode, extraInfo);
#else

        // ingest the error - this is a "hard" connection refusal.
        setRedirectErrorState(_errorDomainURL, reasonMessage, (int)reasonCode, extraInfo);
#endif
    }


    // Some connection refusal reasons imply that a login is required. If so, suggest a new login.
    if (reasonSuggestsMetaverseLogin(reasonCode)) {
        qCWarning(networking) << "Make sure you are logged in to the metaverse.";

        auto accountManager = DependencyManager::get<AccountManager>();

        if (!_hasCheckedForAccessToken) {
            accountManager->checkAndSignalForAccessToken();
            _hasCheckedForAccessToken = true;
        }

        static const int CONNECTION_DENIALS_FOR_KEYPAIR_REGEN = 3;

        // force a re-generation of key-pair after CONNECTION_DENIALS_FOR_KEYPAIR_REGEN failed connection attempts
        if (++_connectionDenialsSinceKeypairRegen >= CONNECTION_DENIALS_FOR_KEYPAIR_REGEN) {
            accountManager->generateNewUserKeypair();
            _connectionDenialsSinceKeypairRegen = 0;
        }

        // Server with domain login will prompt for domain login, not metaverse, so reset domain values if asked for metaverse.
        auto domainAccountManager = DependencyManager::get<DomainAccountManager>();
        domainAccountManager->setAuthURL(QUrl());
        domainAccountManager->setClientID(QString());

    } else if (reasonSuggestsDomainLogin(reasonCode)) {
        qCWarning(networking) << "Make sure you are logged in to the domain.";

        auto domainAccountManager = DependencyManager::get<DomainAccountManager>();
        if (!extraInfo.isEmpty()) {
            auto extraInfoComponents = extraInfo.split("|");
            domainAccountManager->setAuthURL(extraInfoComponents.value(0));
            domainAccountManager->setClientID(extraInfoComponents.value(1));
        } else {
            // Shouldn't occur, but just in case.
            domainAccountManager->setAuthURL(QUrl());
            domainAccountManager->setClientID(QString());
        }

        if (!_hasCheckedForDomainAccessToken) {
            domainAccountManager->checkAndSignalForAccessToken();
            _hasCheckedForDomainAccessToken = true;
        }

        // ####### TODO: regenerate key-pair after several failed connection attempts, similar to metaverse login code?

    }
}

static const int SILENT_DOMAIN_TRAFFIC_DROP_MIN = 2;

bool DomainHandler::checkInPacketTimeout() {
    ++_checkInPacketsSinceLastReply;

    if (_checkInPacketsSinceLastReply > 1) {
        qCDebug(networking_ice) << "Silent domain checkins:" << _checkInPacketsSinceLastReply;
    }

    auto nodeList = DependencyManager::get<NodeList>();

    if (_checkInPacketsSinceLastReply > SILENT_DOMAIN_TRAFFIC_DROP_MIN) {
        qCDebug(networking_ice) << _checkInPacketsSinceLastReply << "seconds since last domain list request, squelching traffic";
        nodeList->setDropOutgoingNodeTraffic(true);
    }

    if (_checkInPacketsSinceLastReply > MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {

        // we haven't heard back from DS in MAX_SILENT_DOMAIN_SERVER_CHECK_INS
        // so emit our signal that says that

#ifdef DEBUG_EVENT_QUEUE
        int nodeListQueueSize = ::hifi::qt::getEventQueueSize(nodeList->thread());
        qCDebug(networking) << "Limit of silent domain checkins reached (network qt queue: " << nodeListQueueSize << ")";
#else  // DEBUG_EVENT_QUEUE
        qCDebug(networking) << "Limit of silent domain checkins reached";
#endif // DEBUG_EVENT_QUEUE

        emit limitOfSilentDomainCheckInsReached();
        return true;
    } else {
        return false;
    }
}
