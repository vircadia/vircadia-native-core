//
//  DomainHandler.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainHandler_h
#define hifi_DomainHandler_h

#include <QProcessEnvironment>

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtCore/QUrl>
#include <QtNetwork/QHostInfo>

#include <shared/ReadWriteLockable.h>
#include <SettingHandle.h>

#include "SockAddr.h"
#include "NetworkPeer.h"
#include "NLPacket.h"
#include "NLPacketList.h"
#include "Node.h"
#include "ReceivedMessage.h"
#include "NetworkingConstants.h"
#include "MetaverseAPI.h"

const unsigned short DEFAULT_DOMAIN_SERVER_PORT =
    QProcessEnvironment::systemEnvironment()
    .contains("HIFI_DOMAIN_SERVER_PORT")
        ? QProcessEnvironment::systemEnvironment()
            .value("HIFI_DOMAIN_SERVER_PORT")
            .toUShort()
        : 40102;  // UDP

const unsigned short DEFAULT_DOMAIN_SERVER_WS_PORT =
    QProcessEnvironment::systemEnvironment()
    .contains("VIRCADIA_DOMAIN_SERVER_WS_PORT")
        ? QProcessEnvironment::systemEnvironment()
            .value("VIRCADIA_DOMAIN_SERVER_WS_PORT")
            .toUShort()
        : 40102;  // TCP

const unsigned short DEFAULT_DOMAIN_SERVER_DTLS_PORT =
    QProcessEnvironment::systemEnvironment()
    .contains("HIFI_DOMAIN_SERVER_DTLS_PORT")
        ? QProcessEnvironment::systemEnvironment()
            .value("HIFI_DOMAIN_SERVER_DTLS_PORT")
            .toUShort()
        : 40103;

const quint16 DOMAIN_SERVER_HTTP_PORT =
    QProcessEnvironment::systemEnvironment()
    .contains("HIFI_DOMAIN_SERVER_HTTP_PORT")
        ? QProcessEnvironment::systemEnvironment()
            .value("HIFI_DOMAIN_SERVER_HTTP_PORT")
            .toUInt()
        : 40100;

const quint16 DOMAIN_SERVER_HTTPS_PORT =
    QProcessEnvironment::systemEnvironment()
    .contains("HIFI_DOMAIN_SERVER_HTTPS_PORT")
        ? QProcessEnvironment::systemEnvironment()
            .value("HIFI_DOMAIN_SERVER_HTTPS_PORT")
            .toUInt()
        : 40101;

const quint16 DOMAIN_SERVER_EXPORTER_PORT =
    QProcessEnvironment::systemEnvironment()
    .contains("VIRCADIA_DOMAIN_SERVER_EXPORTER_PORT")
        ? QProcessEnvironment::systemEnvironment()
            .value("VIRCADIA_DOMAIN_SERVER_EXPORTER_PORT")
            .toUInt()
        : 9703;
        
const quint16 DOMAIN_SERVER_METADATA_EXPORTER_PORT =
    QProcessEnvironment::systemEnvironment()
    .contains("VIRCADIA_DOMAIN_SERVER_METADATA_EXPORTER_PORT")
        ? QProcessEnvironment::systemEnvironment()
            .value("VIRCADIA_DOMAIN_SERVER_METADATA_EXPORTER_PORT")
            .toUInt()
        : 9704;

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

class DomainHandler : public QObject {
    Q_OBJECT
public:
    DomainHandler(QObject* parent = 0);

    void disconnect(QString reason);
    void clearSettings();

    const QUuid& getUUID() const { return _uuid; }
    void setUUID(const QUuid& uuid);

    Node::LocalID getLocalID() const { return _localID; }
    void setLocalID(Node::LocalID localID) { _localID = localID; }

    QString getScheme() const { return _domainURL.scheme(); }
    QString getHostname() const { return _domainURL.host(); }

    QUrl getErrorDomainURL(){ return _errorDomainURL; }
    void setErrorDomainURL(const QUrl& url);

    int getLastDomainConnectionError() { return _lastDomainConnectionError; }

    const QHostAddress& getIP() const { return _sockAddr.getAddress(); }
    void setIPToLocalhost() { _sockAddr.setAddress(QHostAddress(QHostAddress::LocalHost)); }

    const SockAddr& getSockAddr() const { return _sockAddr; }
    void setSockAddr(const SockAddr& sockAddr, const QString& hostname);

    unsigned short getPort() const { return _sockAddr.getPort(); }
    void setPort(quint16 port) { _sockAddr.setPort(port); }

    const QUuid& getConnectionToken() const { return _connectionToken; }
    void setConnectionToken(const QUuid& connectionToken) { _connectionToken = connectionToken; }

    const QUuid& getAssignmentUUID() const { return _assignmentUUID; }
    void setAssignmentUUID(const QUuid& assignmentUUID) { _assignmentUUID = assignmentUUID; }

    const QUuid& getPendingDomainID() const { return _pendingDomainID; }

    const QUuid& getICEClientID() const { return _iceClientID; }

    bool requiresICE() const { return !_iceServerSockAddr.isNull(); }
    const SockAddr& getICEServerSockAddr() const { return _iceServerSockAddr; }
    NetworkPeer& getICEPeer() { return _icePeer; }
    void activateICELocalSocket();
    void activateICEPublicSocket();

    bool isConnected() const { return _isConnected; }
    void setIsConnected(bool isConnected);

