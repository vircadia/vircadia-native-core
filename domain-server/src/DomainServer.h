//
//  DomainServer.h
//  domain-server/src
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainServer_h
#define hifi_DomainServer_h

#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

#include <gnutls/gnutls.h>

#include <Assignment.h>
#include <HTTPSConnection.h>
#include <LimitedNodeList.h>

#include "DTLSServerSession.h"

typedef QSharedPointer<Assignment> SharedAssignmentPointer;

class DomainServer : public QCoreApplication, public HTTPSRequestHandler {
    Q_OBJECT
public:
    DomainServer(int argc, char* argv[]);
    ~DomainServer();
    
    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url);
    bool handleHTTPSRequest(HTTPSConnection* connection, const QUrl& url);
    
    void exit(int retCode = 0);
    
public slots:
    /// Called by NodeList to inform us a node has been added
    void nodeAdded(SharedNodePointer node);
    /// Called by NodeList to inform us a node has been killed
    void nodeKilled(SharedNodePointer node);
    
private slots:
    
    void readAvailableDatagrams();
    void readAvailableDTLSDatagrams();
private:
    void setupNodeListAndAssignments(const QUuid& sessionUUID = QUuid::createUuid());
    bool optionallySetupOAuth();
    bool optionallySetupDTLS();
    bool optionallyReadX509KeyAndCertificate();
    
    void processDatagram(const QByteArray& receivedPacket, const HifiSockAddr& senderSockAddr);
    
    void handleConnectRequest(const QByteArray& packet, const HifiSockAddr& senderSockAddr);
    int parseNodeDataFromByteArray(NodeType_t& nodeType, HifiSockAddr& publicSockAddr,
                                    HifiSockAddr& localSockAddr, const QByteArray& packet, const HifiSockAddr& senderSockAddr);
    NodeSet nodeInterestListFromPacket(const QByteArray& packet, int numPreceedingBytes);
    void sendDomainListToNode(const SharedNodePointer& node, const HifiSockAddr& senderSockAddr,
                              const NodeSet& nodeInterestList);
    
    void parseAssignmentConfigs(QSet<Assignment::Type>& excludedTypes);
    void addStaticAssignmentToAssignmentHash(Assignment* newAssignment);
    void createScriptedAssignmentsFromArray(const QJsonArray& configArray);
    void createStaticAssignmentsForType(Assignment::Type type, const QJsonArray& configArray);
    void populateDefaultStaticAssignmentsExcludingTypes(const QSet<Assignment::Type>& excludedTypes);
    
    SharedAssignmentPointer matchingQueuedAssignmentForCheckIn(const QUuid& checkInUUID, NodeType_t nodeType);
    SharedAssignmentPointer deployableAssignmentForRequest(const Assignment& requestAssignment);
    void removeMatchingAssignmentFromQueue(const SharedAssignmentPointer& removableAssignment);
    void refreshStaticAssignmentAndAddToQueue(SharedAssignmentPointer& assignment);
    void addStaticAssignmentsToQueue();
    
    QUrl oauthAuthorizationURL();
    
    QJsonObject jsonForSocket(const HifiSockAddr& socket);
    QJsonObject jsonObjectForNode(const SharedNodePointer& node);
    
    HTTPManager _httpManager;
    HTTPSManager* _httpsManager;
    
    QHash<QUuid, SharedAssignmentPointer> _allAssignments;
    QQueue<SharedAssignmentPointer> _unfulfilledAssignments;
    
    QVariantMap _argumentVariantMap;
    
    bool _isUsingDTLS;
    gnutls_certificate_credentials_t* _x509Credentials;
    gnutls_dh_params_t* _dhParams;
    gnutls_datum_t* _cookieKey;
    gnutls_priority_t* _priorityCache;
    
    QHash<HifiSockAddr, DTLSServerSession*> _dtlsSessions;
    
    QUrl _oauthProviderURL;
    QString _oauthClientID;
    QString _oauthClientSecret;
    QString _hostname;
};

#endif // hifi_DomainServer_h
