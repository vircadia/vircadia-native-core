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

#include <NodeList.h>

#include "OctreeLogging.h"
#include "OctreeHeadlessViewer.h"

OctreeHeadlessViewer::OctreeHeadlessViewer() :
    OctreeRenderer(),
    _voxelSizeScale(DEFAULT_OCTREE_SIZE_SCALE),
    _boundaryLevelAdjust(0),
    _maxPacketsPerSecond(DEFAULT_MAX_OCTREE_PPS)
{
    _viewFrustum.setProjection(glm::perspective(glm::radians(DEFAULT_FIELD_OF_VIEW_DEGREES), DEFAULT_ASPECT_RATIO, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
}

OctreeHeadlessViewer::~OctreeHeadlessViewer() {
}

void OctreeHeadlessViewer::init() {
    OctreeRenderer::init();
    setViewFrustum(&_viewFrustum);
}

void OctreeHeadlessViewer::queryOctree() {
    char serverType = getMyNodeType();
    PacketType packetType = getMyQueryMessageType();
    NodeToJurisdictionMap& jurisdictions = *_jurisdictionListener->getJurisdictions();

    bool wantExtraDebugging = false;

    if (wantExtraDebugging) {
        qCDebug(octree) << "OctreeHeadlessViewer::queryOctree() _jurisdictionListener=" << _jurisdictionListener;
        qCDebug(octree) << "---------------";
        qCDebug(octree) << "_jurisdictionListener=" << _jurisdictionListener;
        qCDebug(octree) << "Jurisdictions...";
        jurisdictions.lockForRead();
        for (NodeToJurisdictionMapIterator i = jurisdictions.begin(); i != jurisdictions.end(); ++i) {
            qCDebug(octree) << i.key() << ": " << &i.value();
        }
        jurisdictions.unlock();
        qCDebug(octree) << "---------------";
    }

    // These will be the same for all servers, so we can set them up once and then reuse for each server we send to.
    _octreeQuery.setWantLowResMoving(true);
    _octreeQuery.setWantColor(true);
    _octreeQuery.setWantDelta(true);
    _octreeQuery.setWantOcclusionCulling(false);
    _octreeQuery.setWantCompression(true); // TODO: should be on by default

    _octreeQuery.setCameraPosition(_viewFrustum.getPosition());
    _octreeQuery.setCameraOrientation(_viewFrustum.getOrientation());
    _octreeQuery.setCameraFov(_viewFrustum.getFieldOfView());
    _octreeQuery.setCameraAspectRatio(_viewFrustum.getAspectRatio());
    _octreeQuery.setCameraNearClip(_viewFrustum.getNearClip());
    _octreeQuery.setCameraFarClip(_viewFrustum.getFarClip());
    _octreeQuery.setCameraEyeOffsetPosition(glm::vec3());

    _octreeQuery.setOctreeSizeScale(getVoxelSizeScale());
    _octreeQuery.setBoundaryLevelAdjust(getBoundaryLevelAdjust());

    // Iterate all of the nodes, and get a count of how many voxel servers we have...
    int totalServers = 0;
    int inViewServers = 0;
    int unknownJurisdictionServers = 0;

    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& node){
        // only send to the NodeTypes that are serverType
        if (node->getActiveSocket() && node->getType() == serverType) {
            totalServers++;

            // get the server bounds for this server
            QUuid nodeUUID = node->getUUID();

            // if we haven't heard from this voxel server, go ahead and send it a query, so we
            // can get the jurisdiction...
            jurisdictions.lockForRead();
            if (jurisdictions.find(nodeUUID) == jurisdictions.end()) {
                jurisdictions.unlock();
                unknownJurisdictionServers++;
            } else {
                const JurisdictionMap& map = (jurisdictions)[nodeUUID];

                unsigned char* rootCode = map.getRootOctalCode();

                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    jurisdictions.unlock();
                    AACube serverBounds(glm::vec3(rootDetails.x, rootDetails.y, rootDetails.z), rootDetails.s);

                    ViewFrustum::location serverFrustumLocation = _viewFrustum.cubeInFrustum(serverBounds);

                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inViewServers++;
                    }
                } else {
                    jurisdictions.unlock();
                }
            }
        }
    });

    if (wantExtraDebugging) {
        qCDebug(octree, "Servers: total %d, in view %d, unknown jurisdiction %d",
            totalServers, inViewServers, unknownJurisdictionServers);
    }

    int perServerPPS = 0;
    const int SMALL_BUDGET = 10;
    int perUnknownServer = SMALL_BUDGET;
    int totalPPS = getMaxPacketsPerSecond();

    // determine PPS based on number of servers
    if (inViewServers >= 1) {
        // set our preferred PPS to be exactly evenly divided among all of the voxel servers... and allocate 1 PPS
        // for each unknown jurisdiction server
        perServerPPS = (totalPPS / inViewServers) - (unknownJurisdictionServers * perUnknownServer);
    } else {
        if (unknownJurisdictionServers > 0) {
            perUnknownServer = (totalPPS / unknownJurisdictionServers);
        }
    }

    if (wantExtraDebugging) {
        qCDebug(octree, "perServerPPS: %d perUnknownServer: %d", perServerPPS, perUnknownServer);
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->eachNode([&](const SharedNodePointer& node){
        // only send to the NodeTypes that are serverType
        if (node->getActiveSocket() && node->getType() == serverType) {

            // get the server bounds for this server
            QUuid nodeUUID = node->getUUID();

            bool inView = false;
            bool unknownView = false;

            // if we haven't heard from this voxel server, go ahead and send it a query, so we
            // can get the jurisdiction...
            jurisdictions.lockForRead();
            if (jurisdictions.find(nodeUUID) == jurisdictions.end()) {
                jurisdictions.unlock();
                unknownView = true; // assume it's in view
                if (wantExtraDebugging) {
                    qCDebug(octree) << "no known jurisdiction for node " << *node << ", assume it's visible.";
                }
            } else {
                const JurisdictionMap& map = (jurisdictions)[nodeUUID];

                unsigned char* rootCode = map.getRootOctalCode();

                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    jurisdictions.unlock();
                    AACube serverBounds(glm::vec3(rootDetails.x, rootDetails.y, rootDetails.z), rootDetails.s);

                    ViewFrustum::location serverFrustumLocation = _viewFrustum.cubeInFrustum(serverBounds);
                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inView = true;
                    } else {
                        inView = false;
                    }
                } else {
                    jurisdictions.unlock();
                    if (wantExtraDebugging) {
                        qCDebug(octree) << "Jurisdiction without RootCode for node " << *node << ". That's unusual!";
                    }
                }
            }

            if (inView) {
                _octreeQuery.setMaxQueryPacketsPerSecond(perServerPPS);
                if (wantExtraDebugging) {
                    qCDebug(octree) << "inView for node " << *node << ", give it budget of " << perServerPPS;
                }
            } else if (unknownView) {
                if (wantExtraDebugging) {
                    qCDebug(octree) << "no known jurisdiction for node " << *node << ", give it budget of "
                    << perUnknownServer << " to send us jurisdiction.";
                }

                // set the query's position/orientation to be degenerate in a manner that will get the scene quickly
                // If there's only one server, then don't do this, and just let the normal voxel query pass through
                // as expected... this way, we will actually get a valid scene if there is one to be seen
                if (totalServers > 1) {
                    _octreeQuery.setCameraPosition(glm::vec3(-0.1,-0.1,-0.1));
                    const glm::quat OFF_IN_NEGATIVE_SPACE = glm::quat(-0.5, 0, -0.5, 1.0);
                    _octreeQuery.setCameraOrientation(OFF_IN_NEGATIVE_SPACE);
                    _octreeQuery.setCameraNearClip(0.1f);
                    _octreeQuery.setCameraFarClip(0.1f);
                    if (wantExtraDebugging) {
                        qCDebug(octree) << "Using 'minimal' camera position for node" << *node;
                    }
                } else {
                    if (wantExtraDebugging) {
                        qCDebug(octree) << "Using regular camera position for node" << *node;
                    }
                }
                _octreeQuery.setMaxQueryPacketsPerSecond(perUnknownServer);
            } else {
                _octreeQuery.setMaxQueryPacketsPerSecond(0);
            }

            // setup the query packet
            auto queryPacket = NLPacket::create(packetType);
            _octreeQuery.getBroadcastData(reinterpret_cast<unsigned char*>(queryPacket->getPayload()));

            // ask the NodeList to send it
            nodeList->sendPacket(std::move(queryPacket), *node);
        }
    });
}


int OctreeHeadlessViewer::parseOctreeStats(QSharedPointer<NLPacket> packet, SharedNodePointer sourceNode) {

    OctreeSceneStats temp;
    int statsMessageLength = temp.unpackFromPacket(*packet);

    // TODO: actually do something with these stats, like expose them to JS...

    return statsMessageLength;
}

void OctreeHeadlessViewer::trackIncomingOctreePacket(const QByteArray& packet,
                                const SharedNodePointer& sendingNode, bool wasStatsPacket) {

}
