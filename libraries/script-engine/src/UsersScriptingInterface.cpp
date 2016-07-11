//
//  UsersScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-07-11.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UsersScriptingInterface.h"

#include <NodeList.h>

void UsersScriptingInterface::ignore(const QUuid& nodeID) {
    // setup the ignore packet we send to all nodes (that currently handle it)
    // to ignore the data (audio/avatar) for this user

    // enumerate the nodes to send a reliable ignore packet to each that can leverage it
    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachMatchingNode([&nodeID](const SharedNodePointer& node)->bool {
        if (node->getType() == NodeType::AudioMixer || node->getType() == NodeType::AvatarMixer) {
            return true;
        } else {
            return false;
        }
    }, [&nodeID, &nodeList](const SharedNodePointer& destinationNode) {
        // create a reliable NLPacket with space for the ignore UUID
        auto ignorePacket = NLPacket::create(PacketType::NodeIgnoreRequest, NUM_BYTES_RFC4122_UUID, true);

        // write the node ID to the packet
        ignorePacket->write(nodeID.toRfc4122());

        qDebug() << "Sending packet to ignore node" << uuidStringWithoutCurlyBraces(nodeID);

        // send off this ignore packet reliably to the matching node
        nodeList->sendPacket(std::move(ignorePacket), *destinationNode);
    });

    emit ignoredNode(nodeID);
}
