 //
//  RenderableEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <ObjectMotionState.h>
#include "RenderableEntityItem.h"

namespace render {
    template <> const ItemKey payloadGetKey(const RenderableEntityItemProxy::Pointer& payload) { 
        if (payload && payload->entity) {
            if (payload->entity->getType() == EntityTypes::Light) {
                return ItemKey::Builder::light();
            }
            if (payload && (payload->entity->getType() == EntityTypes::PolyLine || payload->entity->isTransparent())) {
                return ItemKey::Builder::transparentShape();
            }
        }
        return ItemKey::Builder::opaqueShape();
    }
    
    template <> const Item::Bound payloadGetBound(const RenderableEntityItemProxy::Pointer& payload) { 
        if (payload && payload->entity) {
            bool success;
            auto result = payload->entity->getAABox(success);
            if (!success) {
                return render::Item::Bound();
            }
            return result;
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const RenderableEntityItemProxy::Pointer& payload, RenderArgs* args) {
        if (args) {
            if (payload && payload->entity && payload->entity->getVisible()) {
                payload->entity->render(args);
            }
        }
    }
}

void makeEntityItemStatusGetters(EntityItemPointer entity, render::Item::Status::Getters& statusGetters) {
    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid& myNodeID = nodeList->getSessionUUID();

    statusGetters.push_back([entity] () -> render::Item::Status::Value {
        quint64 delta = usecTimestampNow() - entity->getLastEditedFromRemote();
        const float WAIT_THRESHOLD_INV = 1.0f / (0.2f * USECS_PER_SECOND);
        float normalizedDelta = delta * WAIT_THRESHOLD_INV;
        // Status icon will scale from 1.0f down to 0.0f after WAIT_THRESHOLD
        // Color is red if last update is after WAIT_THRESHOLD, green otherwise (120 deg is green)
        return render::Item::Status::Value(1.0f - normalizedDelta, (normalizedDelta > 1.0f ?
                                                                    render::Item::Status::Value::GREEN :
                                                                    render::Item::Status::Value::RED),
                                           (unsigned char) RenderItemStatusIcon::PACKET_RECEIVED);
    });

    statusGetters.push_back([entity] () -> render::Item::Status::Value {
        quint64 delta = usecTimestampNow() - entity->getLastBroadcast();
        const float WAIT_THRESHOLD_INV = 1.0f / (0.4f * USECS_PER_SECOND);
        float normalizedDelta = delta * WAIT_THRESHOLD_INV;
        // Status icon will scale from 1.0f down to 0.0f after WAIT_THRESHOLD
        // Color is Magenta if last update is after WAIT_THRESHOLD, cyan otherwise (180 deg is green)
        return render::Item::Status::Value(1.0f - normalizedDelta, (normalizedDelta > 1.0f ?
                                                                    render::Item::Status::Value::MAGENTA :
                                                                    render::Item::Status::Value::CYAN),
                                           (unsigned char)RenderItemStatusIcon::PACKET_SENT);
    });

    statusGetters.push_back([entity] () -> render::Item::Status::Value {
        ObjectMotionState* motionState = static_cast<ObjectMotionState*>(entity->getPhysicsInfo());
        if (motionState && motionState->isActive()) {
            return render::Item::Status::Value(1.0f, render::Item::Status::Value::BLUE,
                                               (unsigned char)RenderItemStatusIcon::ACTIVE_IN_BULLET);
        }
        return render::Item::Status::Value(0.0f, render::Item::Status::Value::BLUE,
                                           (unsigned char)RenderItemStatusIcon::ACTIVE_IN_BULLET);
    });

    statusGetters.push_back([entity, myNodeID] () -> render::Item::Status::Value {
        bool weOwnSimulation = entity->getSimulationOwner().matchesValidID(myNodeID);
        bool otherOwnSimulation = !weOwnSimulation && !entity->getSimulationOwner().isNull();

        if (weOwnSimulation) {
            return render::Item::Status::Value(1.0f, render::Item::Status::Value::BLUE,
                                               (unsigned char)RenderItemStatusIcon::SIMULATION_OWNER);
        } else if (otherOwnSimulation) {
            return render::Item::Status::Value(1.0f, render::Item::Status::Value::RED,
                                               (unsigned char)RenderItemStatusIcon::OTHER_SIMULATION_OWNER);
        }
        return render::Item::Status::Value(0.0f, render::Item::Status::Value::BLUE,
                                           (unsigned char)RenderItemStatusIcon::SIMULATION_OWNER);
    });

    statusGetters.push_back([entity] () -> render::Item::Status::Value {
        if (entity->hasActions()) {
            return render::Item::Status::Value(1.0f, render::Item::Status::Value::GREEN,
                                               (unsigned char)RenderItemStatusIcon::HAS_ACTIONS);
        }
        return render::Item::Status::Value(0.0f, render::Item::Status::Value::GREEN,
                                           (unsigned char)RenderItemStatusIcon::HAS_ACTIONS);
    });

    statusGetters.push_back([entity, myNodeID] () -> render::Item::Status::Value {
        if (entity->getClientOnly()) {
            if (entity->getOwningAvatarID() == myNodeID) {
                return render::Item::Status::Value(1.0f, render::Item::Status::Value::GREEN,
                                                   (unsigned char)RenderItemStatusIcon::CLIENT_ONLY);
            } else {
                return render::Item::Status::Value(1.0f, render::Item::Status::Value::RED,
                                                   (unsigned char)RenderItemStatusIcon::CLIENT_ONLY);
            }
        }
        return render::Item::Status::Value(0.0f, render::Item::Status::Value::GREEN,
                                           (unsigned char)RenderItemStatusIcon::CLIENT_ONLY);
    });
}
