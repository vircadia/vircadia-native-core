//
//  DomainHandler.h
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DomainHandler__
#define __hifi__DomainHandler__

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QUrl>
#include <QtNetwork/QHostInfo>

#include "DTLSClientSession.h"
#include "HifiSockAddr.h"

const QString DEFAULT_DOMAIN_HOSTNAME = "alpha.highfidelity.io";
const unsigned short DEFAULT_DOMAIN_SERVER_PORT = 40102;
const unsigned short DEFAULT_DOMAIN_SERVER_DTLS_PORT = 40103;

class DomainHandler : public QObject {
    Q_OBJECT
public:
    DomainHandler();
    ~DomainHandler();
    
    void clearConnectionInfo();
    
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
    
    bool isConnected() const { return _isConnected; }
    void setIsConnected(bool isConnected);
    
    void parseDTLSRequirementPacket(const QByteArray& dtlsRequirementPacket);
    
private slots:
    void completedHostnameLookup(const QHostInfo& hostInfo);
signals:
    void hostnameChanged(const QString& hostname);
    void connectedToDomain(const QString& hostname);
    void initializedDTLSSession();
private:
    void reset();
    void initializeDTLSSession();
    
    QUuid _uuid;
    QString _hostname;
    HifiSockAddr _sockAddr;
    QUuid _assignmentUUID;
    bool _isConnected;
    bool _requiresDTLS;
    DTLSClientSession* _dtlsSession;
};

#endif /* defined(__hifi__DomainHandler__) */
