//
//  DomainHandler.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainHandler.h"

#include <math.h>

#include <PathUtils.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QDataStream>

#include "AddressManager.h"
#include "Assignment.h"
#include "HifiSockAddr.h"
#include "NodeList.h"
#include "udt/Packet.h"
#include "udt/PacketHeaders.h"
#include "NLPacket.h"
#include "SharedUtil.h"
#include "UserActivityLogger.h"
#include "NetworkLogging.h"

DomainHandler::DomainHandler(QObject* parent) :
    QObject(parent),
    _sockAddr(HifiSockAddr(QHostAddress::Null, DEFAULT_DOMAIN_SERVER_PORT)),
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

void DomainHandler::disconnect() {
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

void DomainHandler::softReset() {
    qCDebug(networking) << "Resetting current domain connection information.";
    disconnect();

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

void DomainHandler::hardReset() {
    emit resetting();

    softReset();
    _isInErrorState = false;

    qCDebug(networking) << "Hard reset in NodeList DomainHandler.";
    _pendingDomainID = QUuid();
    _iceServerSockAddr = HifiSockAddr();
    _sockAddr.clear();
    _domainURL = QUrl();

    _domainConnectionRefusals.clear();

    _hasCheckedForAccessToken = false;

    // clear any pending path we may have wanted to ask the previous DS about
    _pendingPath.clear();
}

void DomainHandler::setErrorDomainURL(const QUrl& url) {
    _errorDomainURL = url;
    return;
}

void DomainHandler::setSockAddr(const HifiSockAddr& sockAddr, const QString& hostname) {
    if (_sockAddr != sockAddr) {
        // we should reset on a sockAddr change
        hardReset();
        // change the sockAddr
        _sockAddr = sockAddr;
    }

    if (!_sockAddr.isNull()) {
        DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    }

    // some callers may pass a hostname, this is not to be used for lookup but for DTLS certificate verification
    _domainURL = QUrl();
    _domainURL.setScheme(URL_SCHEME_HIFI);
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

    if (domainURL.scheme() != URL_SCHEME_HIFI) {
        _sockAddr.clear();

        // if this is a file URL we need to see if it has a ~ for us to expand
        if (domainURL.scheme() == URL_SCHEME_FILE) {
            domainURL = PathUtils::expandToLocalDataAbsolutePath(domainURL);
        }
    }

    auto domainPort = domainURL.port();
    if (domainPort == -1) {
        domainPort = DEFAULT_DOMAIN_SERVER_PORT;
    }

    // if it's in the error state, reset and try again.
    if ((_domainURL != domainURL || _sockAddr.getPort() != domainPort) || _isInErrorState) {
        // re-set the domain info so that auth information is reloaded
        hardReset();

        QString previousHost = _domainURL.host();
        _domainURL = domainURL;

        if (previousHost != domainURL.host()) {
            qCDebug(networking) << "Updated domain hostname to" << domainURL.host();

            if (!domainURL.host().isEmpty()) {
                if (domainURL.scheme() == URL_SCHEME_HIFI) {
                    // re-set the sock addr to null and fire off a lookup of the IP address for this domain-server's hostname
                    qCDebug(networking, "Looking up DS hostname %s.", domainURL.host().toLocal8Bit().constData());
                    QHostInfo::lookupHost(domainURL.host(), this, SLOT(completedHostnameLookup(const QHostInfo&)));
                }

                DependencyManager::get<NodeList>()->flagTimeForConnectionStep(
                    LimitedNodeList::ConnectionStep::SetDomainHostname);

                UserActivityLogger::getInstance().changedDomain(domainURL.host());
            }
        }

        emit domainURLChanged(_domainURL);

        if (_sockAddr.getPort() != domainPort) {
            qCDebug(networking) << "Updated domain port to" << domainPort;
            _sockAddr.setPort(domainPort);
        }
    }
}

void DomainHandler::setIceServerHostnameAndID(const QString& iceServerHostname, const QUuid& id) {

    // if it's in the error state, reset and try again.
    if ((_iceServerSockAddr.getAddress().toString() != iceServerHostname || id != _pendingDomainID) || _isInErrorState) {
        // re-set the domain info to connect to new domain
        hardReset();

        // refresh our ICE client UUID to something new
        _iceClientID = QUuid::createUuid();

        _pendingDomainID = id;

        HifiSockAddr* replaceableSockAddr = &_iceServerSockAddr;
        replaceableSockAddr->~HifiSockAddr();
        replaceableSockAddr = new (replaceableSockAddr) HifiSockAddr(iceServerHostname, ICE_SERVER_DEFAULT_PORT);
        _iceServerSockAddr.setObjectName("IceServer");

        auto nodeList = DependencyManager::get<NodeList>();

        nodeList->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetICEServerHostname);

        if (_iceServerSockAddr.getAddress().isNull()) {
            // connect to lookup completed for ice-server socket so we can request a heartbeat once hostname is looked up
            connect(&_iceServerSockAddr, &HifiSockAddr::lookupCompleted, this, &DomainHandler::completedIceServerHostnameLookup);
        } else {
            completedIceServerHostnameLookup();
        }

        qCDebug(networking) << "ICE required to connect to domain via ice server at" << iceServerHostname;
    }
}

void DomainHandler::activateICELocalSocket() {
    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    _sockAddr = _icePeer.getLocalSocket();
    _domainURL.setScheme(URL_SCHEME_HIFI);
    _domainURL.setHost(_sockAddr.getAddress().toString());
    emit domainURLChanged(_domainURL);
    emit completedSocketDiscovery();
}

void DomainHandler::activateICEPublicSocket() {
    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    _sockAddr = _icePeer.getPublicSocket();
    _domainURL.setScheme(URL_SCHEME_HIFI);
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
    qCDebug(networking) << "ICE server socket is at" << _iceServerSockAddr;

    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetICEServerSocket);

    // emit our signal so we can send a heartbeat to ice-server immediately
    emit iceSocketAndIDReceived();
}

