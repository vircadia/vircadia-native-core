//
//  GrabManager.cpp
//  interface/src/avatar/
//
//  Created by Seth Alves on 2018-12-4.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "GrabManager.h"
#include <QtCore/QSharedPointer>

void GrabManager::simulateGrabs() {
    QSharedPointer<AvatarManager> avatarManager = DependencyManager::get<AvatarManager>();

    // Update grabbed objects
    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>();
    auto entityTree = entityTreeRenderer->getTree();
    auto sessionID = DependencyManager::get<NodeList>()->getSessionUUID();
    EntityEditPacketSender* packetSender = entityTreeRenderer ? entityTreeRenderer->getPacketSender() : nullptr;
    entityTree->withReadLock([&] {
        PROFILE_RANGE(simulation, "Grabs");

        std::map<QUuid, GrabLocationAccumulator> grabAccumulators;
        avatarManager->accumulateGrabPositions(grabAccumulators);

        for (auto& accumulatedLocation : grabAccumulators) {
            QUuid grabbedThingID = accumulatedLocation.first;
            GrabLocationAccumulator& acc = accumulatedLocation.second;
            bool success;
            SpatiallyNestablePointer grabbedThing = SpatiallyNestable::findByID(grabbedThingID, success);
            if (success && grabbedThing) {
                auto entity = std::dynamic_pointer_cast<EntityItem>(grabbedThing);
                if (entity) {
                    if (entity->getLocked()) {
                        continue; // even if someone else claims to be grabbing it, don't move a locked thing
                    }
                    const GrabPropertyGroup& grabProps = entity->getGrabProperties();
                    if (!grabProps.getGrabbable()) {
                        continue; // even if someone else claims to be grabbing it, don't move non-grabbable
                    }
                }

                glm::vec3 finalPosition = acc.finalizePosition();
                glm::quat finalOrientation = acc.finalizeOrientation();
                grabbedThing->setTransform(createMatFromQuatAndPos(finalOrientation, finalPosition));
                bool iShouldTellServer = grabbedThing->getEditSenderID() == sessionID;
                entityTree->updateEntityQueryAACube(grabbedThing, packetSender, false, iShouldTellServer);
            }
        }
    });
}
