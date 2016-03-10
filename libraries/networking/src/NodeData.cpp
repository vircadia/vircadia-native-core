//
//  NodeData.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/19/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NodeData.h"

NodeData::NodeData(const QUuid& nodeID) :
    _mutex(),
    _nodeID(nodeID)
{
    
}

NodeData::~NodeData() {
    
}
