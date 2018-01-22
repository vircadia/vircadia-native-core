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


OctreeHeadlessViewer::OctreeHeadlessViewer() {
    _viewFrustum.setProjection(glm::perspective(glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES), DEFAULT_ASPECT_RATIO, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
}

void OctreeHeadlessViewer::queryOctree() {
    char serverType = getMyNodeType();
    PacketType packetType = getMyQueryMessageType();

    _octreeQuery.setCameraPosition(_viewFrustum.getPosition());
    _octreeQuery.setCameraOrientation(_viewFrustum.getOrientation());
    _octreeQuery.setCameraFov(_viewFrustum.getFieldOfView());
    _octreeQuery.setCameraAspectRatio(_viewFrustum.getAspectRatio());
    _octreeQuery.setCameraNearClip(_viewFrustum.getNearClip());
    _octreeQuery.setCameraFarClip(_viewFrustum.getFarClip());
    _octreeQuery.setCameraEyeOffsetPosition(glm::vec3());
    _octreeQuery.setCameraCenterRadius(_viewFrustum.getCenterRadius());
    _octreeQuery.setOctreeSizeScale(_voxelSizeScale);
    _octreeQuery.setBoundaryLevelAdjust(_boundaryLevelAdjust);

    auto nodeList = DependencyManager::get<NodeList>();

    auto node = nodeList->soloNodeOfType(serverType);
    if (node && node->getActiveSocket()) {
        _octreeQuery.setMaxQueryPacketsPerSecond(getMaxPacketsPerSecond());

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
