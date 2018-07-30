//
//  AgentScriptingInterface.cpp
//  assignment-client/src
//
//  Created by Thijs Wenker on 7/23/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AgentScriptingInterface.h"

AgentScriptingInterface::AgentScriptingInterface(Agent* agent) :
    QObject(agent),
    _agent(agent)
{ }
