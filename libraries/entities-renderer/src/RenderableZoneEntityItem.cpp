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

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <PerfStat.h>
#include <procedural/ProceduralSkybox.h>
#include <LightPayload.h>
#include <DeferredLightingEffect.h>

#include "EntityTreeRenderer.h"

// Sphere entities should fit inside a cube entity of the same size, so a sphere that has dimensions 1x1x1
// is a half unit sphere.  However, the geometry cache renders a UNIT sphere, so we need to scale down.
static const float SPHERE_ENTITY_SCALE = 0.5f;

using namespace render;
using namespace render::entities;

ZoneEntityRenderer::ZoneEntityRenderer(const EntityItemPointer& entity)
    : Parent(entity) {
    _background->setSkybox(std::make_shared<ProceduralSkybox>());
}

void ZoneEntityRenderer::onRemoveFromSceneTyped(const TypedEntityPointer& entity) {
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

void ZoneEntityRenderer::doRender(RenderArgs* args) {
#if 0
    if (ZoneEntityItem::getDrawZoneBoundaries()) {
        switch (_entity->getShapeType()) {
            case SHAPE_TYPE_BOX:
            case SHAPE_TYPE_SPHERE:
                {
                    PerformanceTimer perfTimer("zone->renderPrimitive");
                    static const glm::vec4 DEFAULT_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
                    if (!updateModelTransform()) {
                        break;
                    }
                    auto geometryCache = DependencyManager::get<GeometryCache>();
                    gpu::Batch& batch = *args->_batch;
                    batch.setModelTransform(_modelTransform);
                    if (_entity->getShapeType() == SHAPE_TYPE_SPHERE) {
                        geometryCache->renderWireSphereInstance(args, batch, DEFAULT_COLOR);
                    } else {
                        geometryCache->renderWireCubeInstance(args, batch, DEFAULT_COLOR);
                    }
                }
                break;

            // Compund shapes are handled by the _model member
            case SHAPE_TYPE_COMPOUND:
            default:
                // Not handled
                break;
        }
    }
#endif
    if (!_stage) {
        _stage = args->_scene->getStage<LightStage>();
        assert(_stage);
    }

    if (!_backgroundStage) {
        _backgroundStage = args->_scene->getStage<BackgroundStage>();
        assert(_backgroundStage);
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

    if (_visible) {
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

void ZoneEntityRenderer::removeFromScene(const ScenePointer& scene, Transaction& transaction) {
#if 0
    if (_model) {
        _model->removeFromScene(scene, transaction);
    }
#endif
    Parent::removeFromScene(scene, transaction);
}


void ZoneEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {
    DependencyManager::get<EntityTreeRenderer>()->updateZone(entity->getID());

    // FIXME one of the bools here could become true between being fetched and being reset, 
    // resulting in a lost update
    bool sunChanged = entity->keyLightPropertiesChanged();
    bool backgroundChanged = entity->backgroundPropertiesChanged();
    bool skyboxChanged = entity->skyboxPropertiesChanged();
    entity->resetRenderingPropertiesChanged();
    _lastPosition = entity->getPosition();
    _lastRotation = entity->getRotation();
    _lastDimensions = entity->getDimensions();

    _keyLightProperties = entity->getKeyLightProperties();
    _stageProperties = entity->getStageProperties();
    _skyboxProperties = entity->getSkyboxProperties();


#if 0
    if (_lastShapeURL != _typedEntity->getCompoundShapeURL()) {
        _lastShapeURL = _typedEntity->getCompoundShapeURL();
        _model.reset();
        _model = std::make_shared<Model>();
        _model->setIsWireframe(true);
        _model->init();
        _model->setURL(_lastShapeURL);
    }

    if (_model && _model->isActive()) {
        _model->setScaleToFit(true, _lastDimensions);
        _model->setSnapModelToRegistrationPoint(true, _entity->getRegistrationPoint());
        _model->setRotation(_lastRotation);
        _model->setTranslation(_lastPosition);
        _model->simulate(0.0f);
    }
#endif

    updateKeyZoneItemFromEntity();

    if (sunChanged) {
        updateKeySunFromEntity();
    }

    if (sunChanged || skyboxChanged) {
        updateKeyAmbientFromEntity();
    }
    if (backgroundChanged || skyboxChanged) {
        updateKeyBackgroundFromEntity(entity);
    }
}

void ZoneEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    if (entity->getShapeType() == SHAPE_TYPE_SPHERE) {
        _modelTransform.postScale(SPHERE_ENTITY_SCALE);
    }
}


ItemKey ZoneEntityRenderer::getKey() {
    return ItemKey::Builder().withTypeMeta().build();
}

bool ZoneEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (entity->keyLightPropertiesChanged() ||
        entity->backgroundPropertiesChanged() ||
        entity->skyboxPropertiesChanged()) {
        return true;
    }

    if (_skyboxTextureURL != entity->getSkyboxProperties().getURL()) {
        return true;
    }

    if (entity->getPosition() != _lastPosition) {
        return true;
    }
    if (entity->getDimensions() != _lastDimensions) {
        return true;
    }
    if (entity->getRotation() != _lastRotation) {
        return true;
    }

#if 0
    if (_typedEntity->getCompoundShapeURL() != _lastShapeURL) {
        return true;
    }

    if (_model) {
        if (!_model->needsFixupInScene() && (!ZoneEntityItem::getDrawZoneBoundaries() || _entity->getShapeType() != SHAPE_TYPE_COMPOUND)) {
            return true;
        }

        if (_model->needsFixupInScene() && (ZoneEntityItem::getDrawZoneBoundaries() || _entity->getShapeType() == SHAPE_TYPE_COMPOUND)) {
            return true;
        }

        if (_lastModelActive != _model->isActive()) {
            return true;
        }
    }
#endif

    return false;
}

void ZoneEntityRenderer::updateKeySunFromEntity() {
    const auto& sunLight = editSunLight();
    sunLight->setType(model::Light::SUN);
    sunLight->setPosition(_lastPosition);
    sunLight->setOrientation(_lastRotation);

    // Set the keylight
    sunLight->setColor(ColorUtils::toVec3(_keyLightProperties.getColor()));
    sunLight->setIntensity(_keyLightProperties.getIntensity());
    sunLight->setDirection(_keyLightProperties.getDirection());
}

void ZoneEntityRenderer::updateKeyAmbientFromEntity() {
    const auto& ambientLight = editAmbientLight();
    ambientLight->setType(model::Light::AMBIENT);
    ambientLight->setPosition(_lastPosition);
    ambientLight->setOrientation(_lastRotation);


    // Set the keylight
    ambientLight->setAmbientIntensity(_keyLightProperties.getAmbientIntensity());

    if (_keyLightProperties.getAmbientURL().isEmpty()) {
        setAmbientURL(_skyboxProperties.getURL());
    } else {
        setAmbientURL(_keyLightProperties.getAmbientURL());
    }
}

void ZoneEntityRenderer::updateKeyBackgroundFromEntity(const TypedEntityPointer& entity) {
    editBackground();
    setBackgroundMode(entity->getBackgroundMode());
    setSkyboxColor(_skyboxProperties.getColorVec3());
    setProceduralUserData(entity->getUserData());
    setSkyboxURL(_skyboxProperties.getURL());
}

void ZoneEntityRenderer::updateKeyZoneItemFromEntity() {
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

void ZoneEntityRenderer::setAmbientURL(const QString& ambientUrl) {
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

void ZoneEntityRenderer::updateAmbientMap() {
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

void ZoneEntityRenderer::setSkyboxURL(const QString& skyboxUrl) {
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

void ZoneEntityRenderer::updateSkyboxMap() {
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

void ZoneEntityRenderer::setBackgroundMode(BackgroundMode mode) {
    _backgroundMode = mode;
}

void ZoneEntityRenderer::setSkyboxColor(const glm::vec3& color) {
    editSkybox()->setColor(color);
}

void ZoneEntityRenderer::setProceduralUserData(const QString& userData) {
    if (_proceduralUserData != userData) {
        _proceduralUserData = userData;
        std::dynamic_pointer_cast<ProceduralSkybox>(editSkybox())->parse(_proceduralUserData);
    }
}

#if 0
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

void RenderableZoneEntityItem::notifyBoundChanged() {
    notifyChangedRenderItem();
}

#endif
