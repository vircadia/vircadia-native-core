//
//  NodeList.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NodeList_h
#define hifi_NodeList_h

#include <stdint.h>
#include <iterator>
#include <assert.h>

#ifndef _WIN32
#include <unistd.h> // not on windows, not needed for mac or windows
#endif

#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include <DependencyManager.h>

#include "DomainHandler.h"
#include "LimitedNodeList.h"
#include "Node.h"

const quint64 DOMAIN_SERVER_CHECK_IN_MSECS = 1 * 1000;

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

using NodePacketPair = std::pair<SharedNodePointer, std::unique_ptr<NLPacket>>;
using NodeSharedPacketPair = std::pair<SharedNodePointer, QSharedPointer<NLPacket>>;

class Application;
class Assignment;

class NodeList : public LimitedNodeList {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    NodeType_t getOwnerType() const { return _ownerType; }
    void setOwnerType(NodeType_t ownerType) { _ownerType = ownerType; }

    qint64 sendStats(const QJsonObject& statsObject, const HifiSockAddr& destination);
    qint64 sendStatsToDomainServer(const QJsonObject& statsObject);

    int getNumNoReplyDomainCheckIns() const { return _numNoReplyDomainCheckIns; }
    DomainHandler& getDomainHandler() { return _domainHandler; }

    const NodeSet& getNodeInterestSet() const { return _nodeTypesOfInterest; }
    void addNodeTypeToInterestSet(NodeType_t nodeTypeToAdd);
    void addSetOfNodeTypesToNodeInterestSet(const NodeSet& setOfNodeTypes);
    void resetNodeInterestSet() { _nodeTypesOfInterest.clear(); }

    void processReceivedPacket(std::unique_ptr<NLPacket>, HifiSockAddr senderSockAddr);

    void setAssignmentServerSocket(const HifiSockAddr& serverSocket) { _assignmentServerSocket = serverSocket; }
    void sendAssignment(Assignment& assignment);

public slots:
    void reset();
    void sendDomainServerCheckIn();
    void handleDSPathQuery(const QString& newPath);

    void processDomainServerList(QSharedPointer<NLPacket> packet);
    void processDomainServerAddedNode(QSharedPointer<NLPacket> packet);
    void processDomainServerPathResponse(QSharedPointer<NLPacket> packet);

    void processPingPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode);
    void processPingReplyPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode);

    void processICEPingPacket(QSharedPointer<NLPacket> packet);
signals:
    void limitOfSilentDomainCheckInsReached();
private slots:
    void sendPendingDSPathQuery();
    void handleICEConnectionToDomainServer();

    void startNodeHolePunch(const SharedNodePointer& node);
    void handleNodePingTimeout();

    void pingPunchForDomainServer();
private:
    NodeList() : LimitedNodeList(0, 0) { assert(false); } // Not implemented, needed for DependencyManager templates compile
    NodeList(char ownerType, unsigned short socketListenPort = 0, unsigned short dtlsListenPort = 0);
    NodeList(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(NodeList const&); // Don't implement, needed to avoid copies of singleton

    void processDomainServerAuthRequest(const QByteArray& packet);
    void requestAuthForDomainServer();
    void activateSocketFromNodeCommunication(QSharedPointer<NLPacket> packet, const SharedNodePointer& sendingNode);
    void timePingReply(QSharedPointer<NLPacket> packet, const SharedNodePointer& sendingNode);

    void sendDSPathQuery(const QString& newPath);
 
    void parseNodeFromPacketStream(QDataStream& packetStream);

    void pingPunchForInactiveNode(const SharedNodePointer& node);

    NodeType_t _ownerType;
    NodeSet _nodeTypesOfInterest;
    DomainHandler _domainHandler;
    int _numNoReplyDomainCheckIns;
    HifiSockAddr _assignmentServerSocket;
};

#endif // hifi_NodeList_h
