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


#include "RenderableEntityItem.h"

#include <ObjectMotionState.h>

#include "EntityTreeRenderer.h"
#include "RenderableLightEntityItem.h"
#include "RenderableLineEntityItem.h"
#include "RenderableModelEntityItem.h"
#include "RenderableParticleEffectEntityItem.h"
#include "RenderablePolyVoxEntityItem.h"
#include "RenderablePolyLineEntityItem.h"
#include "RenderableShapeEntityItem.h"
#include "RenderableTextEntityItem.h"
#include "RenderableWebEntityItem.h"
#include "RenderableZoneEntityItem.h"

using namespace render;
using namespace render::entities;

// These or the icon "name" used by the render item status value, they correspond to the atlas texture used by the DrawItemStatus
// job in the current rendering pipeline defined as of now  (11/2015) in render-utils/RenderDeferredTask.cpp.
enum class RenderItemStatusIcon {
    ACTIVE_IN_BULLET = 0,
    PACKET_SENT = 1,
    PACKET_RECEIVED = 2,
    SIMULATION_OWNER = 3,
    HAS_ACTIONS = 4,
    OTHER_SIMULATION_OWNER = 5,
    CLIENT_ONLY = 6,
    NONE = 255
};

std::function<bool()> EntityRenderer::_entitiesShouldFadeFunction = []() { return true; };

void EntityRenderer::initEntityRenderers() {
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Model, RenderableModelEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(PolyVox, RenderablePolyVoxEntityItem::factory)
}



