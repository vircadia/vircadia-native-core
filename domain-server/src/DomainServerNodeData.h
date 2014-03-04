//
//  DomainServerNodeData.h
//  hifi
//
//  Created by Stephen Birarda on 2/6/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__DomainServerNodeData__
#define __hifi__DomainServerNodeData__

#include <QtCore/QHash>
#include <QtCore/QUuid>

#include <NodeData.h>

class DomainServerNodeData : public NodeData {
public:
    DomainServerNodeData();
    int parseData(const QByteArray& packet) { return 0; }
    
    void setStaticAssignmentUUID(const QUuid& staticAssignmentUUID) { _staticAssignmentUUID = staticAssignmentUUID; }
    const QUuid& getStaticAssignmentUUID() const { return _staticAssignmentUUID; }
    
    QHash<QUuid, QUuid>& getSessionSecretHash() { return _sessionSecretHash; }
private:
    QHash<QUuid, QUuid> _sessionSecretHash;
    QUuid _staticAssignmentUUID;
};

#endif /* defined(__hifi__DomainServerNodeData__) */
