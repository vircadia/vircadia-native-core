//
//  DomainHandler.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainHandler_h
#define hifi_DomainHandler_h

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtCore/QUrl>
#include <QtNetwork/QHostInfo>

#include "HifiSockAddr.h"
#include "NetworkPeer.h"
#include "NLPacket.h"
#include "NLPacketList.h"
#include "Node.h"
#include "ReceivedMessage.h"
#include "NetworkingConstants.h"

const unsigned short DEFAULT_DOMAIN_SERVER_PORT = 40102;
const unsigned short DEFAULT_DOMAIN_SERVER_DTLS_PORT = 40103;
const quint16 DOMAIN_SERVER_HTTP_PORT = 40100;
const quint16 DOMAIN_SERVER_HTTPS_PORT = 40101;

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

class DomainHandler : public QObject {
    Q_OBJECT
public:
    DomainHandler(QObject* parent = 0);

    void disconnect();
    void clearSettings();

    const QUuid& getUUID() const { return _uuid; }
    void setUUID(const QUuid& uuid);

    Node::LocalID getLocalID() const { return _localID; }
    void setLocalID(Node::LocalID localID) { _localID = localID; }

    QString getHostname() const { return _domainURL.host(); }

    QUrl getErrorDomainURL(){ return _errorDomainURL; }
    void setErrorDomainURL(const QUrl& url);

    int getLastDomainConnectionError() { return _lastDomainConnectionError; }

    const QHostAddress& getIP() const { return _sockAddr.getAddress(); }
    void setIPToLocalhost() { _sockAddr.setAddress(QHostAddress(QHostAddress::LocalHost)); }

    const HifiSockAddr& getSockAddr() const { return _sockAddr; }
    void setSockAddr(const HifiSockAddr& sockAddr, const QString& hostname);

    unsigned short getPort() const { return _sockAddr.getPort(); }
    void setPort(quint16 port) { _sockAddr.setPort(port); }

    const QUuid& getConnectionToken() const { return _connectionToken; }
    void setConnectionToken(const QUuid& connectionToken) { _connectionToken = connectionToken; }

    const QUuid& getAssignmentUUID() const { return _assignmentUUID; }
    void setAssignmentUUID(const QUuid& assignmentUUID) { _assignmentUUID = assignmentUUID; }

    const QUuid& getPendingDomainID() const { return _pendingDomainID; }

    const QUuid& getICEClientID() const { return _iceClientID; }

    bool requiresICE() const { return !_iceServerSockAddr.isNull(); }
    const HifiSockAddr& getICEServerSockAddr() const { return _iceServerSockAddr; }
    NetworkPeer& getICEPeer() { return _icePeer; }
    void activateICELocalSocket();
    void activateICEPublicSocket();

    bool isConnected() const { return _isConnected; }
    void setIsConnected(bool isConnected);
    bool isServerless() const { return _domainURL.scheme() != URL_SCHEME_HIFI; }

    void connectedToServerless(std::map<QString, QString> namedPaths);

    void loadedErrorDomain(std::map<QString, QString> namedPaths);

    QString getViewPointFromNamedPath(QString namedPath);

    bool hasSettings() const { return !_settingsObject.isEmpty(); }
    void requestDomainSettings();
    const QJsonObject& getSettingsObject() const { return _settingsObject; }

    void setPendingPath(const QString& pendingPath) { _pendingPath = pendingPath; }
    const QString& getPendingPath() { return _pendingPath; }
    void clearPendingPath() { _pendingPath.clear(); }

    bool isSocketKnown() const { return !_sockAddr.getAddress().isNull(); }

    void softReset();

    int getCheckInPacketsSinceLastReply() const { return _checkInPacketsSinceLastReply; }
    bool checkInPacketTimeout();
    void clearPendingCheckins() { _checkInPacketsSinceLastReply = 0; }

