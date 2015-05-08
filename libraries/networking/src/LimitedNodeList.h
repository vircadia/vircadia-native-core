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
#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qhostaddress.h>
#include <QSharedMemory>

#include <tbb/concurrent_unordered_map.h>

#include <DependencyManager.h>

#include "DomainHandler.h"
#include "Node.h"
#include "UUIDHasher.h"

const int MAX_PACKET_SIZE = 1450;

const quint64 NODE_SILENCE_THRESHOLD_MSECS = 2 * 1000;

extern const char SOLO_NODE_TYPES[2];

const char DEFAULT_ASSIGNMENT_SERVER_HOSTNAME[] = "localhost";

const char STUN_SERVER_HOSTNAME[] = "stun.highfidelity.io";
const unsigned short STUN_SERVER_PORT = 3478;

const QString DOMAIN_SERVER_LOCAL_PORT_SMEM_KEY = "domain-server.local-port";
const QString DOMAIN_SERVER_LOCAL_HTTP_PORT_SMEM_KEY = "domain-server.local-http-port";
const QString DOMAIN_SERVER_LOCAL_HTTPS_PORT_SMEM_KEY = "domain-server.local-https-port";
const QString ASSIGNMENT_CLIENT_MONITOR_LOCAL_PORT_SMEM_KEY = "assignment-client-monitor.local-port";

const char DEFAULT_ASSIGNMENT_CLIENT_MONITOR_HOSTNAME[] = "localhost";
const unsigned short DEFAULT_ASSIGNMENT_CLIENT_MONITOR_PORT = 40104;

const QString USERNAME_UUID_REPLACEMENT_STATS_KEY = "$username";

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

class LimitedNodeList : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    const QUuid& getSessionUUID() const { return _sessionUUID; }
    void setSessionUUID(const QUuid& sessionUUID);

    bool getThisNodeCanAdjustLocks() const { return _thisNodeCanAdjustLocks; }
    void setThisNodeCanAdjustLocks(bool canAdjustLocks);

    bool getThisNodeCanRez() const { return _thisNodeCanRez; }
    void setThisNodeCanRez(bool canRez);
    
    void rebindNodeSocket();
    QUdpSocket& getNodeSocket() { return _nodeSocket; }
    QUdpSocket& getDTLSSocket();
    
    bool packetVersionAndHashMatch(const QByteArray& packet);

    qint64 readDatagram(QByteArray& incomingPacket, QHostAddress* address, quint16 * port);
    
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
                                      const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket,
                                      bool canAdjustLocks, bool canRez);
    
    const HifiSockAddr& getLocalSockAddr() const { return _localSockAddr; }
    const HifiSockAddr& getSTUNSockAddr() const { return _stunSockAddr; }

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
            functor(it->second);
        }
    }

    template<typename PredLambda, typename NodeLambda>
    void eachMatchingNode(PredLambda predicate, NodeLambda functor) {
        QReadLocker readLock(&_nodeMutex);

        for (NodeHash::const_iterator it = _nodeHash.cbegin(); it != _nodeHash.cend(); ++it) {
            if (predicate(it->second)) {
                functor(it->second);
            }
        }
    }

    template<typename BreakableNodeLambda>
    void eachNodeBreakable(BreakableNodeLambda functor) {
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

    void putLocalPortIntoSharedMemory(const QString key, QObject* parent, quint16 localPort);
    bool getLocalServerPortFromSharedMemory(const QString key, QSharedMemory*& sharedMem, quint16& localPort);
    
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

    void canAdjustLocksChanged(bool canAdjustLocks);
    void canRezChanged(bool canRez);

    void dataSent(const quint8 channel_type, const int bytes);
    void dataReceived(const quint8 channel_type, const int bytes);

    void packetVersionMismatch();
    
protected:
    LimitedNodeList(unsigned short socketListenPort = 0, unsigned short dtlsListenPort = 0);
    LimitedNodeList(LimitedNodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(LimitedNodeList const&); // Don't implement, needed to avoid copies of singleton
    
    qint64 writeDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr,
                         const QUuid& connectionSecret);
    
    void changeSocketBufferSizes(int numBytes);
    
    void handleNodeKill(const SharedNodePointer& node);

    QUuid _sessionUUID;
    NodeHash _nodeHash;
    QReadWriteLock _nodeMutex;
    QUdpSocket _nodeSocket;
    QUdpSocket* _dtlsSocket;
    HifiSockAddr _localSockAddr;
    HifiSockAddr _publicSockAddr;
    HifiSockAddr _stunSockAddr;

    // XXX can BandwidthRecorder be used for this?
    int _numCollectedPackets;
    int _numCollectedBytes;

    QElapsedTimer _packetStatTimer;
    bool _thisNodeCanAdjustLocks;
    bool _thisNodeCanRez;
    
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
