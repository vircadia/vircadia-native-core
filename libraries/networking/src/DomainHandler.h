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

const unsigned short DEFAULT_DOMAIN_SERVER_PORT = 40102;
const unsigned short DEFAULT_DOMAIN_SERVER_DTLS_PORT = 40103;
const quint16 DOMAIN_SERVER_HTTP_PORT = 40100;
const quint16 DOMAIN_SERVER_HTTPS_PORT = 40101;

class DomainHandler : public QObject {
    Q_OBJECT
public:
    DomainHandler(QObject* parent = 0);
    
    void clearConnectionInfo();
    void clearSettings();

    const QUuid& getUUID() const { return _uuid; }
    void setUUID(const QUuid& uuid);

    const QString& getHostname() const { return _hostname; }

    const QHostAddress& getIP() const { return _sockAddr.getAddress(); }
    void setIPToLocalhost() { _sockAddr.setAddress(QHostAddress(QHostAddress::LocalHost)); }

    const HifiSockAddr& getSockAddr() { return _sockAddr; }
    void setSockAddr(const HifiSockAddr& sockAddr, const QString& hostname);

    unsigned short getPort() const { return _sockAddr.getPort(); }
    void setPort(quint16 port) { _sockAddr.setPort(port); }

    const QUuid& getConnectionToken() const { return _connectionToken; }
    void setConnectionToken(const QUuid& connectionToken) { _connectionToken = connectionToken; }
    
    const QUuid& getAssignmentUUID() const { return _assignmentUUID; }
    void setAssignmentUUID(const QUuid& assignmentUUID) { _assignmentUUID = assignmentUUID; }
    
    const QUuid& getICEDomainID() const { return _iceDomainID; }

    const QUuid& getICEClientID() const { return _iceClientID; }

    bool requiresICE() const { return !_iceServerSockAddr.isNull(); }
    const HifiSockAddr& getICEServerSockAddr() const { return _iceServerSockAddr; }
    NetworkPeer& getICEPeer() { return _icePeer; }
    void activateICELocalSocket();
    void activateICEPublicSocket();

    bool isConnected() const { return _isConnected; }
    void setIsConnected(bool isConnected);

    bool hasSettings() const { return !_settingsObject.isEmpty(); }
    void requestDomainSettings();
    const QJsonObject& getSettingsObject() const { return _settingsObject; }

   
    void setPendingPath(const QString& pendingPath) { _pendingPath = pendingPath; }
    const QString& getPendingPath() { return _pendingPath; }
    void clearPendingPath() { _pendingPath.clear(); }

    bool isSocketKnown() const { return !_sockAddr.getAddress().isNull(); }

    void softReset();
public slots:
    void setHostnameAndPort(const QString& hostname, quint16 port = DEFAULT_DOMAIN_SERVER_PORT);
    void setIceServerHostnameAndID(const QString& iceServerHostname, const QUuid& id);

    void processSettingsPacketList(QSharedPointer<NLPacketList> packetList);
    void processICEPingReplyPacket(QSharedPointer<NLPacket> packet);
    void processDTLSRequirementPacket(QSharedPointer<NLPacket> dtlsRequirementPacket);
    void processICEResponsePacket(QSharedPointer<NLPacket> icePacket);

private slots:
    void completedHostnameLookup(const QHostInfo& hostInfo);
    void completedIceServerHostnameLookup();

signals:
    void hostnameChanged(const QString& hostname);

    // NOTE: the emission of completedSocketDiscovery does not mean a connection to DS is established
    // It means that, either from DNS lookup or ICE, we think we have a socket we can talk to DS on
    void completedSocketDiscovery();

    void connectedToDomain(const QString& hostname);
    void disconnectedFromDomain();

    void iceSocketAndIDReceived();
    void icePeerSocketsReceived();

    void settingsReceived(const QJsonObject& domainSettingsObject);
    void settingsReceiveFail();

private:
    void hardReset();

    QUuid _uuid;
    QString _hostname;
    HifiSockAddr _sockAddr;
    QUuid _assignmentUUID;
    QUuid _connectionToken;
    QUuid _iceDomainID;
    QUuid _iceClientID;
    HifiSockAddr _iceServerSockAddr;
    NetworkPeer _icePeer;
    bool _isConnected;
    QJsonObject _settingsObject;
    int _failedSettingsRequests;
    QString _pendingPath;
};

#endif // hifi_DomainHandler_h
