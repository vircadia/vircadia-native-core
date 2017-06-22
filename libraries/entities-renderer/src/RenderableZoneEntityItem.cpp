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

#include <model/Stage.h>

#include <AbstractViewStateInterface.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <procedural/ProceduralSkybox.h>

#include "EntityTreeRenderer.h"
#include "RenderableEntityItem.h"

#include <LightPayload.h>
#include "DeferredLightingEffect.h"


class RenderableZoneEntityItemMeta {
public:
    RenderableZoneEntityItemMeta(EntityItemPointer entity);
    ~RenderableZoneEntityItemMeta();

    typedef render::Payload<RenderableZoneEntityItemMeta> Payload;
    typedef Payload::DataPointer Pointer;

    EntityItemPointer entity;

    void render(RenderArgs* args);

    void setVisible(bool visible) { _isVisible = visible; }
    bool isVisible() const { return _isVisible; }

    render::Item::Bound& editBound() { _needUpdate = true; return _bound; }

    model::LightPointer editSunLight() { _needSunUpdate = true; return _sunLight; }
    model::LightPointer editAmbientLight() { _needAmbientUpdate = true; return _ambientLight; }
    model::SunSkyStagePointer editBackground() { _needBackgroundUpdate = true; return _background; }
    model::SkyboxPointer editSkybox() { return editBackground()->getSkybox(); }

    void setAmbientURL(const QString& ambientUrl);

    void setSkyboxURL(const QString& skyboxUrl);

    void setBackgroundMode(BackgroundMode mode);
    void setSkyboxColor(const glm::vec3& color);
    void setProceduralUserData(QString userData);

protected:
    render::Item::Bound _bound;

    model::LightPointer _sunLight;
    model::LightPointer _ambientLight;
    model::SunSkyStagePointer _background;
    BackgroundMode _backgroundMode { BACKGROUND_MODE_INHERIT };

    LightStagePointer _stage;
    LightStage::Index _sunIndex { LightStage::INVALID_INDEX };
    LightStage::Index _ambientIndex { LightStage::INVALID_INDEX };

    BackgroundStagePointer _backgroundStage;
    BackgroundStage::Index _backgroundIndex { BackgroundStage::INVALID_INDEX };

    bool _needUpdate { true };
    bool _needSunUpdate { true };
    bool _needAmbientUpdate { true };
    bool _needBackgroundUpdate { true };
    bool _isVisible { true };


    void updateAmbientMap();
    void updateSkyboxMap();

    // More attributes used for rendering:
    QString _ambientTextureURL;
    NetworkTexturePointer _ambientTexture;
    bool _pendingAmbientTexture { false };
    bool _validAmbientTexture { false };

    QString _skyboxTextureURL;
    NetworkTexturePointer _skyboxTexture;
    bool _pendingSkyboxTexture { false };
    bool _validSkyboxTexture { false };

    QString _proceduralUserData;
};

// Sphere entities should fit inside a cube entity of the same size, so a sphere that has dimensions 1x1x1
// is a half unit sphere.  However, the geometry cache renders a UNIT sphere, so we need to scale down.
static const float SPHERE_ENTITY_SCALE = 0.5f;

EntityItemPointer RenderableZoneEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableZoneEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

