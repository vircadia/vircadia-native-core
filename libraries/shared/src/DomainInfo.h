//
//  DomainInfo.h
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DomainInfo__
#define __hifi__DomainInfo__

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtNetwork/QHostInfo>

#include "HifiSockAddr.h"

const QString DEFAULT_DOMAIN_HOSTNAME = "root.highfidelity.io";
const unsigned short DEFAULT_DOMAIN_SERVER_PORT = 40102;

class DomainInfo : public QObject {
    Q_OBJECT
public:
    DomainInfo();
    
    const QString& getHostname() const { return _hostname; }
    void setHostname(const QString& hostname);
    
    const QHostAddress& getIP() const { return _sockAddr.getAddress(); }
    void setIPToLocalhost() { _sockAddr.setAddress(QHostAddress(QHostAddress::LocalHost)); }
    
    const HifiSockAddr& getSockAddr() { return _sockAddr; }
    void setSockAddr(const HifiSockAddr& sockAddr) { _sockAddr = sockAddr; }
    
    unsigned short getPort() const { return _sockAddr.getPort(); }
    
    bool requiresAuthentication() const { return _requiresAuthentication; }
    void setRequiresAuthentication(bool requiresAuthentication) { _requiresAuthentication = requiresAuthentication; }
    
    const QUuid& getConnectionSecret() const { return _connectionSecret; }
    void setConnectionSecret(const QUuid& connectionSecret) { _connectionSecret = connectionSecret; }
    
    const QString& getRegistrationToken() const { return _registrationToken; }
    void setRegistrationToken(const QString& registrationToken);
private slots:
    void completedHostnameLookup(const QHostInfo& hostInfo);
signals:
    void hostnameChanged(const QString& hostname);
private:
    QString _hostname;
    HifiSockAddr _sockAddr;
    bool _requiresAuthentication;
    QUuid _connectionSecret;
    QString _registrationToken;
};

#endif /* defined(__hifi__DomainInfo__) */
