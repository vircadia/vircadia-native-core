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

#include <assert.h>
#include <stdint.h>
#include <iterator>
#include <memory>
#include <set>
#include <unordered_map>

#ifndef _WIN32
#include <unistd.h> // not on windows, not needed for mac or windows
#endif

#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QReadWriteLock>
#include <QtCore/QSet>
#include <QtCore/QSharedMemory>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QHostAddress>

#include <TBBHelpers.h>

#include <DependencyManager.h>
#include <SharedUtil.h>

#include "DomainHandler.h"
#include "Node.h"
#include "NLPacket.h"
#include "NLPacketList.h"
#include "PacketReceiver.h"
#include "ReceivedMessage.h"
#include "udt/ControlPacket.h"
#include "udt/PacketHeaders.h"
#include "udt/Socket.h"
#include "UUIDHasher.h"

const int INVALID_PORT = -1;

const quint64 NODE_SILENCE_THRESHOLD_MSECS = 5 * 1000;

extern const std::set<NodeType_t> SOLO_NODE_TYPES;

const char DEFAULT_ASSIGNMENT_SERVER_HOSTNAME[] = "localhost";

const char STUN_SERVER_HOSTNAME[] = "stun.highfidelity.io";
const unsigned short STUN_SERVER_PORT = 3478;

const QString DOMAIN_SERVER_LOCAL_PORT_SMEM_KEY = "domain-server.local-port";
const QString DOMAIN_SERVER_LOCAL_HTTP_PORT_SMEM_KEY = "domain-server.local-http-port";
const QString DOMAIN_SERVER_LOCAL_HTTPS_PORT_SMEM_KEY = "domain-server.local-https-port";

const QHostAddress DEFAULT_ASSIGNMENT_CLIENT_MONITOR_HOSTNAME = QHostAddress::LocalHost;

const QString USERNAME_UUID_REPLACEMENT_STATS_KEY = "$username";

const QString LOCAL_SOCKET_CHANGE_STAT = "LocalSocketChanges";