template<typename Lambda>
void RenderableZoneEntityItem::changeProperties(Lambda setNewProperties) {
    QString oldShapeURL = getCompoundShapeURL();
    glm::vec3 oldPosition = getPosition(), oldDimensions = getDimensions();
    glm::quat oldRotation = getRotation();

    setNewProperties();

    if (oldShapeURL != getCompoundShapeURL()) {
        if (_model) {
            _model.reset();
        }

        _model = std::make_shared<Model>();
        _model->setIsWireframe(true);
        _model->init();
        _needsInitialSimulation = true;
        _model->setURL(getCompoundShapeURL());
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

void RenderableZoneEntityItem::somethingChangedNotification() {
    DependencyManager::get<EntityTreeRenderer>()->updateZone(_id);

    // If graphics elements are changed, we need to update the render items
    notifyChangedRenderItem();

    // Poopagate back to parent
    ZoneEntityItem::somethingChangedNotification();
}


int RenderableZoneEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                                ReadBitstreamToTreeParams& args,
                                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                                bool& somethingChanged) {
    int bytesRead = 0;
    changeProperties([&]() {
        bytesRead = ZoneEntityItem::readEntitySubclassDataFromBuffer(data, bytesLeftToRead,
                                                                     args, propertyFlags, 
                                                                     overwriteLocalData, somethingChanged);
    });
    return bytesRead;
}

void RenderableZoneEntityItem::updateGeometry() {
    if (_model && !_model->isActive() && hasCompoundShapeURL()) {
        // Since we have a delayload, we need to update the geometry if it has been downloaded
        _model->setURL(getCompoundShapeURL());
    }
    if (_model && _model->isActive() && _needsInitialSimulation) {
        _model->setScaleToFit(true, getDimensions());
        _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
        _model->setRotation(getRotation());
        _model->setTranslation(getPosition());
        _model->simulate(0.0f);
        _needsInitialSimulation = false;
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
                    render::Transaction transaction;
                    _model->removeFromScene(scene, transaction);
                    render::Item::Status::Getters statusGetters;
                    makeEntityItemStatusGetters(getThisPointer(), statusGetters);
                    _model->addToScene(scene, transaction);

                    scene->enqueueTransaction(transaction);

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

                bool success;
                auto shapeTransform = getTransformToCenter(success);
                if (!success) {
                    break;
                }
                auto geometryCache = DependencyManager::get<GeometryCache>();
                if (getShapeType() == SHAPE_TYPE_SPHERE) {
                    shapeTransform.postScale(SPHERE_ENTITY_SCALE);
                    batch.setModelTransform(shapeTransform);
                    geometryCache->renderWireSphereInstance(batch, DEFAULT_COLOR);
                } else {
                    batch.setModelTransform(shapeTransform);
                    geometryCache->renderWireCubeInstance(batch, DEFAULT_COLOR);
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
        render::Transaction transaction;
        _model->removeFromScene(scene, transaction);
        scene->enqueueTransaction(transaction);
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

bool RenderableZoneEntityItem::addToScene(const EntityItemPointer& self, const render::ScenePointer& scene,
    render::Transaction& transaction) {
    _myMetaItem = scene->allocateID();

    auto renderData = std::make_shared<RenderableZoneEntityItemMeta>(self);
    auto renderPayload = std::make_shared<RenderableZoneEntityItemMeta::Payload>(renderData);
    updateKeyZoneItemFromEntity((*renderData));
    updateKeySunFromEntity((*renderData));
    updateKeyAmbientFromEntity((*renderData));
    updateKeyBackgroundFromEntity((*renderData));

    render::Item::Status::Getters statusGetters;
    makeEntityItemStatusGetters(getThisPointer(), statusGetters);
    renderPayload->addStatusGetters(statusGetters);

    transaction.resetItem(_myMetaItem, renderPayload);

    return true;
}

void RenderableZoneEntityItem::removeFromScene(const EntityItemPointer& self, const render::ScenePointer& scene,
    render::Transaction& transaction) {
    transaction.removeItem(_myMetaItem);
    render::Item::clearID(_myMetaItem);
    if (_model) {
        _model->removeFromScene(scene, transaction);
    }
}

void RenderableZoneEntityItem::notifyBoundChanged() {
    notifyChangedRenderItem();
}

void RenderableZoneEntityItem::updateKeySunFromEntity(RenderableZoneEntityItemMeta& keyZonePayload) {
    auto sunLight = keyZonePayload.editSunLight();
    sunLight->setType(model::Light::SUN);

    sunLight->setPosition(this->getPosition());
    sunLight->setOrientation(this->getRotation());

    // Set the keylight
    sunLight->setColor(ColorUtils::toVec3(this->getKeyLightProperties().getColor()));
    sunLight->setIntensity(this->getKeyLightProperties().getIntensity());
    sunLight->setDirection(this->getKeyLightProperties().getDirection());
}

void RenderableZoneEntityItem::updateKeyAmbientFromEntity(RenderableZoneEntityItemMeta& keyZonePayload) {
    auto ambientLight = keyZonePayload.editAmbientLight();
    ambientLight->setType(model::Light::AMBIENT);

    ambientLight->setPosition(this->getPosition());
    ambientLight->setOrientation(this->getRotation());


    // Set the keylight
    ambientLight->setAmbientIntensity(this->getKeyLightProperties().getAmbientIntensity());
 
    if (this->getKeyLightProperties().getAmbientURL().isEmpty()) {
        keyZonePayload.setAmbientURL(this->getSkyboxProperties().getURL());
    } else {
        keyZonePayload.setAmbientURL(this->getKeyLightProperties().getAmbientURL());
    }

}

void RenderableZoneEntityItem::updateKeyBackgroundFromEntity(RenderableZoneEntityItemMeta& keyZonePayload) {
    auto background = keyZonePayload.editBackground();

    keyZonePayload.setBackgroundMode(this->getBackgroundMode());
    keyZonePayload.setSkyboxColor(this->getSkyboxProperties().getColorVec3());
    keyZonePayload.setProceduralUserData(this->getUserData());
    keyZonePayload.setSkyboxURL(this->getSkyboxProperties().getURL());
}


void RenderableZoneEntityItem::updateKeyZoneItemFromEntity(RenderableZoneEntityItemMeta& keyZonePayload) {

    keyZonePayload.setVisible(this->getVisible());

    bool success;
    keyZonePayload.editBound() = this->getAABox(success);
    if (!success) {
        keyZonePayload.editBound() = render::Item::Bound();
    }

    /* TODO: Implement the sun model behavior / Keep this code here for reference, this is how we
    {
        // Set the stage
        bool isSunModelEnabled = this->getStageProperties().getSunModelEnabled();
        sceneStage->setSunModelEnable(isSunModelEnabled);
        if (isSunModelEnabled) {
            sceneStage->setLocation(this->getStageProperties().getLongitude(),
                this->getStageProperties().getLatitude(),
                this->getStageProperties().getAltitude());

            auto sceneTime = sceneStage->getTime();
            sceneTime->setHour(this->getStageProperties().calculateHour());
            sceneTime->setDay(this->getStageProperties().calculateDay());
        }
    }*/
}


void RenderableZoneEntityItem::sceneUpdateRenderItemFromEntity(render::Transaction& transaction) {
    if (!render::Item::isValidID(_myMetaItem)) {
        return;
    }

    bool sunChanged = _keyLightPropertiesChanged;
    bool backgroundChanged = _backgroundPropertiesChanged;
    bool skyboxChanged = _skyboxPropertiesChanged;

    transaction.updateItem<RenderableZoneEntityItemMeta>(_myMetaItem, [=](RenderableZoneEntityItemMeta& data) {
        
        updateKeyZoneItemFromEntity(data);

        if (sunChanged) {
            updateKeySunFromEntity(data);
        }

        if (sunChanged || skyboxChanged) {
            updateKeyAmbientFromEntity(data);
        }
        if (backgroundChanged || skyboxChanged) {
            updateKeyBackgroundFromEntity(data);
        }
    });
}

void RenderableZoneEntityItem::notifyChangedRenderItem() {
    if (!render::Item::isValidID(_myMetaItem)) {
        return;
    }

    render::Transaction transaction;
    render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
    sceneUpdateRenderItemFromEntity(transaction);
    scene->enqueueTransaction(transaction);
}









namespace render {
    template <> const ItemKey payloadGetKey(const RenderableZoneEntityItemMeta::Pointer& payload) {
        return ItemKey::Builder().withTypeMeta().build();
    }

    template <> const Item::Bound payloadGetBound(const RenderableZoneEntityItemMeta::Pointer& payload) {
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
    template <> void payloadRender(const RenderableZoneEntityItemMeta::Pointer& payload, RenderArgs* args) {
        payload->render(args);
    }
}

RenderableZoneEntityItemMeta::RenderableZoneEntityItemMeta(EntityItemPointer entity) :
    entity(entity),
    _sunLight(std::make_shared<model::Light>()),
    _ambientLight(std::make_shared<model::Light>()),
    _background(std::make_shared<model::SunSkyStage>())
{
    _background->setSkybox(std::make_shared<ProceduralSkybox>());
}


RenderableZoneEntityItemMeta::~RenderableZoneEntityItemMeta() {
    if (_stage) {
        if (!LightStage::isIndexInvalid(_sunIndex)) {
            _stage->removeLight(_sunIndex);
        }
        if (!LightStage::isIndexInvalid(_ambientIndex)) {
            _stage->removeLight(_ambientIndex); 
             
        }
    }

    if (_backgroundStage) {
        if (!BackgroundStage::isIndexInvalid(_backgroundIndex)) {
            _backgroundStage->removeBackground(_backgroundIndex);
        }
    }
}

void RenderableZoneEntityItemMeta::setAmbientURL(const QString& ambientUrl) {
    // nothing change if nothing change
    if (_ambientTextureURL == ambientUrl) {
        return;
    }
    _ambientTextureURL = ambientUrl;

    if (_ambientTextureURL.isEmpty()) {
        _validAmbientTexture = false;
        _pendingAmbientTexture = false;
        _ambientTexture.clear();

        _ambientLight->setAmbientMap(nullptr);
        _ambientLight->setAmbientSpherePreset(gpu::SphericalHarmonics::BREEZEWAY);
    } else {
        _pendingAmbientTexture = true;
        auto textureCache = DependencyManager::get<TextureCache>();
        _ambientTexture = textureCache->getTexture(_ambientTextureURL, image::TextureUsage::CUBE_TEXTURE);
    
        // keep whatever is assigned on the ambient map/sphere until texture is loaded
    }
}

void RenderableZoneEntityItemMeta::updateAmbientMap() {
    if (_pendingAmbientTexture) {
        if (_ambientTexture && _ambientTexture->isLoaded()) {
            _pendingAmbientTexture = false;

            auto texture = _ambientTexture->getGPUTexture();
            if (texture) {
                if (texture->getIrradiance()) {
                    _ambientLight->setAmbientSphere(*texture->getIrradiance());
                } else {
                    _ambientLight->setAmbientSpherePreset(gpu::SphericalHarmonics::BREEZEWAY);
                }
                editAmbientLight()->setAmbientMap(texture);
                _validAmbientTexture = true;
            } else {
                qCDebug(entitiesrenderer) << "Failed to load ambient texture:" << _ambientTexture->getURL();
            }
        }
    }
}


void RenderableZoneEntityItemMeta::setSkyboxURL(const QString& skyboxUrl) {
    // nothing change if nothing change
    if (_skyboxTextureURL == skyboxUrl) {
        return;
    }
    _skyboxTextureURL = skyboxUrl;

    if (_skyboxTextureURL.isEmpty()) {
        _validSkyboxTexture = false;
        _pendingSkyboxTexture = false;
        _skyboxTexture.clear();

        editSkybox()->setCubemap(nullptr);
    } else {
        _pendingSkyboxTexture = true;
        auto textureCache = DependencyManager::get<TextureCache>();
        _skyboxTexture = textureCache->getTexture(_skyboxTextureURL, image::TextureUsage::CUBE_TEXTURE);
    }
}

void RenderableZoneEntityItemMeta::updateSkyboxMap() {
    if (_pendingSkyboxTexture) {
        if (_skyboxTexture && _skyboxTexture->isLoaded()) {
            _pendingSkyboxTexture = false;

            auto texture = _skyboxTexture->getGPUTexture();
            if (texture) {
                editSkybox()->setCubemap(texture);
                _validSkyboxTexture = true;
            } else {
                qCDebug(entitiesrenderer) << "Failed to load Skybox texture:" << _skyboxTexture->getURL();
            }
        }
    }
}

void RenderableZoneEntityItemMeta::setBackgroundMode(BackgroundMode mode) {
    _backgroundMode = mode;
}

void RenderableZoneEntityItemMeta::setSkyboxColor(const glm::vec3& color) {
    editSkybox()->setColor(color);
}

void RenderableZoneEntityItemMeta::setProceduralUserData(QString userData) {
    if (_proceduralUserData != userData) {
        _proceduralUserData = userData;
        std::dynamic_pointer_cast<ProceduralSkybox>(editSkybox())->parse(_proceduralUserData);
    }
}



void RenderableZoneEntityItemMeta::render(RenderArgs* args) {
    if (!_stage) {
        _stage = DependencyManager::get<DeferredLightingEffect>()->getLightStage();
    }

    if (!_backgroundStage) {
        _backgroundStage = DependencyManager::get<DeferredLightingEffect>()->getBackgroundStage();
    }

    { // Sun 
        // Need an update ?
        if (_needSunUpdate) {
            // Do we need to allocate the light in the stage ?
            if (LightStage::isIndexInvalid(_sunIndex)) {
                _sunIndex = _stage->addLight(_sunLight);
            } else {
                _stage->updateLightArrayBuffer(_sunIndex);
            }
            _needSunUpdate = false;
        }
    }

    { // Ambient
        updateAmbientMap();

        // Need an update ?
        if (_needAmbientUpdate) {
            // Do we need to allocate the light in the stage ?
            if (LightStage::isIndexInvalid(_ambientIndex)) {
                _ambientIndex = _stage->addLight(_ambientLight);
            } else {
                _stage->updateLightArrayBuffer(_ambientIndex);
            }
            _needAmbientUpdate = false;
        }
    }

    { // Skybox
        updateSkyboxMap();

        if (_needBackgroundUpdate) {
            if (BackgroundStage::isIndexInvalid(_backgroundIndex)) {
                _backgroundIndex = _backgroundStage->addBackground(_background);
            } else {

            }
            _needBackgroundUpdate = false;
        }
    }

    if (isVisible()) {
        // FInally, push the light visible in the frame
        // THe directional key light for sure
        _stage->_currentFrame.pushSunLight(_sunIndex);

        // The ambient light only if it has a valid texture to render with
        if (_validAmbientTexture || _validSkyboxTexture) {
            _stage->_currentFrame.pushAmbientLight(_ambientIndex);
        }

        // The background only if the mode is not inherit
        if (_backgroundMode != BACKGROUND_MODE_INHERIT) {
            _backgroundStage->_currentFrame.pushBackground(_backgroundIndex);
        }
    }
}
