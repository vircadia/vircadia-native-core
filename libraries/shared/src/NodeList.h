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

#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtCore/QSettings>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "Node.h"

const int MAX_PACKET_SIZE = 1500;

const quint64 NODE_SILENCE_THRESHOLD_USECS = 2 * 1000 * 1000;
const quint64 DOMAIN_SERVER_CHECK_IN_USECS = 1 * 1000000;
const quint64 PING_INACTIVE_NODE_INTERVAL_USECS = 1 * 1000 * 1000;

extern const char SOLO_NODE_TYPES[2];

extern const QString DEFAULT_DOMAIN_HOSTNAME;
extern const unsigned short DEFAULT_DOMAIN_SERVER_PORT;

const char DEFAULT_ASSIGNMENT_SERVER_HOSTNAME[] = "localhost";

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

class Assignment;
class HifiSockAddr;

typedef QSet<NodeType_t> NodeSet;

typedef QSharedPointer<Node> SharedNodePointer;
typedef QHash<QUuid, SharedNodePointer> NodeHash;
Q_DECLARE_METATYPE(SharedNodePointer)

class NodeList : public QObject {
    Q_OBJECT
public:
    static NodeList* createInstance(char ownerType, unsigned short int socketListenPort = 0);
    static NodeList* getInstance();
    NodeType_t getOwnerType() const { return _ownerType; }
    void setOwnerType(NodeType_t ownerType) { _ownerType = ownerType; }

    const QString& getDomainHostname() const { return _domainHostname; }
    void setDomainHostname(const QString& domainHostname);

    const QHostAddress& getDomainIP() const { return _domainSockAddr.getAddress(); }
    void setDomainIPToLocalhost() { _domainSockAddr.setAddress(QHostAddress(INADDR_LOOPBACK)); }

    void setDomainSockAddr(const HifiSockAddr& domainSockAddr) { _domainSockAddr = domainSockAddr; }

    unsigned short getDomainPort() const { return _domainSockAddr.getPort(); }

    const QUuid& getSessionUUID() const { return _sessionUUID; }
    void setSessionUUID(const QUuid& sessionUUID);

    QUdpSocket& getNodeSocket() { return _nodeSocket; }

    void(*linkedDataCreateCallback)(Node *);

    NodeHash getNodeHash();
    int size() const { return _nodeHash.size(); }

    int getNumNoReplyDomainCheckIns() const { return _numNoReplyDomainCheckIns; }

    void reset();
    
    const NodeSet& getNodeInterestSet() const { return _nodeTypesOfInterest; }
    void addNodeTypeToInterestSet(NodeType_t nodeTypeToAdd);
    void addSetOfNodeTypesToNodeInterestSet(const NodeSet& setOfNodeTypes);

    int processDomainServerList(const QByteArray& packet);

    void setAssignmentServerSocket(const HifiSockAddr& serverSocket) { _assignmentServerSocket = serverSocket; }
    void sendAssignment(Assignment& assignment);

    QByteArray constructPingPacket();
    QByteArray constructPingReplyPacket(const QByteArray& pingPacket);
    void pingPublicAndLocalSocketsForInactiveNode(Node* node);

    SharedNodePointer nodeWithAddress(const HifiSockAddr& senderSockAddr);
    SharedNodePointer nodeWithUUID(const QUuid& nodeUUID);

    SharedNodePointer addOrUpdateNode(const QUuid& uuid, char nodeType, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);

    void processNodeData(const HifiSockAddr& senderSockAddr, const QByteArray& packet);
    void processKillNode(const QByteArray& datagram);

    int updateNodeWithData(Node *node, const HifiSockAddr& senderSockAddr, const QByteArray& packet);

    unsigned broadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes);
    SharedNodePointer soloNodeOfType(char nodeType);

    void loadData(QSettings* settings);
    void saveData(QSettings* settings);

    const HifiSockAddr* getNodeActiveSocketOrPing(Node* node);
public slots:
    void sendDomainServerCheckIn();
    void pingInactiveNodes();
    void removeSilentNodes();
    
    void killNodeWithUUID(const QUuid& nodeUUID);
signals:
    void domainChanged(const QString& domainHostname);
    void uuidChanged(const QUuid& ownerUUID);
    void nodeAdded(SharedNodePointer);
    void nodeKilled(SharedNodePointer);
private:
    static NodeList* _sharedInstance;

    NodeList(char ownerType, unsigned short int socketListenPort);
    ~NodeList();
    NodeList(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void sendSTUNRequest();
    void processSTUNResponse(const QByteArray& packet);

    NodeHash::iterator killNodeAtHashIterator(NodeHash::iterator& nodeItemToKill);

    NodeHash _nodeHash;
    QMutex _nodeHashMutex;
    QString _domainHostname;
    HifiSockAddr _domainSockAddr;
    QUdpSocket _nodeSocket;
    NodeType_t _ownerType;
    NodeSet _nodeTypesOfInterest;
    QUuid _sessionUUID;
    int _numNoReplyDomainCheckIns;
    HifiSockAddr _assignmentServerSocket;
    HifiSockAddr _publicSockAddr;
    bool _hasCompletedInitialSTUNFailure;
    unsigned int _stunRequestsSinceSuccess;

    void activateSocketFromNodeCommunication(const HifiSockAddr& nodeSockAddr);
    void timePingReply(const QByteArray& packet);
    void resetDomainData(char domainField[], const char* domainData);
    void domainLookup();
    void clear();
};

#endif /* defined(__hifi__NodeList__) */