typedef std::pair<QUuid, SharedNodePointer> UUIDNodePair;
typedef tbb::concurrent_unordered_map<QUuid, SharedNodePointer, UUIDHasher> NodeHash;

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

    enum ConnectionStep {
        LookupAddress = 1,
        HandleAddress,
        SendSTUNRequest,
        SetPublicSocketFromSTUN,
        SetICEServerHostname,
        SetICEServerSocket,
        SendICEServerQuery,
        ReceiveDSPeerInformation,
        SendPingsToDS,
        SetDomainHostname,
        SetDomainSocket,
        SendDSCheckIn,
        ReceiveDSList,
        AddedAudioMixer,
        SendAudioPing,
        SetAudioMixerSocket,
        SendAudioPacket,
        ReceiveFirstAudioPacket
    };

    Q_ENUM(ConnectionStep);
    const QUuid& getSessionUUID() const { return _sessionUUID; }
    void setSessionUUID(const QUuid& sessionUUID);

    void setPermissions(const NodePermissions& newPermissions);
    bool isAllowedEditor() const { return _permissions.can(NodePermissions::Permission::canAdjustLocks); }
    bool getThisNodeCanRez() const { return _permissions.can(NodePermissions::Permission::canRezPermanentEntities); }
    bool getThisNodeCanRezTmp() const { return _permissions.can(NodePermissions::Permission::canRezTemporaryEntities); }
    bool getThisNodeCanWriteAssets() const { return _permissions.can(NodePermissions::Permission::canWriteToAssetServer); }
    bool getThisNodeCanKick() const { return _permissions.can(NodePermissions::Permission::canKick); }
    bool getThisNodeCanReplaceContent() const { return _permissions.can(NodePermissions::Permission::canReplaceDomainContent); }
    
    quint16 getSocketLocalPort() const { return _nodeSocket.localPort(); }
    Q_INVOKABLE void setSocketLocalPort(quint16 socketLocalPort);

    QUdpSocket& getDTLSSocket();

    PacketReceiver& getPacketReceiver() { return *_packetReceiver; }

    qint64 sendUnreliablePacket(const NLPacket& packet, const Node& destinationNode);
    qint64 sendUnreliablePacket(const NLPacket& packet, const HifiSockAddr& sockAddr,
                                const QUuid& connectionSecret = QUuid());

    qint64 sendPacket(std::unique_ptr<NLPacket> packet, const Node& destinationNode);
    qint64 sendPacket(std::unique_ptr<NLPacket> packet, const HifiSockAddr& sockAddr,
                      const QUuid& connectionSecret = QUuid());

    qint64 sendPacketList(NLPacketList& packetList, const Node& destinationNode);
    qint64 sendPacketList(NLPacketList& packetList, const HifiSockAddr& sockAddr,
                          const QUuid& connectionSecret = QUuid());
    qint64 sendPacketList(std::unique_ptr<NLPacketList> packetList, const HifiSockAddr& sockAddr);
    qint64 sendPacketList(std::unique_ptr<NLPacketList> packetList, const Node& destinationNode);

    std::function<void(Node*)> linkedDataCreateCallback;

    size_t size() const { QReadLocker readLock(&_nodeMutex); return _nodeHash.size(); }

    SharedNodePointer nodeWithUUID(const QUuid& nodeUUID);

    SharedNodePointer addOrUpdateNode(const QUuid& uuid, NodeType_t nodeType,
                                      const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket,
                                      bool isReplicated = false, bool isUpstream = false,
                                      const QUuid& connectionSecret = QUuid(),
                                      const NodePermissions& permissions = DEFAULT_AGENT_PERMISSIONS);

    static bool parseSTUNResponse(udt::BasePacket* packet, QHostAddress& newPublicAddress, uint16_t& newPublicPort);
    bool hasCompletedInitialSTUN() const { return _hasCompletedInitialSTUN; }

    const HifiSockAddr& getLocalSockAddr() const { return _localSockAddr; }
    const HifiSockAddr& getPublicSockAddr() const { return _publicSockAddr; }
    const HifiSockAddr& getSTUNSockAddr() const { return _stunSockAddr; }

    void processKillNode(ReceivedMessage& message);

    int updateNodeWithDataFromPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer matchingNode);
    NodeData* getOrCreateLinkedData(SharedNodePointer node);

    unsigned int broadcastToNodes(std::unique_ptr<NLPacket> packet, const NodeSet& destinationNodeTypes);
    SharedNodePointer soloNodeOfType(NodeType_t nodeType);

    void getPacketStats(float& packetsInPerSecond, float& bytesInPerSecond, float& packetsOutPerSecond, float& bytesOutPerSecond);
    void resetPacketStats();

    std::unique_ptr<NLPacket> constructPingPacket(PingType_t pingType = PingType::Agnostic);
    std::unique_ptr<NLPacket> constructPingReplyPacket(ReceivedMessage& message);

    static std::unique_ptr<NLPacket> constructICEPingPacket(PingType_t pingType, const QUuid& iceID);
    static std::unique_ptr<NLPacket> constructICEPingReplyPacket(ReceivedMessage& message, const QUuid& iceID);

    void sendPeerQueryToIceServer(const HifiSockAddr& iceServerSockAddr, const QUuid& clientID, const QUuid& peerID);

    SharedNodePointer findNodeWithAddr(const HifiSockAddr& addr);

    using value_type = SharedNodePointer;
    using const_iterator = std::vector<value_type>::const_iterator;

    // Cede control of iteration under a single read lock (e.g. for use by thread pools)
    // Use this for nested loops instead of taking nested read locks!
    //   This allows multiple threads (i.e. a thread pool) to share a lock
    //   without deadlocking when a dying node attempts to acquire a write lock
    template<typename NestedNodeLambda>
    void nestedEach(NestedNodeLambda functor, 
                    int* lockWaitOut = nullptr, 
                    int* nodeTransformOut = nullptr, 
                    int* functorOut = nullptr) {
        auto start = usecTimestampNow();
        {
            QReadLocker readLock(&_nodeMutex);
            auto endLock = usecTimestampNow();
            if (lockWaitOut) {
                *lockWaitOut = (endLock - start);
            }

            // Size of _nodeHash could change at any time,
            // so reserve enough memory for the current size
            // and then back insert all the nodes found
            std::vector<SharedNodePointer> nodes;
            nodes.reserve(_nodeHash.size());
            std::transform(_nodeHash.cbegin(), _nodeHash.cend(), std::back_inserter(nodes), [&](const NodeHash::value_type& it) {
                return it.second;
            });
            auto endTransform = usecTimestampNow();
            if (nodeTransformOut) {
                *nodeTransformOut = (endTransform - endLock);
            }

            functor(nodes.cbegin(), nodes.cend());
            auto endFunctor = usecTimestampNow();
            if (functorOut) {
                *functorOut = (endFunctor - endTransform);
            }
        }
    }

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

    // This is unsafe because it does not take a lock
    // Must only be called when you know that a read lock on the node mutex is held
    // and will be held for the duration of your iteration
    template<typename NodeLambda>
    void unsafeEachNode(NodeLambda functor) {
        for (NodeHash::const_iterator it = _nodeHash.cbegin(); it != _nodeHash.cend(); ++it) {
            functor(it->second);
        }
    }

    void putLocalPortIntoSharedMemory(const QString key, QObject* parent, quint16 localPort);
    bool getLocalServerPortFromSharedMemory(const QString key, quint16& localPort);

    const QMap<quint64, ConnectionStep> getLastConnectionTimes() const
        { QReadLocker readLock(&_connectionTimeLock); return _lastConnectionTimes; }
    void flagTimeForConnectionStep(ConnectionStep connectionStep);

    udt::Socket::StatsVector sampleStatsForAllConnections() { return _nodeSocket.sampleStatsForAllConnections(); }

    void setConnectionMaxBandwidth(int maxBandwidth) { _nodeSocket.setConnectionMaxBandwidth(maxBandwidth); }

    void setPacketFilterOperator(udt::PacketFilterOperator filterOperator) { _nodeSocket.setPacketFilterOperator(filterOperator); }
    bool packetVersionMatch(const udt::Packet& packet);

    bool isPacketVerifiedWithSource(const udt::Packet& packet, Node* sourceNode = nullptr);
    bool isPacketVerified(const udt::Packet& packet) { return isPacketVerifiedWithSource(packet); }

    static void makeSTUNRequestPacket(char* stunRequestPacket);

