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

#include <QtCore/QHash>
#include <QtCore/QUuid>

#include <NodeData.h>

class DomainServerNodeData : public NodeData {
public:
    DomainServerNodeData();
    int parseData(const QByteArray& packet) { return 0; }
    
    const QJsonObject& getStatsJSONObject() const { return _statsJSONObject; }
    
    void parseJSONStatsPacket(const QByteArray& statsPacket);
    
    void setStaticAssignmentUUID(const QUuid& staticAssignmentUUID) { _staticAssignmentUUID = staticAssignmentUUID; }
    const QUuid& getStaticAssignmentUUID() const { return _staticAssignmentUUID; }
    
    QHash<QUuid, QUuid>& getSessionSecretHash() { return _sessionSecretHash; }
private:
    QJsonObject mergeJSONStatsFromNewObject(const QJsonObject& newObject, QJsonObject destinationObject);
    
    QHash<QUuid, QUuid> _sessionSecretHash;
    QUuid _staticAssignmentUUID;
    QJsonObject _statsJSONObject;
};

#endif // hifi_DomainServerNodeData_h
