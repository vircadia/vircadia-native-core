//
//  RenderableZoneEntityItem.cpp
//
//
//  Created by Clement on 4/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableZoneEntityItem.h"

#include <gpu/Batch.h>

#include <AbstractViewStateInterface.h>
#include <DeferredLightingEffect.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <PerfStat.h>

EntityItemPointer RenderableZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableZoneEntityItem>(entityID, properties);
}

template<typename Lambda>
void RenderableZoneEntityItem::changeProperties(Lambda setNewProperties) {
    QString oldShapeURL = getCompoundShapeURL();
    glm::vec3 oldPosition = getPosition(), oldDimensions = getDimensions();
    glm::quat oldRotation = getRotation();
    
    setNewProperties();
    
    if (oldShapeURL != getCompoundShapeURL()) {
        if (_model) {
            delete _model;
        }
        
        _model = getModel();
        _needsInitialSimulation = true;
        _model->setURL(getCompoundShapeURL(), QUrl(), true, true);
    }
    if (oldPosition != getPosition() ||
        oldRotation != getRotation() ||
        oldDimensions != getDimensions()) {
        _needsInitialSimulation = true;
    }
}

bool RenderableZoneEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    changeProperties([&]() {
        somethingChanged = this->ZoneEntityItem::setProperties(properties);
    });
    return somethingChanged;
}

int RenderableZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                                ReadBitstreamToTreeParams& args,
                                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    int bytesRead = 0;
    changeProperties([&]() {
        bytesRead = ZoneEntityItem::readEntitySubclassDataFromBuffer(data, bytesLeftToRead,
                                                                     args, propertyFlags, overwriteLocalData);
    });
    return bytesRead;
}

Model* RenderableZoneEntityItem::getModel() {
    Model* model = new Model();
    model->setIsWireframe(true);
    model->init();
    return model;
}

void RenderableZoneEntityItem::initialSimulation() {
    _model->setScaleToFit(true, getDimensions());
    _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
    _model->setRotation(getRotation());
    _model->setTranslation(getPosition());
    _model->simulate(0.0f);
    _needsInitialSimulation = false;
}

void RenderableZoneEntityItem::updateGeometry() {
    if (_model && !_model->isActive() && hasCompoundShapeURL()) {
        // Since we have a delayload, we need to update the geometry if it has been downloaded
        _model->setURL(getCompoundShapeURL(), QUrl(), true);
    }
    if (_model && _model->isActive() && _needsInitialSimulation) {
        initialSimulation();
    }
}

void RenderableZoneEntityItem::render(RenderArgs* args) {
    Q_ASSERT(getType() == EntityTypes::Zone);
    
    if (_drawZoneBoundaries) {
        switch (getShapeType()) {
            case SHAPE_TYPE_COMPOUND: {
                PerformanceTimer perfTimer("zone->renderCompound");
                updateGeometry();
                if (_model && _model->needsFixupInScene()) {
                    // check to see if when we added our models to the scene they were ready, if they were not ready, then
                    // fix them up in the scene
                    render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
                    render::PendingChanges pendingChanges;
                    _model->removeFromScene(scene, pendingChanges);
                    _model->addToScene(scene, pendingChanges);
                    
                    scene->enqueuePendingChanges(pendingChanges);
                    
                    _model->setVisibleInScene(getVisible(), scene);
                }
                break;
            }
            case SHAPE_TYPE_BOX:
            case SHAPE_TYPE_SPHERE: {
                PerformanceTimer perfTimer("zone->renderPrimitive");
                glm::vec4 DEFAULT_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
                
                Q_ASSERT(args->_batch);
                gpu::Batch& batch = *args->_batch;
                batch.setModelTransform(getTransformToCenter());
                
                auto deferredLightingEffect = DependencyManager::get<DeferredLightingEffect>();
                
                if (getShapeType() == SHAPE_TYPE_SPHERE) {
                    const int SLICES = 15, STACKS = 15;
                    deferredLightingEffect->renderWireSphere(batch, 0.5f, SLICES, STACKS, DEFAULT_COLOR);
                } else {
                    deferredLightingEffect->renderWireCube(batch, 1.0f, DEFAULT_COLOR);
                }
                break;
            }
            default:
                // Not handled
                break;
        }
    }
    
    if ((!_drawZoneBoundaries || getShapeType() != SHAPE_TYPE_COMPOUND) &&
        _model && !_model->needsFixupInScene()) {
        // If the model is in the scene but doesn't need to be, remove it.
        render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
        render::PendingChanges pendingChanges;
        _model->removeFromScene(scene, pendingChanges);
        scene->enqueuePendingChanges(pendingChanges);
    }
}

bool RenderableZoneEntityItem::contains(const glm::vec3& point) const {
    if (getShapeType() != SHAPE_TYPE_COMPOUND) {
        return EntityItem::contains(point);
    }
    const_cast<RenderableZoneEntityItem*>(this)->updateGeometry();
    
    if (_model && _model->isActive() && EntityItem::contains(point)) {
        return _model->convexHullContains(point);
    }
    
    return false;
}

class RenderableZoneEntityItemMeta {
public:
    RenderableZoneEntityItemMeta(EntityItemPointer entity) : entity(entity){ }
    typedef render::Payload<RenderableZoneEntityItemMeta> Payload;
    typedef Payload::DataPointer Pointer;
    
    EntityItemPointer entity;
};

namespace render {
    template <> const ItemKey payloadGetKey(const RenderableZoneEntityItemMeta::Pointer& payload) {
        return ItemKey::Builder::opaqueShape();
    }
    
    template <> const Item::Bound payloadGetBound(const RenderableZoneEntityItemMeta::Pointer& payload) {
        if (payload && payload->entity) {
            return payload->entity->getAABox();
        }
        return render::Item::Bound();
    }
    template <> void payloadRender(const RenderableZoneEntityItemMeta::Pointer& payload, RenderArgs* args) {
        if (args) {
            if (payload && payload->entity) {
                payload->entity->render(args);
            }
        }
    }
}

bool RenderableZoneEntityItem::addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene,
                                           render::PendingChanges& pendingChanges) {
    _myMetaItem = scene->allocateID();
    
    auto renderData = std::make_shared<RenderableZoneEntityItemMeta>(self);
    auto renderPayload = std::make_shared<RenderableZoneEntityItemMeta::Payload>(renderData);
    
    pendingChanges.resetItem(_myMetaItem, renderPayload);
    return true;
}

void RenderableZoneEntityItem::removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene,
                                                render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_myMetaItem);
    if (_model) {
        _model->removeFromScene(scene, pendingChanges);
    }
}