#if (PR_BUILD || DEV_BUILD)
    void sendFakedHandshakeRequestToNode(SharedNodePointer node);
#endif

public slots:
    void reset();
    void eraseAllNodes();

    void removeSilentNodes();

    void updateLocalSocket();

    void startSTUNPublicSocketUpdate();
    virtual void sendSTUNRequest();

    bool killNodeWithUUID(const QUuid& nodeUUID);

signals:
    void dataSent(quint8 channelType, int bytes);
    void dataReceived(quint8 channelType, int bytes);

    // QUuid might be zero for non-sourced packet types.
    void packetVersionMismatch(PacketType type, const HifiSockAddr& senderSockAddr, const QUuid& senderUUID);

    void uuidChanged(const QUuid& ownerUUID, const QUuid& oldUUID);
    void nodeAdded(SharedNodePointer);
    void nodeSocketUpdated(SharedNodePointer);
    void nodeKilled(SharedNodePointer);
    void nodeActivated(SharedNodePointer);

    void clientConnectionToNodeReset(SharedNodePointer);

    void localSockAddrChanged(const HifiSockAddr& localSockAddr);
    void publicSockAddrChanged(const HifiSockAddr& publicSockAddr);

    void isAllowedEditorChanged(bool isAllowedEditor);
    void canRezChanged(bool canRez);
    void canRezTmpChanged(bool canRezTmp);
    void canWriteAssetsChanged(bool canWriteAssets);
    void canKickChanged(bool canKick);
    void canReplaceContentChanged(bool canReplaceContent);