    void setCanConnectWithoutAvatarEntities(bool canConnect);
    bool canConnectWithoutAvatarEntities();

    bool isServerless() const { return _domainURL.scheme() != URL_SCHEME_VIRCADIA; }
    bool getInterstitialModeEnabled() const;
    void setInterstitialModeEnabled(bool enableInterstitialMode);

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

    void softReset(QString reason);

    int getCheckInPacketsSinceLastReply() const { return _checkInPacketsSinceLastReply; }
    bool checkInPacketTimeout();
    void clearPendingCheckins() { _checkInPacketsSinceLastReply = 0; }

    void resetConfirmConnectWithoutAvatarEntities() {
        _haveAskedConnectWithoutAvatarEntities = false;
    }

    /*@jsdoc
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
     *       <td><strong>LoginErrorMetaverse</strong></td>
     *       <td><code>2</code></td>
     *       <td>You could not be logged into the domain per your metaverse login.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>NotAuthorizedMetaverse</strong></td>
     *       <td><code>3</code></td>
     *       <td>You are not authorized to connect to the domain per your metaverse login.</td>
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
     *     <tr>
     *       <td><strong>LoginErrorDomain</strong></td>
     *       <td><code>2</code></td>
     *       <td>You could not be logged into the domain per your domain login.</td>
     *     </tr>
     *     <tr>
     *       <td><strong>NotAuthorizedDomain</strong></td>
     *       <td><code>6</code></td>
     *       <td>You are not authorized to connect to the domain per your domain login.</td>
     *     </tr>
     *   </tbody>
     * </table>
     * @typedef {number} Window.ConnectionRefusedReason
     */
    enum class ConnectionRefusedReason : uint8_t {
        Unknown,
        ProtocolMismatch,
        LoginErrorMetaverse,
        NotAuthorizedMetaverse,
        TooManyUsers,
        TimedOut,
        LoginErrorDomain,
        NotAuthorizedDomain
    };

public slots:
    void setURLAndID(QUrl domainURL, QUuid domainID);
    void setIceServerHostnameAndID(const QString& iceServerHostname, const QUuid& id);

    void processSettingsPacketList(QSharedPointer<ReceivedMessage> packetList);
    void processICEPingReplyPacket(QSharedPointer<ReceivedMessage> message);
    void processDTLSRequirementPacket(QSharedPointer<ReceivedMessage> dtlsRequirementPacket);
    void processICEResponsePacket(QSharedPointer<ReceivedMessage> icePacket);
    void processDomainServerConnectionDeniedPacket(QSharedPointer<ReceivedMessage> message);

    // sets domain handler in error state.
    void setRedirectErrorState(QUrl errorUrl, QString reasonMessage = "", int reasonCode = -1, const QString& extraInfo = "");

    bool isInErrorState() { return _isInErrorState; }

private slots:
    void completedHostnameLookup(const QHostInfo& hostInfo);
    void completedIceServerHostnameLookup();

signals:
    void domainURLChanged(QUrl domainURL);

    // NOTE: the emission of completedSocketDiscovery does not mean a connection to DS is established
    // It means that, either from DNS lookup or ICE, we think we have a socket we can talk to DS on
    void completedSocketDiscovery();

    void resetting();
    void confirmConnectWithoutAvatarEntities();
    void connectedToDomain(QUrl domainURL);
    void disconnectedFromDomain();

    void iceSocketAndIDReceived();
    void icePeerSocketsReceived();

    void settingsReceived(const QJsonObject& domainSettingsObject);
    void settingsReceiveFail();

    void domainConnectionRefused(QString reasonMessage, int reasonCode, const QString& extraInfo);
    void redirectToErrorDomainURL(QUrl errorDomainURL);
    void redirectErrorStateChanged(bool isInErrorState);

    void limitOfSilentDomainCheckInsReached();

private:
    bool reasonSuggestsMetaverseLogin(ConnectionRefusedReason reasonCode);
    bool reasonSuggestsDomainLogin(ConnectionRefusedReason reasonCode);
    void sendDisconnectPacket();
    void hardReset(QString reason);

    bool isHardRefusal(int reasonCode);

    QUuid _uuid;
    Node::LocalID _localID;
    QUrl _domainURL;
    QUrl _errorDomainURL;
    SockAddr _sockAddr;
    QUuid _assignmentUUID;
    QUuid _connectionToken;
    QUuid _pendingDomainID; // ID of domain being connected to, via ICE or direct connection
    QUuid _iceClientID;
    SockAddr _iceServerSockAddr;
    NetworkPeer _icePeer;
    bool _isConnected { false };
    bool _haveAskedConnectWithoutAvatarEntities { false };
    bool _canConnectWithoutAvatarEntities { false };
    bool _isInErrorState { false };
    QJsonObject _settingsObject;
    QString _pendingPath;
    QTimer _settingsTimer;
    mutable ReadWriteLockable _interstitialModeSettingLock;
#ifdef Q_OS_ANDROID
    Setting::Handle<bool> _enableInterstitialMode{ "enableInterstitialMode", false };
#else
    Setting::Handle<bool> _enableInterstitialMode { "enableInterstitialMode", false };
#endif

    QSet<QString> _domainConnectionRefusals;
    bool _hasCheckedForAccessToken { false };
    bool _hasCheckedForDomainAccessToken { false };
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