void DomainHandler::setIsConnected(bool isConnected) {
    if (_isConnected != isConnected) {
        _isConnected = isConnected;

        if (_isConnected) {
            emit connectedToDomain(_domainURL);

            if (_domainURL.scheme() == URL_SCHEME_HIFI && !_domainURL.host().isEmpty()) {
                // we've connected to new domain - time to ask it for global settings
                requestDomainSettings();
            }

        } else {
            emit disconnectedFromDomain();
        }
    }
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

void DomainHandler::setRedirectErrorState(QUrl errorUrl, int reasonCode) {
    _errorDomainURL = errorUrl;
    _lastDomainConnectionError = reasonCode;
    _isInErrorState = true;
    emit redirectToErrorDomainURL(_errorDomainURL);
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
    const HifiSockAddr& senderSockAddr = message->getSenderSockAddr();
    qCDebug(networking) << "Received reply from domain-server on" << senderSockAddr;

    if (getIP().isNull()) {
        // we're hearing back from this domain-server, no need to refresh API information
        _apiRefreshTimer.stop();

        // for now we're unsafely assuming this came back from the domain
        if (senderSockAddr == _icePeer.getLocalSocket()) {
            qCDebug(networking) << "Connecting to domain using local socket";
            activateICELocalSocket();
        } else if (senderSockAddr == _icePeer.getPublicSocket()) {
            qCDebug(networking) << "Conecting to domain using public socket";
            activateICEPublicSocket();
        } else {
            qCDebug(networking) << "Reply does not match either local or public socket for domain. Will not connect.";
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
        qCDebug(networking) << "Received an ICE peer packet for domain-server but we already have sockets. Not processing.";
        // bail on processing this packet if our ice peer already has sockets
        return;
    }

    // start or restart the API refresh timer now that we have new information
    _apiRefreshTimer.start();

    QDataStream iceResponseStream(message->getMessage());

    iceResponseStream >> _icePeer;

    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::ReceiveDSPeerInformation);

    if (_icePeer.getUUID() != _pendingDomainID) {
        qCDebug(networking) << "Received a network peer with ID that does not match current domain. Will not attempt connection.";
        _icePeer.reset();
    } else {
        qCDebug(networking) << "Received network peer object for domain -" << _icePeer;

        // ask the peer object to start its ping timer
        _icePeer.startPingTimer();

        // emit our signal so the NodeList knows to send a ping immediately
        emit icePeerSocketsReceived();
    }
}

bool DomainHandler::reasonSuggestsLogin(ConnectionRefusedReason reasonCode) {
    switch (reasonCode) {
        case ConnectionRefusedReason::LoginError:
        case ConnectionRefusedReason::NotAuthorized:
            return true;
    
        default:
        case ConnectionRefusedReason::Unknown:
        case ConnectionRefusedReason::ProtocolMismatch:
        case ConnectionRefusedReason::TooManyUsers:
            return false;
    }
    return false;
}

void DomainHandler::processDomainServerConnectionDeniedPacket(QSharedPointer<ReceivedMessage> message) {
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
    qCWarning(networking) << "The domain-server denied a connection request: " << reasonMessage << " extraInfo:" << extraInfo;

    if (!_domainConnectionRefusals.contains(reasonMessage)) {
        _domainConnectionRefusals.insert(reasonMessage);
#if defined(Q_OS_ANDROID)
        emit domainConnectionRefused(reasonMessage, (int)reasonCode, extraInfo);
#else
        if (reasonCode == ConnectionRefusedReason::ProtocolMismatch || reasonCode == ConnectionRefusedReason::NotAuthorized) {
            // ingest the error - this is a "hard" connection refusal.
            setRedirectErrorState(_errorDomainURL, (int)reasonCode);
        } else {
            emit domainConnectionRefused(reasonMessage, (int)reasonCode, extraInfo);
        }
        _lastDomainConnectionError = (int)reasonCode;
#endif
    }

    auto accountManager = DependencyManager::get<AccountManager>();

    // Some connection refusal reasons imply that a login is required. If so, suggest a new login
    if (reasonSuggestsLogin(reasonCode)) {
        qCWarning(networking) << "Make sure you are logged in.";

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
    }
}

bool DomainHandler::checkInPacketTimeout() {
    ++_checkInPacketsSinceLastReply;

    if (_checkInPacketsSinceLastReply > MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
        // we haven't heard back from DS in MAX_SILENT_DOMAIN_SERVER_CHECK_INS
        // so emit our signal that says that
        qCDebug(networking) << "Limit of silent domain checkins reached";
        emit limitOfSilentDomainCheckInsReached();
        return true;
    } else {
        return false;
    }
}