protected slots:
    void connectedForLocalSocketTest();
    void errorTestingLocalSocket();

    void clientConnectionToSockAddrReset(const HifiSockAddr& sockAddr);

protected:
    LimitedNodeList(int socketListenPort = INVALID_PORT, int dtlsListenPort = INVALID_PORT);
    LimitedNodeList(LimitedNodeList const&) = delete; // Don't implement, needed to avoid copies of singleton
    void operator=(LimitedNodeList const&) = delete; // Don't implement, needed to avoid copies of singleton

    qint64 sendPacket(std::unique_ptr<NLPacket> packet, const Node& destinationNode,
                      const HifiSockAddr& overridenSockAddr);
    qint64 writePacket(const NLPacket& packet, const HifiSockAddr& destinationSockAddr,
                       const QUuid& connectionSecret = QUuid());
    void collectPacketStats(const NLPacket& packet);
    void fillPacketHeader(const NLPacket& packet, const QUuid& connectionSecret = QUuid());

    void setLocalSocket(const HifiSockAddr& sockAddr);

    bool packetSourceAndHashMatchAndTrackBandwidth(const udt::Packet& packet, Node* sourceNode = nullptr);
    void processSTUNResponse(std::unique_ptr<udt::BasePacket> packet);

    void handleNodeKill(const SharedNodePointer& node);

    void stopInitialSTUNUpdate(bool success);

    void sendPacketToIceServer(PacketType packetType, const HifiSockAddr& iceServerSockAddr, const QUuid& clientID,
                               const QUuid& peerRequestID = QUuid());

    bool sockAddrBelongsToNode(const HifiSockAddr& sockAddr) { return findNodeWithAddr(sockAddr) != SharedNodePointer(); }

    QUuid _sessionUUID;
    NodeHash _nodeHash;
    mutable QReadWriteLock _nodeMutex;
    udt::Socket _nodeSocket;
    QUdpSocket* _dtlsSocket;
    HifiSockAddr _localSockAddr;
    HifiSockAddr _publicSockAddr;
    HifiSockAddr _stunSockAddr;
    bool _hasTCPCheckedLocalSocket { false };

    PacketReceiver* _packetReceiver;

    std::atomic<int> _numCollectedPackets;
    std::atomic<int> _numCollectedBytes;

    QElapsedTimer _packetStatTimer;
    NodePermissions _permissions;

    QPointer<QTimer> _initialSTUNTimer;

    int _numInitialSTUNRequests = 0;
    bool _hasCompletedInitialSTUN = false;
    quint64 _firstSTUNTime = 0;
    quint64 _publicSocketUpdateTime = 0;

    mutable QReadWriteLock _connectionTimeLock { };
    QMap<quint64, ConnectionStep> _lastConnectionTimes;
    bool _areConnectionTimesComplete = false;

    template<typename IteratorLambda>
    void eachNodeHashIterator(IteratorLambda functor) {
        QWriteLocker writeLock(&_nodeMutex);
        NodeHash::iterator it = _nodeHash.begin();

        while (it != _nodeHash.end()) {
            functor(it);
        }
    }


private slots:
    void flagTimeForConnectionStep(ConnectionStep connectionStep, quint64 timestamp);
    void possiblyTimeoutSTUNAddressLookup();
    void addSTUNHandlerToUnfiltered(); // called once STUN socket known
};

#endif // hifi_LimitedNodeList_h
