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

#include <tbb/concurrent_unordered_set.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include <DependencyManager.h>
#include <SettingHandle.h>

#include "DomainHandler.h"
#include "LimitedNodeList.h"
#include "Node.h"

const quint64 DOMAIN_SERVER_CHECK_IN_MSECS = 1 * 1000;

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

using NodePacketPair = std::pair<SharedNodePointer, std::unique_ptr<NLPacket>>;
using NodeSharedPacketPair = std::pair<SharedNodePointer, QSharedPointer<NLPacket>>;
using NodeSharedReceivedMessagePair = std::pair<SharedNodePointer, QSharedPointer<ReceivedMessage>>;

class Application;
class Assignment;

class NodeList : public LimitedNodeList {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    NodeType_t getOwnerType() const { return _ownerType.load(); }
    void setOwnerType(NodeType_t ownerType) { _ownerType.store(ownerType); }

    Q_INVOKABLE qint64 sendStats(QJsonObject statsObject, HifiSockAddr destination);
    Q_INVOKABLE qint64 sendStatsToDomainServer(QJsonObject statsObject);

    int getNumNoReplyDomainCheckIns() const { return _numNoReplyDomainCheckIns; }
    DomainHandler& getDomainHandler() { return _domainHandler; }

    const NodeSet& getNodeInterestSet() const { return _nodeTypesOfInterest; }
    void addNodeTypeToInterestSet(NodeType_t nodeTypeToAdd);
    void addSetOfNodeTypesToNodeInterestSet(const NodeSet& setOfNodeTypes);
    void resetNodeInterestSet() { _nodeTypesOfInterest.clear(); }

    void setAssignmentServerSocket(const HifiSockAddr& serverSocket) { _assignmentServerSocket = serverSocket; }
    void sendAssignment(Assignment& assignment);
    
    void setIsShuttingDown(bool isShuttingDown) { _isShuttingDown = isShuttingDown; }

    void ignoreNodesInRadius(bool enabled = true);
    bool getIgnoreRadiusEnabled() const { return _ignoreRadiusEnabled.get(); }
    void toggleIgnoreRadius() { ignoreNodesInRadius(!getIgnoreRadiusEnabled()); }
    void enableIgnoreRadius() { ignoreNodesInRadius(true); }
    void disableIgnoreRadius() { ignoreNodesInRadius(false); }
    void ignoreNodeBySessionID(const QUuid& nodeID);
    void unignoreNodeBySessionID(const QUuid& nodeID);
    bool isIgnoringNode(const QUuid& nodeID) const;
    void personalMuteNodeBySessionID(const QUuid& nodeID, bool muteEnabled);

    void kickNodeBySessionID(const QUuid& nodeID);
    void muteNodeBySessionID(const QUuid& nodeID);
    void requestUsernameFromSessionID(const QUuid& nodeID);
    bool getRequestsDomainListData() { return _requestsDomainListData; }
    void setRequestsDomainListData(bool isRequesting);
    void requestPersonalMuteStatus(const QUuid& nodeID);

public slots:
    void reset();
    void sendDomainServerCheckIn();
    void handleDSPathQuery(const QString& newPath);

    void processDomainServerList(QSharedPointer<ReceivedMessage> message);
    void processDomainServerAddedNode(QSharedPointer<ReceivedMessage> message);
    void processDomainServerRemovedNode(QSharedPointer<ReceivedMessage> message);
    void processDomainServerPathResponse(QSharedPointer<ReceivedMessage> message);

    void processDomainServerConnectionTokenPacket(QSharedPointer<ReceivedMessage> message);
    
    void processPingPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);
    void processPingReplyPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

    void processICEPingPacket(QSharedPointer<ReceivedMessage> message);

    void processUsernameFromIDReply(QSharedPointer<ReceivedMessage> message);
    void processPersonalMuteStatusReply(QSharedPointer<ReceivedMessage> message);

#if (PR_BUILD || DEV_BUILD)
    void toggleSendNewerDSConnectVersion(bool shouldSendNewerVersion) { _shouldSendNewerVersion = shouldSendNewerVersion; }
#endif

signals:
    void limitOfSilentDomainCheckInsReached();
    void receivedDomainServerList();
    void ignoredNode(const QUuid& nodeID);
    void unignoredNode(const QUuid& nodeID);
    void ignoreRadiusEnabledChanged(bool isIgnored);
    void usernameFromIDReply(const QString& nodeID, const QString& username, const QString& machineFingerprint);
    void personalMuteStatusReply(const QString& nodeID, bool isPersonalMuted);

private slots:
    void stopKeepalivePingTimer();
    void sendPendingDSPathQuery();
    void handleICEConnectionToDomainServer();

    void startNodeHolePunch(const SharedNodePointer& node);
    void handleNodePingTimeout();

    void pingPunchForDomainServer();
    
    void sendKeepAlivePings();

    void maybeSendIgnoreSetToNode(SharedNodePointer node);
    
private:
    NodeList() : LimitedNodeList(INVALID_PORT, INVALID_PORT) { assert(false); } // Not implemented, needed for DependencyManager templates compile
    NodeList(char ownerType, int socketListenPort = INVALID_PORT, int dtlsListenPort = INVALID_PORT);
    NodeList(NodeList const&) = delete; // Don't implement, needed to avoid copies of singleton
    void operator=(NodeList const&) = delete; // Don't implement, needed to avoid copies of singleton

    void processDomainServerAuthRequest(const QByteArray& packet);
    void requestAuthForDomainServer();
    void activateSocketFromNodeCommunication(ReceivedMessage& message, const SharedNodePointer& sendingNode);
    void timePingReply(ReceivedMessage& message, const SharedNodePointer& sendingNode);

    void sendDSPathQuery(const QString& newPath);
 
    void parseNodeFromPacketStream(QDataStream& packetStream);

    void pingPunchForInactiveNode(const SharedNodePointer& node);

    bool sockAddrBelongsToDomainOrNode(const HifiSockAddr& sockAddr);

    std::atomic<NodeType_t> _ownerType;
    NodeSet _nodeTypesOfInterest;
    DomainHandler _domainHandler;
    int _numNoReplyDomainCheckIns;
    HifiSockAddr _assignmentServerSocket;
    bool _isShuttingDown { false };
    QTimer _keepAlivePingTimer;
    bool _requestsDomainListData;

    mutable QReadWriteLock _ignoredSetLock;
    tbb::concurrent_unordered_set<QUuid, UUIDHasher> _ignoredNodeIDs;

    void sendIgnoreRadiusStateToNode(const SharedNodePointer& destinationNode);
    Setting::Handle<bool> _ignoreRadiusEnabled { "IgnoreRadiusEnabled", true };

#if (PR_BUILD || DEV_BUILD)
    bool _shouldSendNewerVersion { false };
#endif
};

#endif // hifi_NodeList_h
