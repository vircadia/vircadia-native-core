//
//  GVRInterface.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 11/18/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <NodeList.h>

#include "GVRInterface.h"

GVRInterface::GVRInterface(int argc, char* argv[]) : 
    QGuiApplication(argc, argv)
{
    NodeList* nodeList = NodeList::createInstance(NodeType::Agent);
}