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

#include "Node.h"
#include "UDPSocket.h"

#ifdef _WIN32
#include "pthread.h"
#endif

const int MAX_NUM_NODES = 10000;
const int NODES_PER_BUCKET = 100;

const int MAX_PACKET_SIZE = 1500;
const unsigned int NODE_SOCKET_LISTEN_PORT = 40103;

const int NODE_SILENCE_THRESHOLD_USECS = 2 * 1000000;
const int DOMAIN_SERVER_CHECK_IN_USECS = 1 * 1000000;

extern const char SOLO_NODE_TYPES[3];

const int MAX_HOSTNAME_BYTES = 256;

extern const char DEFAULT_DOMAIN_HOSTNAME[MAX_HOSTNAME_BYTES];
extern const char DEFAULT_DOMAIN_IP[INET_ADDRSTRLEN];    //  IP Address will be re-set by lookup on startup
extern const int DEFAULT_DOMAINSERVER_PORT;

const int UNKNOWN_NODE_ID = -1;

class NodeListIterator;

class NodeList {
public:
    static NodeList* createInstance(char ownerType, unsigned int socketListenPort = NODE_SOCKET_LISTEN_PORT);
    static NodeList* getInstance();
    
    typedef NodeListIterator iterator;
  
    NodeListIterator begin() const;
    NodeListIterator end() const;
    
    const char* getDomainHostname() const { return _domainHostname; };
    void setDomainHostname(const char* domainHostname);
    
    void setDomainIP(const char* domainIP);
    void setDomainIPToLocalhost();
    
    char getOwnerType() const { return _ownerType; }
    
    uint16_t getLastNodeID() const { return _lastNodeID; }
    void increaseNodeID() { ++_lastNodeID; }
    
    uint16_t getOwnerID() const { return _ownerID; }
    void setOwnerID(uint16_t ownerID) { _ownerID = ownerID; }
    
    UDPSocket* getNodeSocket() { return &_nodeSocket; }
    
    unsigned int getSocketListenPort() const { return _nodeSocket.getListeningPort(); };
    
    void(*linkedDataCreateCallback)(Node *);
    
    int size() { return _numNodes; }
    int getNumAliveNodes() const;
    
    void clear();
    
    void setNodeTypesOfInterest(const char* nodeTypesOfInterest, int numNodeTypesOfInterest);
    void sendDomainServerCheckIn();
    int processDomainServerList(unsigned char *packetData, size_t dataBytes);
    
    Node* nodeWithAddress(sockaddr *senderAddress);
    Node* nodeWithID(uint16_t nodeID);
    
    Node* addOrUpdateNode(sockaddr* publicSocket, sockaddr* localSocket, char nodeType, uint16_t nodeId);
    
    void processNodeData(sockaddr *senderAddress, unsigned char *packetData, size_t dataBytes);
    void processBulkNodeData(sockaddr *senderAddress, unsigned char *packetData, int numTotalBytes);
   
    int updateNodeWithData(sockaddr *senderAddress, unsigned char *packetData, size_t dataBytes);
    int updateNodeWithData(Node *node, unsigned char *packetData, int dataBytes);
    
    unsigned broadcastToNodes(unsigned char *broadcastData, size_t dataBytes, const char* nodeTypes, int numNodeTypes);
    
    Node* soloNodeOfType(char nodeType);
    
    void startSilentNodeRemovalThread();
    void stopSilentNodeRemovalThread();
    
    friend class NodeListIterator;
private:
    static NodeList* _sharedInstance;
    
    NodeList(char ownerType, unsigned int socketListenPort);
    ~NodeList();
    NodeList(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(NodeList const&); // Don't implement, needed to avoid copies of singleton
    
    void addNodeToList(Node* newNode);
    
    char _domainHostname[MAX_HOSTNAME_BYTES];
    char _domainIP[INET_ADDRSTRLEN];
    Node** _nodeBuckets[MAX_NUM_NODES / NODES_PER_BUCKET];
    int _numNodes;
    UDPSocket _nodeSocket;
    char _ownerType;
    char* _nodeTypesOfInterest;
    unsigned int _socketListenPort;
    uint16_t _ownerID;
    uint16_t _lastNodeID;
    pthread_t removeSilentNodesThread;
    pthread_t checkInWithDomainServerThread;
    
    void handlePingReply(sockaddr *nodeAddress);
    void timePingReply(sockaddr *nodeAddress, unsigned char *packetData);
};

class NodeListIterator : public std::iterator<std::input_iterator_tag, Node> {
public:
    NodeListIterator(const NodeList* nodeList, int nodeIndex);
    ~NodeListIterator() {};
    
    int getNodeIndex() { return _nodeIndex; };
    
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
