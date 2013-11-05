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

#include <QtNetwork/QHostAddress>
#include <QtCore/QSettings>

#include "Node.h"
#include "NodeTypes.h"
#include "UDPSocket.h"

#ifdef _WIN32
#include "pthread.h"
#endif

const int MAX_NUM_NODES = 10000;
const int NODES_PER_BUCKET = 100;

const int MAX_PACKET_SIZE = 1500;

const uint64_t NODE_SILENCE_THRESHOLD_USECS = 2 * 1000 * 1000;
const int DOMAIN_SERVER_CHECK_IN_USECS = 1 * 1000000;

extern const char SOLO_NODE_TYPES[2];

const int MAX_HOSTNAME_BYTES = 256;

extern const QString DEFAULT_DOMAIN_HOSTNAME;
extern const unsigned short DEFAULT_DOMAIN_SERVER_PORT;

const char LOCAL_ASSIGNMENT_SERVER_HOSTNAME[] = "localhost";

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

class Assignment;
class NodeListIterator;

// Callers who want to hook add/kill callbacks should implement this class
class NodeListHook {
public:
    virtual void nodeAdded(Node* node) = 0;
    virtual void nodeKilled(Node* node) = 0;
};

class DomainChangeListener {
public:
    virtual void domainChanged(QString domain) = 0;
};

class NodeList {
public:
    static NodeList* createInstance(char ownerType, unsigned short int socketListenPort = 0);
    static NodeList* getInstance();
    
    typedef NodeListIterator iterator;
  
    NodeListIterator begin() const;
    NodeListIterator end() const;
    
    NODE_TYPE getOwnerType() const { return _ownerType; }
    void setOwnerType(NODE_TYPE ownerType) { _ownerType = ownerType; }

    const QString& getDomainHostname() const { return _domainHostname; }
    void setDomainHostname(const QString& domainHostname);
    
    const QHostAddress& getDomainIP() const { return _domainIP; }
    void setDomainIP(const QHostAddress& domainIP) { _domainIP = domainIP; }
    void setDomainIPToLocalhost() { _domainIP = QHostAddress(INADDR_LOOPBACK); }
    
    unsigned short getDomainPort() const { return _domainPort; }
    void setDomainPort(unsigned short domainPort) { _domainPort = domainPort; }
    
    const QUuid& getOwnerUUID() const { return _ownerUUID; }
    void setOwnerUUID(const QUuid& ownerUUID) { _ownerUUID = ownerUUID; }
    
    UDPSocket* getNodeSocket() { return &_nodeSocket; }
    
    unsigned short int getSocketListenPort() const { return _nodeSocket.getListeningPort(); }
    
    void(*linkedDataCreateCallback)(Node *);
    
    int size() { return _numNodes; }
    int getNumAliveNodes() const;
    
    int getNumNoReplyDomainCheckIns() const { return _numNoReplyDomainCheckIns; }
    
    void clear();
    void reset();
    
    void setNodeTypesOfInterest(const char* nodeTypesOfInterest, int numNodeTypesOfInterest);
    
    void sendDomainServerCheckIn();
    int processDomainServerList(unsigned char *packetData, size_t dataBytes);
    
    void setAssignmentServerSocket(sockaddr* serverSocket) { _assignmentServerSocket = serverSocket; }
    void sendAssignment(Assignment& assignment);
    
    void pingPublicAndLocalSocketsForInactiveNode(Node* node) const;
    
    Node* nodeWithAddress(sockaddr *senderAddress);
    Node* nodeWithUUID(const QUuid& nodeUUID);
    
    Node* addOrUpdateNode(const QUuid& uuid, char nodeType, sockaddr* publicSocket, sockaddr* localSocket);
    void killNode(Node* node, bool mustLockNode = true);
    
    void processNodeData(sockaddr *senderAddress, unsigned char *packetData, size_t dataBytes);
    void processBulkNodeData(sockaddr *senderAddress, unsigned char *packetData, int numTotalBytes);
   
    int updateNodeWithData(Node *node, sockaddr* senderAddress, unsigned char *packetData, int dataBytes);
    
    unsigned broadcastToNodes(unsigned char *broadcastData, size_t dataBytes, const char* nodeTypes, int numNodeTypes);
    
    Node* soloNodeOfType(char nodeType);
    
    void startSilentNodeRemovalThread();
    void stopSilentNodeRemovalThread();
    
    void loadData(QSettings* settings);
    void saveData(QSettings* settings);
    
    friend class NodeListIterator;
    
    void addHook(NodeListHook* hook);
    void removeHook(NodeListHook* hook);
    void notifyHooksOfAddedNode(Node* node);
    void notifyHooksOfKilledNode(Node* node);
    
    void addDomainListener(DomainChangeListener* listener);
    void removeDomainListener(DomainChangeListener* listener);
    
    void possiblyPingInactiveNodes();
    sockaddr* getNodeActiveSocketOrPing(Node* node);
private:
    static NodeList* _sharedInstance;
    
    NodeList(char ownerType, unsigned short int socketListenPort);
    ~NodeList();
    NodeList(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(NodeList const&); // Don't implement, needed to avoid copies of singleton
    
    void addNodeToList(Node* newNode);
    
    void sendSTUNRequest();
    void processSTUNResponse(unsigned char* packetData, size_t dataBytes);
    
    QString _domainHostname;
    QHostAddress _domainIP;
    unsigned short _domainPort;
    Node** _nodeBuckets[MAX_NUM_NODES / NODES_PER_BUCKET];
    int _numNodes;
    UDPSocket _nodeSocket;
    char _ownerType;
    char* _nodeTypesOfInterest;
    QUuid _ownerUUID;
    pthread_t removeSilentNodesThread;
    pthread_t checkInWithDomainServerThread;
    int _numNoReplyDomainCheckIns;
    sockaddr* _assignmentServerSocket;
    QHostAddress _publicAddress;
    uint16_t _publicPort;
    bool _hasCompletedInitialSTUNFailure;
    unsigned int _stunRequestsSinceSuccess;
    
    void activateSocketFromNodeCommunication(sockaddr *nodeAddress);
    void timePingReply(sockaddr *nodeAddress, unsigned char *packetData);
    
    std::vector<NodeListHook*> _hooks;
    std::vector<DomainChangeListener*> _domainListeners;
    
    void resetDomainData(char domainField[], const char* domainData);
    void notifyDomainChanged();
    void domainLookup();
};

class NodeListIterator : public std::iterator<std::input_iterator_tag, Node> {
public:
    NodeListIterator(const NodeList* nodeList, int nodeIndex);
    
    int getNodeIndex() { return _nodeIndex; }
    
	NodeListIterator& operator=(const NodeListIterator& otherValue);
    
    bool operator==(const NodeListIterator& otherValue);
	bool operator!= (const NodeListIterator& otherValue);
    
    Node& operator*();
    Node* operator->();
    
	NodeListIterator& operator++();
    NodeListIterator operator++(int);
private:
    void skipDeadAndStopIncrement();
    
    const NodeList* _nodeList;
    int _nodeIndex;
};

#endif /* defined(__hifi__NodeList__) */
