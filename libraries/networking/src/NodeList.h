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

#include "DomainHandler.h"
#include "LimitedNodeList.h"
#include "Node.h"

const quint64 DOMAIN_SERVER_CHECK_IN_MSECS = 1 * 1000;

const int MAX_SILENT_DOMAIN_SERVER_CHECK_INS = 5;

class Assignment;

class NodeList : public LimitedNodeList {
    Q_OBJECT
public:
    static NodeList* createInstance(char ownerType, unsigned short socketListenPort = 0, unsigned short dtlsPort = 0);
    static NodeList* getInstance();
    NodeType_t getOwnerType() const { return _ownerType; }
    void setOwnerType(NodeType_t ownerType) { _ownerType = ownerType; }

    qint64 sendStatsToDomainServer(const QJsonObject& statsObject);

    int getNumNoReplyDomainCheckIns() const { return _numNoReplyDomainCheckIns; }
    DomainHandler& getDomainHandler() { return _domainHandler; }
    
    const NodeSet& getNodeInterestSet() const { return _nodeTypesOfInterest; }
    void addNodeTypeToInterestSet(NodeType_t nodeTypeToAdd);
    void addSetOfNodeTypesToNodeInterestSet(const NodeSet& setOfNodeTypes);
    void resetNodeInterestSet() { _nodeTypesOfInterest.clear(); }

    void processNodeData(const HifiSockAddr& senderSockAddr, const QByteArray& packet);
    
    int processDomainServerList(const QByteArray& packet);

    void setAssignmentServerSocket(const HifiSockAddr& serverSocket) { _assignmentServerSocket = serverSocket; }
    void sendAssignment(Assignment& assignment);
    
    void pingPunchForInactiveNode(const SharedNodePointer& node);
    
    void loadData(QSettings* settings);
    void saveData(QSettings* settings);
public slots:
    void reset();
    void sendDomainServerCheckIn();
    void pingInactiveNodes();
signals:
    void limitOfSilentDomainCheckInsReached();
private:
    NodeList(char ownerType, unsigned short socketListenPort, unsigned short dtlsListenPort);
    NodeList(NodeList const&); // Don't implement, needed to avoid copies of singleton
    void operator=(NodeList const&); // Don't implement, needed to avoid copies of singleton
    
    void sendSTUNRequest();
    bool processSTUNResponse(const QByteArray& packet);
    
    void handleICEConnectionToDomainServer();
    
    void processDomainServerAuthRequest(const QByteArray& packet);
    void requestAuthForDomainServer();
    void activateSocketFromNodeCommunication(const QByteArray& packet, const SharedNodePointer& sendingNode);
    void timePingReply(const QByteArray& packet, const SharedNodePointer& sendingNode);
    
    NodeType_t _ownerType;
    NodeSet _nodeTypesOfInterest;
    DomainHandler _domainHandler;
    int _numNoReplyDomainCheckIns;
    HifiSockAddr _assignmentServerSocket;
    bool _hasCompletedInitialSTUNFailure;
    unsigned int _stunRequestsSinceSuccess;
};

#endif // hifi_NodeList_h
