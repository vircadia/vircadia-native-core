//
//  NodeData.h
//  hifi
//
//  Created by Stephen Birarda on 2/19/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_NodeData_h
#define hifi_NodeData_h

#include <QtCore/QObject>

class Node;

class NodeData : public QObject {
  Q_OBJECT
public:
    NodeData(Node* owningNode);
    
    virtual ~NodeData() = 0;
    virtual int parseData(unsigned char* sourceBuffer, int numBytes) = 0;
    
    Node* getOwningNode() { return _owningNode; }
protected:
    Node* _owningNode;
};

#endif
