//
//  NodeList.h
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__NodeList__
#define __hifi__NodeList__

#include <netinet/in.h>
#include <stdint.h>
#include <iterator>
#include <unistd.h>

#include <QtCore/QSettings>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "Node.h"
#include "NodeTypes.h"

const int MAX_NUM_NODES = 10000;
const int NODES_PER_BUCKET = 100;

const int MAX_PACKET_SIZE = 1500;

const uint64_t NODE_SILENCE_THRESHOLD_USECS = 2 * 1000 * 1000;
const uint64_t DOMAIN_SERVER_CHECK_IN_USECS = 1 * 1000000;
const uint64_t PING_INACTIVE_NODE_INTERVAL_USECS = 1 * 1000 * 1000;

extern const char SOLO_NODE_TYPES[2];

const int MAX_HOSTNAME_BYTES = 256;

extern const QString DEFAULT_DOMAIN_HOSTNAME;
extern const unsigned short DEFAULT_DOMAIN_SERVER_PORT;

const char DEFAULT_ASSIGNMENT_SERVER_HOSTNAME[] = "localhost";

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

class Assignment;
class HifiSockAddr;

typedef QSharedPointer<Node> SharedNodePointer;
typedef QHash<QUuid, SharedNodePointer> NodeHash;
Q_DECLARE_METATYPE(SharedNodePointer)

class NodeList : public QObject {
    Q_OBJECT
public:
    static NodeList* createInstance(char ownerType, unsigned short int socketListenPort = 0);
    static NodeList* getInstance();
    
    NODE_TYPE getOwnerType() const { return _ownerType; }
    void setOwnerType(NODE_TYPE ownerType) { _ownerType = ownerType; }

    const QString& getDomainHostname() const { return _domainHostname; }
    void setDomainHostname(const QString& domainHostname);
    
    const QHostAddress& getDomainIP() const { return _domainSockAddr.getAddress(); }
    void setDomainIPToLocalhost() { _domainSockAddr.setAddress(QHostAddress(INADDR_LOOPBACK)); }
    
    void setDomainSockAddr(const HifiSockAddr& domainSockAddr) { _domainSockAddr = domainSockAddr; }
    
    unsigned short getDomainPort() const { return _domainSockAddr.getPort(); }
    
    const QUuid& getOwnerUUID() const { return _ownerUUID; }
    void setOwnerUUID(const QUuid& ownerUUID) { _ownerUUID = ownerUUID; }
    
    QUdpSocket& getNodeSocket() { return _nodeSocket; }
    
    void(*linkedDataCreateCallback)(Node *);
    
    const NodeHash& getNodeHash() { return _nodeHash; }
    
    int size() const { return _nodeHash.size(); }
    
    int getNumNoReplyDomainCheckIns() const { return _numNoReplyDomainCheckIns; }
    
    void clear();
    void reset();
    
    void setNodeTypesOfInterest(const char* nodeTypesOfInterest, int numNodeTypesOfInterest);
    
    int processDomainServerList(unsigned char *packetData, size_t dataBytes);
    
    void setAssignmentServerSocket(const HifiSockAddr& serverSocket) { _assignmentServerSocket = serverSocket; }
    void sendAssignment(Assignment& assignment);
    
    int fillPingPacket(unsigned char* buffer);
    int fillPingReplyPacket(unsigned char* pingBuffer, unsigned char* replyBuffer);
    void pingPublicAndLocalSocketsForInactiveNode(Node* node);
    
    void killNodeWithUUID(const QUuid& nodeUUID);
    void sendKillNode(const char* nodeTypes, int numNodeTypes);
    
    SharedNodePointer nodeWithAddress(const HifiSockAddr& senderSockAddr);
    SharedNodePointer nodeWithUUID(const QUuid& nodeUUID);
    
    SharedNodePointer addOrUpdateNode(const QUuid& uuid, char nodeType, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);
    
    void processNodeData(const HifiSockAddr& senderSockAddr, unsigned char *packetData, size_t dataBytes);
    void processBulkNodeData(const HifiSockAddr& senderSockAddr, unsigned char *packetData, int numTotalBytes);
   
    int updateNodeWithData(Node *node, const HifiSockAddr& senderSockAddr, unsigned char *packetData, int dataBytes);
    
    unsigned broadcastToNodes(unsigned char *broadcastData, size_t dataBytes, const char* nodeTypes, int numNodeTypes);
    
    SharedNodePointer soloNodeOfType(char nodeType);
    
    void loadData(QSettings* settings);
    void saveData(QSettings* settings);
    
    const HifiSockAddr* getNodeActiveSocketOrPing(Node* node);
public slots:
    void sendDomainServerCheckIn();
    void pingInactiveNodes();
    void removeSilentNodes();
signals:
    void domainChanged(const QString& domainHostname);
    void nodeAdded(SharedNodePointer);
    void nodeKilled(SharedNodePointer);
private:
    static NodeList* _sharedInstance;
    
    NodeList(char ownerType, unsigned short int socketListenPort);
    ~NodeList();
    NodeList(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(NodeList const&); // Don't implement, needed to avoid copies of singleton
    
    void sendSTUNRequest();
    void processSTUNResponse(unsigned char* packetData, size_t dataBytes);
    
    void processKillNode(unsigned char* packetData, size_t dataBytes);
    
    NodeHash::iterator killNodeAtHashIterator(NodeHash::iterator& nodeItemToKill);
    
    NodeHash _nodeHash;
    QString _domainHostname;
    HifiSockAddr _domainSockAddr;
    QUdpSocket _nodeSocket;
    char _ownerType;
    char* _nodeTypesOfInterest;
    QUuid _ownerUUID;
    int _numNoReplyDomainCheckIns;
    HifiSockAddr _assignmentServerSocket;
    HifiSockAddr _publicSockAddr;
    bool _hasCompletedInitialSTUNFailure;
    unsigned int _stunRequestsSinceSuccess;
    
    void activateSocketFromNodeCommunication(const HifiSockAddr& nodeSockAddr);
    void timePingReply(const HifiSockAddr& nodeAddress, unsigned char *packetData);
    
    void resetDomainData(char domainField[], const char* domainData);
    void domainLookup();
};

#endif /* defined(__hifi__NodeList__) */
