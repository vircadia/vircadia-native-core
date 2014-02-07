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

#include <NodeData.h>

class DomainServerNodeData : public NodeData {
public:
    DomainServerNodeData() : _sessionSecretHash() {};
    int parseData(const QByteArray& packet) { return 0; }
    
    QHash<QUuid, QUuid>& getSessionSecretHash() { return _sessionSecretHash; }
private:
    QHash<QUuid, QUuid> _sessionSecretHash;
};

#endif /* defined(__hifi__DomainServerNodeData__) */