void EntityRenderer::makeStatusGetters(const EntityItemPointer& entity, Item::Status::Getters& statusGetters) {
    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid& myNodeID = nodeList->getSessionUUID();

    statusGetters.push_back([entity]() -> render::Item::Status::Value {
        quint64 delta = usecTimestampNow() - entity->getLastEditedFromRemote();
        const float WAIT_THRESHOLD_INV = 1.0f / (0.2f * USECS_PER_SECOND);
        float normalizedDelta = delta * WAIT_THRESHOLD_INV;
        // Status icon will scale from 1.0f down to 0.0f after WAIT_THRESHOLD
        // Color is red if last update is after WAIT_THRESHOLD, green otherwise (120 deg is green)
        return render::Item::Status::Value(1.0f - normalizedDelta, (normalizedDelta > 1.0f ?
            render::Item::Status::Value::GREEN :
            render::Item::Status::Value::RED),
            (unsigned char)RenderItemStatusIcon::PACKET_RECEIVED);
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


template <typename T> 
std::shared_ptr<T> make_renderer(const EntityItemPointer& entity) {
    T* rawResult = new T(entity);

    // We want to use deleteLater so that renderer destruction gets pushed to the main thread
    return std::shared_ptr<T>(rawResult, std::bind(&QObject::deleteLater, rawResult));
}

EntityRenderer::EntityRenderer(const EntityItemPointer& entity) : _entity(entity) {
}

EntityRenderer::~EntityRenderer() { }

//
// Smart payload proxy members, implementing the payload interface
//

Item::Bound EntityRenderer::getBound() {
    return _bound;
}

ItemKey EntityRenderer::getKey() {
    if (isTransparent()) {
        return ItemKey::Builder::transparentShape().withTypeMeta();
    }

    return ItemKey::Builder::opaqueShape().withTypeMeta();
}

uint32_t EntityRenderer::metaFetchMetaSubItems(ItemIDs& subItems) {
    if (Item::isValidID(_renderItemID)) {
        subItems.emplace_back(_renderItemID);
        return 1;
    }
    return 0;
}

void EntityRenderer::render(RenderArgs* args) {
    if (!isValidRenderItem()) {
        return;
    }

    if (!_renderUpdateQueued && needsRenderUpdate()) {
        // FIXME find a way to spread out the calls to needsRenderUpdate so that only a given subset of the 
        // items checks every frame, like 1/N of the tree ever N frames
        _renderUpdateQueued = true;
        emit requestRenderUpdate();
    }

    if (_visible) {
        doRender(args);
    }
}

//
// Methods called by the EntityTreeRenderer
//

EntityRenderer::Pointer EntityRenderer::addToScene(EntityTreeRenderer& renderer, const EntityItemPointer& entity, const ScenePointer& scene) {
    EntityRenderer::Pointer result;
    if (!entity) {
        return result;
    }

    using Type = EntityTypes::EntityType_t;
    auto type = entity->getType();
    switch (type) {
        case Type::Light:
            result = make_renderer<LightEntityRenderer>(entity);
            break;

        case Type::Line:
            result = make_renderer<LineEntityRenderer>(entity);
            break;

        case Type::Model:
            result = make_renderer<ModelEntityRenderer>(entity);
            break;

        case Type::ParticleEffect:
            result = make_renderer<ParticleEffectEntityRenderer>(entity);
            break;

        case Type::PolyLine:
            result = make_renderer<PolyLineEntityRenderer>(entity);
            break;

        case Type::PolyVox:
            result = make_renderer<PolyVoxEntityRenderer>(entity);
            break;

        case Type::Shape:
        case Type::Box:
        case Type::Sphere:
            result = make_renderer<ShapeEntityRenderer>(entity);
            break;

        case Type::Text:
            result = make_renderer<TextEntityRenderer>(entity);
            break;

        case Type::Web:
            if (!nsightActive()) {
                result = make_renderer<WebEntityRenderer>(entity);
            }
            break;

        case Type::Zone:
            result = make_renderer<ZoneEntityRenderer>(entity);
            break;

        default:
            break;
    }

    if (result) {
        Transaction transaction;
        result->addToScene(scene, transaction);
        scene->enqueueTransaction(transaction);
    }

    return result;
}

bool EntityRenderer::addToScene(const ScenePointer& scene, Transaction& transaction) {
    _renderItemID = scene->allocateID();
    // Complicated series of trusses
    auto renderPayload = std::make_shared<PayloadProxyInterface::ProxyPayload>(shared_from_this());
    Item::Status::Getters statusGetters;
    makeStatusGetters(_entity, statusGetters);
    renderPayload->addStatusGetters(statusGetters);
    transaction.resetItem(_renderItemID, renderPayload);
    updateInScene(scene, transaction);
    onAddToScene(_entity);
    return true;
}

void EntityRenderer::removeFromScene(const ScenePointer& scene, Transaction& transaction) {
    onRemoveFromScene(_entity);
    transaction.removeItem(_renderItemID);
    Item::clearID(_renderItemID);
}

void EntityRenderer::updateInScene(const ScenePointer& scene, Transaction& transaction) {
    if (!isValidRenderItem()) {
        return;
    }

    // FIXME is this excessive?
    if (!needsRenderUpdate()) {
        return;
    }

    doRenderUpdateSynchronous(scene, transaction, _entity);
    transaction.updateItem<EntityRenderer>(_renderItemID, [this](EntityRenderer& self) {
        if (!isValidRenderItem()) {
            return;
        }
        // Happens on the render thread.  Classes should use
        doRenderUpdateAsynchronous(_entity);
        _renderUpdateQueued = false;
    });
}

//
// Internal methods
//

// Returns true if the item needs to have updateInscene called because of internal rendering 
// changes (animation, fading, etc)
bool EntityRenderer::needsRenderUpdate() const {
    if (_prevIsTransparent != isTransparent()) {
        return true;
    }
    return needsRenderUpdateFromEntity(_entity);
}

// Returns true if the item in question needs to have updateInScene called because of changes in the entity
bool EntityRenderer::needsRenderUpdateFromEntity(const EntityItemPointer& entity) const {
    bool success = false;
    auto bound = _entity->getAABox(success);
    if (success && _bound != bound) {
        return true;
    }

    auto newModelTransform = _entity->getTransformToCenter(success);
    // FIXME can we use a stale model transform here?
    if (success && newModelTransform != _modelTransform) {
        return true;
    }

    if (_visible != entity->getVisible()) {
        return true;
    }

    if (_moving != entity->isMovingRelativeToParent()) {
        return true;
    }

    return false;
}

void EntityRenderer::doRenderUpdateAsynchronous(const EntityItemPointer& entity) {
    auto transparent = isTransparent();
    if (_prevIsTransparent && !transparent) {
        _isFading = false;
    }
    _prevIsTransparent = transparent;

    bool success = false;
    auto bound = entity->getAABox(success);
    if (success) {
        _bound = bound;
    }
    auto newModelTransform = entity->getTransformToCenter(success);
    if (success) {
        _modelTransform = newModelTransform;
    }

    _moving = entity->isMovingRelativeToParent();
    _visible = entity->getVisible();
}

void EntityRenderer::onAddToScene(const EntityItemPointer& entity) {
    QObject::connect(this, &EntityRenderer::requestRenderUpdate, this, [this] { 
        auto renderer = DependencyManager::get<EntityTreeRenderer>();
        if (renderer) {
            renderer->onEntityChanged(_entity->getID());
        }
    }, Qt::QueuedConnection);
    _changeHandlerId = entity->registerChangeHandler([this](const EntityItemID& changedEntity) { 
        auto renderer = DependencyManager::get<EntityTreeRenderer>();
        if (renderer) {
            renderer->onEntityChanged(changedEntity);
        }
    });
}

void EntityRenderer::onRemoveFromScene(const EntityItemPointer& entity) { 
    entity->deregisterChangeHandler(_changeHandlerId);
    QObject::disconnect(this, &EntityRenderer::requestRenderUpdate, this, nullptr);
}
