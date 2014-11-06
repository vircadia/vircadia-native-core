//
//  LimitedNodeList.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LimitedNodeList_h
#define hifi_LimitedNodeList_h

#include <stdint.h>
#include <iterator>
#include <memory>

#ifndef _WIN32
#include <unistd.h> // not on windows, not needed for mac or windows
#endif

#include <qelapsedtimer.h>
#include <qreadwritelock.h>
#include <qset.h>
#include <qsharedpointer.h>
#include <qhostaddress.h>
#include <qudpsocket.h>

#include <tbb/concurrent_unordered_map.h>

#include "DomainHandler.h"
#include "Node.h"
#include "UUIDHasher.h"

const int MAX_PACKET_SIZE = 1500;

const quint64 NODE_SILENCE_THRESHOLD_MSECS = 2 * 1000;

extern const char SOLO_NODE_TYPES[2];

extern const QUrl DEFAULT_NODE_AUTH_URL;

const char DEFAULT_ASSIGNMENT_SERVER_HOSTNAME[] = "localhost";

const char STUN_SERVER_HOSTNAME[] = "stun.highfidelity.io";
const unsigned short STUN_SERVER_PORT = 3478;

class HifiSockAddr;

typedef QSet<NodeType_t> NodeSet;

typedef QSharedPointer<Node> SharedNodePointer;
Q_DECLARE_METATYPE(SharedNodePointer)

using namespace tbb;
typedef std::pair<QUuid, SharedNodePointer> UUIDNodePair;
typedef concurrent_unordered_map<QUuid, SharedNodePointer, UUIDHasher> NodeHash;

typedef quint8 PingType_t;
namespace PingType {
    const PingType_t Agnostic = 0;
    const PingType_t Local = 1;
    const PingType_t Public = 2;
    const PingType_t Symmetric = 3;
}

class LimitedNodeList : public QObject {
    Q_OBJECT
public:
    static LimitedNodeList* createInstance(unsigned short socketListenPort = 0, unsigned short dtlsPort = 0);
    static LimitedNodeList* getInstance();

    const QUuid& getSessionUUID() const { return _sessionUUID; }
    void setSessionUUID(const QUuid& sessionUUID);
    
    void rebindNodeSocket();
    QUdpSocket& getNodeSocket() { return _nodeSocket; }
    QUdpSocket& getDTLSSocket();
    
    bool packetVersionAndHashMatch(const QByteArray& packet);
    
    qint64 writeDatagram(const QByteArray& datagram, const SharedNodePointer& destinationNode,
                         const HifiSockAddr& overridenSockAddr = HifiSockAddr());

    qint64 writeUnverifiedDatagram(const QByteArray& datagram, const SharedNodePointer& destinationNode,
                               const HifiSockAddr& overridenSockAddr = HifiSockAddr());

    qint64 writeUnverifiedDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr);
    qint64 writeDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                         const HifiSockAddr& overridenSockAddr = HifiSockAddr());

    qint64 writeUnverifiedDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                         const HifiSockAddr& overridenSockAddr = HifiSockAddr());

    void(*linkedDataCreateCallback)(Node *);
    
    int size() const { return _nodeHash.size(); }

    SharedNodePointer nodeWithUUID(const QUuid& nodeUUID);
    SharedNodePointer sendingNodeForPacket(const QByteArray& packet);
    
    SharedNodePointer addOrUpdateNode(const QUuid& uuid, NodeType_t nodeType,
                                      const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);
    
    const HifiSockAddr& getLocalSockAddr() const { return _localSockAddr; }

    void processNodeData(const HifiSockAddr& senderSockAddr, const QByteArray& packet);
    void processKillNode(const QByteArray& datagram);

    int updateNodeWithDataFromPacket(const SharedNodePointer& matchingNode, const QByteArray& packet);
    int findNodeAndUpdateWithDataFromPacket(const QByteArray& packet);

    unsigned broadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes);
    SharedNodePointer soloNodeOfType(char nodeType);

    void getPacketStats(float &packetsPerSecond, float &bytesPerSecond);
    void resetPacketStats();
    
    QByteArray constructPingPacket(PingType_t pingType = PingType::Agnostic, bool isVerified = true,
                                   const QUuid& packetHeaderID = QUuid());
    QByteArray constructPingReplyPacket(const QByteArray& pingPacket, const QUuid& packetHeaderID = QUuid());
    
    virtual void sendSTUNRequest();
    virtual bool processSTUNResponse(const QByteArray& packet);
    
    void sendHeartbeatToIceServer(const HifiSockAddr& iceServerSockAddr,
                                  QUuid headerID = QUuid(), const QUuid& connectRequestID = QUuid());
    
    template<typename NodeLambda>
    void eachNode(NodeLambda functor) {
        QReadLocker readLock(&_nodeMutex);
        
        for (NodeHash::const_iterator it = _nodeHash.cbegin(); it != _nodeHash.cend(); ++it) {
            if (!functor(it->second)) {
                break;
            }
        }
    }
    
    template<typename PredLambda>
    SharedNodePointer nodeMatchingPredicate(const PredLambda predicate) {
        QReadLocker readLock(&_nodeMutex);
        
        for (NodeHash::const_iterator it = _nodeHash.cbegin(); it != _nodeHash.cend(); ++it) {
            if (predicate(it->second)) {
                return it->second;
            }
        }
        
        return SharedNodePointer();
    }
    
public slots:
    void reset();
    void eraseAllNodes();
    
    void removeSilentNodes();
    
    void updateLocalSockAddr();
    
    void killNodeWithUUID(const QUuid& nodeUUID);
signals:
    void uuidChanged(const QUuid& ownerUUID, const QUuid& oldUUID);
    void nodeAdded(SharedNodePointer);
    void nodeKilled(SharedNodePointer);
    
    void localSockAddrChanged(const HifiSockAddr& localSockAddr);
    void publicSockAddrChanged(const HifiSockAddr& publicSockAddr);
protected:
    static std::auto_ptr<LimitedNodeList> _sharedInstance;

    LimitedNodeList(unsigned short socketListenPort, unsigned short dtlsListenPort);
    LimitedNodeList(LimitedNodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(LimitedNodeList const&); // Don't implement, needed to avoid copies of singleton
    
    qint64 writeDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr,
                         const QUuid& connectionSecret);
    
    void changeSocketBufferSizes(int numBytes);

    QUuid _sessionUUID;
    NodeHash _nodeHash;
    QReadWriteLock _nodeMutex;
    QUdpSocket _nodeSocket;
    QUdpSocket* _dtlsSocket;
    HifiSockAddr _localSockAddr;
    HifiSockAddr _publicSockAddr;
    HifiSockAddr _stunSockAddr;
    int _numCollectedPackets;
    int _numCollectedBytes;
    QElapsedTimer _packetStatTimer;
    
    template<typename IteratorLambda>
    void eachNodeHashIterator(IteratorLambda functor) {
        QWriteLocker writeLock(&_nodeMutex);
        NodeHash::iterator it = _nodeHash.begin();
        
        while (it != _nodeHash.end()) {
            functor(it);
        }
    }
    
};

#endif // hifi_LimitedNodeList_h
