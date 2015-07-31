//
//  DomainServerNodeData.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2/6/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainServerNodeData_h
#define hifi_DomainServerNodeData_h

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QUuid>

#include <HifiSockAddr.h>
#include <NLPacket.h>
#include <NodeData.h>
#include <NodeType.h>

class DomainServerNodeData : public NodeData {
public:
    DomainServerNodeData();

    const QJsonObject& getStatsJSONObject() const { return _statsJSONObject; }

    void processJSONStatsPacket(NLPacket& packet);

    void setAssignmentUUID(const QUuid& assignmentUUID) { _assignmentUUID = assignmentUUID; }
    const QUuid& getAssignmentUUID() const { return _assignmentUUID; }

    void setWalletUUID(const QUuid& walletUUID) { _walletUUID = walletUUID; }
    const QUuid& getWalletUUID() const { return _walletUUID; }

    void setUsername(const QString& username) { _username = username; }
    const QString& getUsername() const { return _username; }

    QElapsedTimer& getPaymentIntervalTimer() { return _paymentIntervalTimer; }

    void setSendingSockAddr(const HifiSockAddr& sendingSockAddr) { _sendingSockAddr = sendingSockAddr; }
    const HifiSockAddr& getSendingSockAddr() { return _sendingSockAddr; }

    void setIsAuthenticated(bool isAuthenticated) { _isAuthenticated = isAuthenticated; }
    bool isAuthenticated() const { return _isAuthenticated; }

    QHash<QUuid, QUuid>& getSessionSecretHash() { return _sessionSecretHash; }

    const NodeSet& getNodeInterestSet() const { return _nodeInterestSet; }
    void setNodeInterestSet(const NodeSet& nodeInterestSet) { _nodeInterestSet = nodeInterestSet; }
private:
    QJsonObject mergeJSONStatsFromNewObject(const QJsonObject& newObject, QJsonObject destinationObject);

    QHash<QUuid, QUuid> _sessionSecretHash;
    QUuid _assignmentUUID;
    QUuid _walletUUID;
    QString _username;
    QElapsedTimer _paymentIntervalTimer;
    QJsonObject _statsJSONObject;
    HifiSockAddr _sendingSockAddr;
    bool _isAuthenticated;
    NodeSet _nodeInterestSet;
};

#endif // hifi_DomainServerNodeData_h
