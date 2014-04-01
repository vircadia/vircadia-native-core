//
//  LimitedNodeList.h
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__LimitedNodeList__
#define __hifi__LimitedNodeList__

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

#include <gnutls/gnutls.h>

#include "DomainHandler.h"
#include "Node.h"

const quint64 NODE_SILENCE_THRESHOLD_USECS = 2 * 1000 * 1000;

extern const char SOLO_NODE_TYPES[2];

extern const QUrl DEFAULT_NODE_AUTH_URL;

const char DEFAULT_ASSIGNMENT_SERVER_HOSTNAME[] = "localhost";

class HifiSockAddr;

typedef QSet<NodeType_t> NodeSet;

typedef QSharedPointer<Node> SharedNodePointer;
typedef QHash<QUuid, SharedNodePointer> NodeHash;
Q_DECLARE_METATYPE(SharedNodePointer)

class LimitedNodeList : public QObject {
    Q_OBJECT
public:
    static LimitedNodeList* createInstance(unsigned short socketListenPort = 0, unsigned short dtlsPort = 0);
    static LimitedNodeList* getInstance();

    QUdpSocket& getNodeSocket() { return _nodeSocket; }
    QUdpSocket& getDTLSSocket();
    
    bool packetVersionAndHashMatch(const QByteArray& packet);
    
    qint64 writeDatagram(const QByteArray& datagram, const SharedNodePointer& destinationNode,
                         const HifiSockAddr& overridenSockAddr = HifiSockAddr());
    qint64 writeUnverifiedDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr);
    qint64 writeDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                         const HifiSockAddr& overridenSockAddr = HifiSockAddr());

    void(*linkedDataCreateCallback)(Node *);

    NodeHash getNodeHash();
    int size() const { return _nodeHash.size(); }

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
public slots:
    void reset();
    void eraseAllNodes();
    
    void removeSilentNodes();
    
    void killNodeWithUUID(const QUuid& nodeUUID);
signals:
    void nodeAdded(SharedNodePointer);
    void nodeKilled(SharedNodePointer);
protected:
    static LimitedNodeList* _sharedInstance;

    LimitedNodeList(unsigned short socketListenPort, unsigned short dtlsListenPort);
    LimitedNodeList(LimitedNodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(LimitedNodeList const&); // Don't implement, needed to avoid copies of singleton
    
    qint64 writeDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr,
                         const QUuid& connectionSecret);

    NodeHash::iterator killNodeAtHashIterator(NodeHash::iterator& nodeItemToKill);

    
    void changeSendSocketBufferSize(int numSendBytes);

    NodeHash _nodeHash;
    QMutex _nodeHashMutex;
    QUdpSocket _nodeSocket;
    QUdpSocket* _dtlsSocket;
    int _numCollectedPackets;
    int _numCollectedBytes;
    QElapsedTimer _packetStatTimer;
};

#endif /* defined(__hifi__LimitedNodeList__) */
