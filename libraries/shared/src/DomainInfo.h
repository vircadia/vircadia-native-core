//
//  DomainInfo.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__DomainInfo__
#define __hifi__DomainInfo__

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QUrl>
#include <QtNetwork/QHostInfo>

#include "HifiSockAddr.h"

const QString DEFAULT_DOMAIN_HOSTNAME = "alpha.highfidelity.io";
const unsigned short DEFAULT_DOMAIN_SERVER_PORT = 40102;

class DomainInfo : public QObject {
    Q_OBJECT
public:
    DomainInfo();
    
    void clearConnectionInfo();
    
    void parseAuthInformationFromJsonObject(const QJsonObject& jsonObject);
    
    const QUuid& getUUID() const { return _uuid; }
    void setUUID(const QUuid& uuid) { _uuid = uuid; }

    const QString& getHostname() const { return _hostname; }
    void setHostname(const QString& hostname);
    
    const QHostAddress& getIP() const { return _sockAddr.getAddress(); }
    void setIPToLocalhost() { _sockAddr.setAddress(QHostAddress(QHostAddress::LocalHost)); }
    
    const HifiSockAddr& getSockAddr() { return _sockAddr; }
    void setSockAddr(const HifiSockAddr& sockAddr);
    
    unsigned short getPort() const { return _sockAddr.getPort(); }
    
    const QUuid& getAssignmentUUID() const { return _assignmentUUID; }
    void setAssignmentUUID(const QUuid& assignmentUUID) { _assignmentUUID = assignmentUUID; }
    
    const QUuid& getConnectionSecret() const { return _connectionSecret; }
    void setConnectionSecret(const QUuid& connectionSecret) { _connectionSecret = connectionSecret; }
    
    const QByteArray& getRegistrationToken() const { return _registrationToken; }
    
    const QUrl& getRootAuthenticationURL() const { return _rootAuthenticationURL; }
    void setRootAuthenticationURL(const QUrl& rootAuthenticationURL) { _rootAuthenticationURL = rootAuthenticationURL; }
    
    bool isConnected() const { return _isConnected; }
    void setIsConnected(bool isConnected);
    
private slots:
    void completedHostnameLookup(const QHostInfo& hostInfo);
    
    void logout();
signals:
    void hostnameChanged(const QString& hostname);
    void connectedToDomain(const QString& hostname);
private:
    void reset();
    
    QUuid _uuid;
    QString _hostname;
    HifiSockAddr _sockAddr;
    QUuid _assignmentUUID;
    QUuid _connectionSecret;
    QByteArray _registrationToken;
    QUrl _rootAuthenticationURL;
    QString _publicKey;
    bool _isConnected;
};

#endif /* defined(__hifi__DomainInfo__) */
