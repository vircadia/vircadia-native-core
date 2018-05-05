//
//  OctreeHeadlessViewer.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OctreeHeadlessViewer.h"

#include <NodeList.h>
#include <OctreeLogging.h>

void OctreeHeadlessViewer::queryOctree() {
    char serverType = getMyNodeType();
    PacketType packetType = getMyQueryMessageType();

    if (_hasViewFrustum) {
        _octreeQuery.setMainViewFrustum(_viewFrustum);
    }

    auto nodeList = DependencyManager::get<NodeList>();

    auto node = nodeList->soloNodeOfType(serverType);
    if (node && node->getActiveSocket()) {
        auto queryPacket = NLPacket::create(packetType);

        // encode the query data
        auto packetData = reinterpret_cast<unsigned char*>(queryPacket->getPayload());
        int packetSize = _octreeQuery.getBroadcastData(packetData);
        queryPacket->setPayloadSize(packetSize);

        // make sure we still have an active socket
        nodeList->sendUnreliablePacket(*queryPacket, *node);
    }
}


int OctreeHeadlessViewer::parseOctreeStats(QSharedPointer<ReceivedMessage> message, SharedNodePointer sourceNode) {

    OctreeSceneStats temp;
    int statsMessageLength = temp.unpackFromPacket(*message);

    // TODO: actually do something with these stats, like expose them to JS...

    return statsMessageLength;
}

void OctreeHeadlessViewer::trackIncomingOctreePacket(const QByteArray& packet,
                                const SharedNodePointer& sendingNode, bool wasStatsPacket) {

}
