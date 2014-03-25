//
//  NodeList.h
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__NodeList__
#define __hifi__NodeList__

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <netinet/in.h>
#endif
#include <stdint.h>
#include <iterator>

#ifndef _WIN32
#include <unistd.h> // not on windows, not needed for mac or windows
#endif

#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "DomainInfo.h"
#include "Node.h"

const quint64 NODE_SILENCE_THRESHOLD_USECS = 2 * 1000 * 1000;
const quint64 DOMAIN_SERVER_CHECK_IN_USECS = 1 * 1000000;
const quint64 PING_INACTIVE_NODE_INTERVAL_USECS = 1 * 1000 * 1000;

extern const char SOLO_NODE_TYPES[2];

extern const QUrl DEFAULT_NODE_AUTH_URL;

const char DEFAULT_ASSIGNMENT_SERVER_HOSTNAME[] = "localhost";

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

class Assignment;
class HifiSockAddr;

typedef QSet<NodeType_t> NodeSet;

typedef QSharedPointer<Node> SharedNodePointer;
typedef QHash<QUuid, SharedNodePointer> NodeHash;
Q_DECLARE_METATYPE(SharedNodePointer)

typedef quint8 PingType_t;
namespace PingType {
    const PingType_t Agnostic = 0;
    const PingType_t Local = 1;
    const PingType_t Public = 2;
}

class NodeList : public QObject {
    Q_OBJECT
public:
    static NodeList* createInstance(char ownerType, unsigned short int socketListenPort = 0);
    static NodeList* getInstance();
    NodeType_t getOwnerType() const { return _ownerType; }
    void setOwnerType(NodeType_t ownerType) { _ownerType = ownerType; }

    const QUuid& getSessionUUID() const { return _sessionUUID; }
    void setSessionUUID(const QUuid& sessionUUID);

    QUdpSocket& getNodeSocket() { return _nodeSocket; }
    
    bool packetVersionAndHashMatch(const QByteArray& packet);
    
    qint64 writeDatagram(const QByteArray& datagram, const SharedNodePointer& destinationNode,
                         const HifiSockAddr& overridenSockAddr = HifiSockAddr());
    qint64 writeDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                         const HifiSockAddr& overridenSockAddr = HifiSockAddr());
    qint64 sendStatsToDomainServer(const QJsonObject& statsObject);

    void(*linkedDataCreateCallback)(Node *);

    NodeHash getNodeHash();
    int size() const { return _nodeHash.size(); }

    int getNumNoReplyDomainCheckIns() const { return _numNoReplyDomainCheckIns; }
    DomainInfo& getDomainInfo() { return _domainInfo; }
    
    const NodeSet& getNodeInterestSet() const { return _nodeTypesOfInterest; }
    void addNodeTypeToInterestSet(NodeType_t nodeTypeToAdd);
    void addSetOfNodeTypesToNodeInterestSet(const NodeSet& setOfNodeTypes);
    void resetNodeInterestSet() { _nodeTypesOfInterest.clear(); }

    int processDomainServerList(const QByteArray& packet);

    void setAssignmentServerSocket(const HifiSockAddr& serverSocket) { _assignmentServerSocket = serverSocket; }
    void sendAssignment(Assignment& assignment);

    QByteArray constructPingPacket(PingType_t pingType = PingType::Agnostic);
    QByteArray constructPingReplyPacket(const QByteArray& pingPacket);
    void pingPublicAndLocalSocketsForInactiveNode(const SharedNodePointer& node);

    /// passing false for blockingLock, will tryLock, and may return NULL when a node with the UUID actually does exist
    SharedNodePointer nodeWithUUID(const QUuid& nodeUUID, bool blockingLock = true);
    SharedNodePointer sendingNodeForPacket(const QByteArray& packet);
    
    SharedNodePointer addOrUpdateNode(const QUuid& uuid, char nodeType,
                                      const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);
    SharedNodePointer updateSocketsForNode(const QUuid& uuid,
                                           const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);

    void processNodeData(const HifiSockAddr& senderSockAddr, const QByteArray& packet);
    void processKillNode(const QByteArray& datagram);

    int updateNodeWithDataFromPacket(const SharedNodePointer& matchingNode, const QByteArray& packet);
    int findNodeAndUpdateWithDataFromPacket(const QByteArray& packet);

    unsigned broadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes);
    SharedNodePointer soloNodeOfType(char nodeType);

    void getPacketStats(float &packetsPerSecond, float &bytesPerSecond);
    void resetPacketStats();
    
    void loadData(QSettings* settings);
    void saveData(QSettings* settings);
public slots:
    void reset();
    
    void sendDomainServerCheckIn();
    void pingInactiveNodes();
    void removeSilentNodes();
    
    void killNodeWithUUID(const QUuid& nodeUUID);
signals:
    void uuidChanged(const QUuid& ownerUUID);
    void nodeAdded(SharedNodePointer);
    void nodeKilled(SharedNodePointer);
    void limitOfSilentDomainCheckInsReached();
private slots:
    void domainServerAuthReply(const QJsonObject& jsonObject);
private:
    static NodeList* _sharedInstance;

    NodeList(char ownerType, unsigned short int socketListenPort);
    NodeList(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void sendSTUNRequest();
    void processSTUNResponse(const QByteArray& packet);
    
    qint64 writeDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr,
                         const QUuid& connectionSecret);

    NodeHash::iterator killNodeAtHashIterator(NodeHash::iterator& nodeItemToKill);
    
    void clear();

    void processDomainServerAuthRequest(const QByteArray& packet);
    void requestAuthForDomainServer();
    void activateSocketFromNodeCommunication(const QByteArray& packet, const SharedNodePointer& sendingNode);
    void timePingReply(const QByteArray& packet, const SharedNodePointer& sendingNode);

    NodeHash _nodeHash;
    QMutex _nodeHashMutex;
    QUdpSocket _nodeSocket;
    NodeType_t _ownerType;
    NodeSet _nodeTypesOfInterest;
    DomainInfo _domainInfo;
    QUuid _sessionUUID;
    int _numNoReplyDomainCheckIns;
    HifiSockAddr _assignmentServerSocket;
    HifiSockAddr _publicSockAddr;
    bool _hasCompletedInitialSTUNFailure;
    unsigned int _stunRequestsSinceSuccess;
    int _numCollectedPackets;
    int _numCollectedBytes;
    QElapsedTimer _packetStatTimer;
};

#endif /* defined(__hifi__NodeList__) */