    /**jsdoc
     * <p>The reasons that you may be refused connection to a domain are defined by numeric values:</p>
     * <table>
     *   <thead>
     *     <tr>
     *       <th>Reason</th>
     *       <th>Value</th>
     *       <th>Description</th>
     *     </tr>
     *   </thead>
     *   <tbody>
     *     <tr>
     *       <td><strong>Unknown</strong></td>
     *       <td><code>0</code></td>
     *       <td>Some unknown reason.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>ProtocolMismatch</strong></td>
     *       <td><code>1</code></td>
     *       <td>The communications protocols of the domain and your Interface are not the same.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>LoginError</strong></td>
     *       <td><code>2</code></td>
     *       <td>You could not be logged into the domain.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>NotAuthorized</strong></td>
     *       <td><code>3</code></td>
     *       <td>You are not authorized to connect to the domain.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>TooManyUsers</strong></td>
     *       <td><code>4</code></td>
     *       <td>The domain already has its maximum number of users.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>TimedOut</strong></td>
     *       <td><code>5</code></td>
     *       <td>Connecting to the domain timed out.</td>
     *     </tr>
     *   </tbody>
     * </table>
     * @typedef {number} Window.ConnectionRefusedReason
     */
    enum class ConnectionRefusedReason : uint8_t {
        Unknown,
        ProtocolMismatch,
        LoginError,
        NotAuthorized,
        TooManyUsers,
        TimedOut
    };

public slots:
    void setURLAndID(QUrl domainURL, QUuid id);
    void setIceServerHostnameAndID(const QString& iceServerHostname, const QUuid& id);

    void processSettingsPacketList(QSharedPointer<ReceivedMessage> packetList);
    void processICEPingReplyPacket(QSharedPointer<ReceivedMessage> message);
    void processDTLSRequirementPacket(QSharedPointer<ReceivedMessage> dtlsRequirementPacket);
    void processICEResponsePacket(QSharedPointer<ReceivedMessage> icePacket);
    void processDomainServerConnectionDeniedPacket(QSharedPointer<ReceivedMessage> message);

    // sets domain handler in error state.
    void setRedirectErrorState(QUrl errorUrl, int reasonCode);

    bool isInErrorState() { return _isInErrorState; }

private slots:
    void completedHostnameLookup(const QHostInfo& hostInfo);
    void completedIceServerHostnameLookup();

signals:
    void domainURLChanged(QUrl domainURL);

    void domainConnectionErrorChanged(int reasonCode);

    // NOTE: the emission of completedSocketDiscovery does not mean a connection to DS is established
    // It means that, either from DNS lookup or ICE, we think we have a socket we can talk to DS on
    void completedSocketDiscovery();

    void resetting();
    void connectedToDomain(QUrl domainURL);
    void disconnectedFromDomain();

    void iceSocketAndIDReceived();
    void icePeerSocketsReceived();

    void settingsReceived(const QJsonObject& domainSettingsObject);
    void settingsReceiveFail();

    void domainConnectionRefused(QString reasonMessage, int reason, const QString& extraInfo);
    void redirectToErrorDomainURL(QUrl errorDomainURL);

    void limitOfSilentDomainCheckInsReached();

private:
    bool reasonSuggestsLogin(ConnectionRefusedReason reasonCode);
    void sendDisconnectPacket();
    void hardReset();

    QUuid _uuid;
    Node::LocalID _localID;
    QUrl _domainURL;
    QUrl _errorDomainURL;
    HifiSockAddr _sockAddr;
    QUuid _assignmentUUID;
    QUuid _connectionToken;
    QUuid _pendingDomainID; // ID of domain being connected to, via ICE or direct connection
    QUuid _iceClientID;
    HifiSockAddr _iceServerSockAddr;
    NetworkPeer _icePeer;
    bool _isConnected { false };
    bool _isInErrorState { false };
    QJsonObject _settingsObject;
    QString _pendingPath;
    QTimer _settingsTimer;

    QSet<QString> _domainConnectionRefusals;
    bool _hasCheckedForAccessToken { false };
    int _connectionDenialsSinceKeypairRegen { 0 };
    int _checkInPacketsSinceLastReply { 0 };

    QTimer _apiRefreshTimer;

    std::map<QString, QString> _namedPaths;

    // domain connection error upon connection refusal.
    int _lastDomainConnectionError{ -1 };
};

const QString DOMAIN_SPAWNING_POINT { "/0, -10, 0" };
const QString DEFAULT_NAMED_PATH { "/" };

#endif // hifi_DomainHandler_h
