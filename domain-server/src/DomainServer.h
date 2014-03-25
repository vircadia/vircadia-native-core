//
//  DomainServer.h
//  hifi
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DomainServer__
#define __hifi__DomainServer__

#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

#include <Assignment.h>
#include <HTTPManager.h>
#include <NodeList.h>

typedef QSharedPointer<Assignment> SharedAssignmentPointer;

class DomainServer : public QCoreApplication, public HTTPRequestHandler {
    Q_OBJECT
public:
    DomainServer(int argc, char* argv[]);
    
    bool requiresAuthentication() const { return !_nodeAuthenticationURL.isEmpty(); }
    
    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url);
    
    void exit(int retCode = 0);
    
public slots:
    /// Called by NodeList to inform us a node has been added
    void nodeAdded(SharedNodePointer node);
    /// Called by NodeList to inform us a node has been killed
    void nodeKilled(SharedNodePointer node);
    
private:
    void setupNodeListAndAssignments(const QUuid& sessionUUID = QUuid::createUuid());
    
    void requestAuthenticationFromPotentialNode(const HifiSockAddr& senderSockAddr);
    void addNodeToNodeListAndConfirmConnection(const QByteArray& packet, const HifiSockAddr& senderSockAddr,
                                               const QJsonObject& authJsonObject = QJsonObject());
    int parseNodeDataFromByteArray(NodeType_t& nodeType, HifiSockAddr& publicSockAddr,
                                    HifiSockAddr& localSockAddr, const QByteArray& packet, const HifiSockAddr& senderSockAddr);
    NodeSet nodeInterestListFromPacket(const QByteArray& packet, int numPreceedingBytes);
    void sendDomainListToNode(const SharedNodePointer& node, const HifiSockAddr& senderSockAddr,
                              const NodeSet& nodeInterestList);
    
    void parseCommandLineTypeConfigs(const QStringList& argumentList, QSet<Assignment::Type>& excludedTypes);
    void readConfigFile(const QString& path, QSet<Assignment::Type>& excludedTypes);
    QString readServerAssignmentConfig(const QJsonObject& jsonObject, const QString& nodeName);
    void addStaticAssignmentToAssignmentHash(Assignment* newAssignment);
    void createStaticAssignmentsForTypeGivenConfigString(Assignment::Type type, const QString& configString);
    void populateDefaultStaticAssignmentsExcludingTypes(const QSet<Assignment::Type>& excludedTypes);
    
    SharedAssignmentPointer matchingStaticAssignmentForCheckIn(const QUuid& checkInUUID, NodeType_t nodeType);
    SharedAssignmentPointer deployableAssignmentForRequest(const Assignment& requestAssignment);
    void removeMatchingAssignmentFromQueue(const SharedAssignmentPointer& removableAssignment);
    void refreshStaticAssignmentAndAddToQueue(SharedAssignmentPointer& assignment);
    void addStaticAssignmentsToQueue();
    
    QJsonObject jsonForSocket(const HifiSockAddr& socket);
    QJsonObject jsonObjectForNode(const SharedNodePointer& node);
    
    HTTPManager _HTTPManager;
    
    QHash<QUuid, SharedAssignmentPointer> _staticAssignmentHash;
    QQueue<SharedAssignmentPointer> _assignmentQueue;
    
    QUrl _nodeAuthenticationURL;
    
    QStringList _argumentList;
    
    QHash<QString, QJsonObject> _redeemedTokenResponses;
private slots:
    void requestCreationFromDataServer();
    void processCreateResponseFromDataServer(const QJsonObject& jsonObject);
    void processTokenRedeemResponse(const QJsonObject& jsonObject);
    
    void readAvailableDatagrams();
};

#endif /* defined(__hifi__DomainServer__) */
