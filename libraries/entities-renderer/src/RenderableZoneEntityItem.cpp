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

#include <graphics/Stage.h>

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
static const unsigned int SUN_SHADOW_CASCADE_COUNT{ 4 };
static const float SUN_SHADOW_MAX_DISTANCE{ 40.0f };

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
            _sunIndex = INVALID_INDEX;
            _shadowIndex = INVALID_INDEX;
        }
        if (!LightStage::isIndexInvalid(_ambientIndex)) {
            _stage->removeLight(_ambientIndex);
            _ambientIndex = INVALID_INDEX;
        }
    }

    if (_backgroundStage) {
        if (!BackgroundStage::isIndexInvalid(_backgroundIndex)) {
            _backgroundStage->removeBackground(_backgroundIndex);
            _backgroundIndex = INVALID_INDEX;
        }
    }

    if (_hazeStage) {
        if (!HazeStage::isIndexInvalid(_hazeIndex)) {
            _hazeStage->removeHaze(_hazeIndex);
            _hazeIndex = INVALID_INDEX;
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

    if (!_hazeStage) {
        _hazeStage = args->_scene->getStage<HazeStage>();
        assert(_hazeStage);
    }

    { // Sun 
      // Need an update ?
        if (_needSunUpdate) {
            // Do we need to allocate the light in the stage ?
            if (LightStage::isIndexInvalid(_sunIndex)) {
                _sunIndex = _stage->addLight(_sunLight);
                _shadowIndex = _stage->addShadow(_sunIndex, SUN_SHADOW_MAX_DISTANCE, SUN_SHADOW_CASCADE_COUNT);
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
            if (_skyboxMode == COMPONENT_MODE_ENABLED && BackgroundStage::isIndexInvalid(_backgroundIndex)) {
                _backgroundIndex = _backgroundStage->addBackground(_background);
            }
            _needBackgroundUpdate = false;
        }
    }

    {
        if (_needHazeUpdate) {
            if (HazeStage::isIndexInvalid(_hazeIndex)) {
                _hazeIndex = _hazeStage->addHaze(_haze);
            }
            _needHazeUpdate = false;
        }
    }

    if (_visible) {
        // Finally, push the lights visible in the frame
        //
        // If component is disabled then push component off state
        // else if component is enabled then push current state
        // (else mode is inherit, the value from the parent zone will be used
        //
        if (_keyLightMode == COMPONENT_MODE_DISABLED) {
            _stage->_currentFrame.pushSunLight(_stage->getSunOffLight());
        } else if (_keyLightMode == COMPONENT_MODE_ENABLED) {
            _stage->_currentFrame.pushSunLight(_sunIndex);
        }

        if (_skyboxMode == COMPONENT_MODE_DISABLED) {
            _backgroundStage->_currentFrame.pushBackground(INVALID_INDEX);
        } else if (_skyboxMode == COMPONENT_MODE_ENABLED) {
            _backgroundStage->_currentFrame.pushBackground(_backgroundIndex);
        }

        if (_ambientLightMode == COMPONENT_MODE_DISABLED) {
            _stage->_currentFrame.pushAmbientLight(_stage->getAmbientOffLight());
        } else if (_ambientLightMode == COMPONENT_MODE_ENABLED) {
            _stage->_currentFrame.pushAmbientLight(_ambientIndex);
        }

        // Haze only if the mode is not inherit, as the model deals with on/off
        if (_hazeMode != COMPONENT_MODE_INHERIT) {
            _hazeStage->_currentFrame.pushHaze(_hazeIndex);
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
    bool keyLightChanged = entity->keyLightPropertiesChanged();
    bool ambientLightChanged = entity->ambientLightPropertiesChanged();
    bool skyboxChanged = entity->skyboxPropertiesChanged();
    bool hazeChanged = entity->hazePropertiesChanged();

    entity->resetRenderingPropertiesChanged();
    _lastPosition = entity->getWorldPosition();
    _lastRotation = entity->getWorldOrientation();
    _lastDimensions = entity->getScaledDimensions();

    _keyLightProperties = entity->getKeyLightProperties();
    _ambientLightProperties = entity->getAmbientLightProperties();
    _skyboxProperties = entity->getSkyboxProperties();
    _hazeProperties = entity->getHazeProperties();

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

    if (keyLightChanged) {
        updateKeySunFromEntity(entity);
    }

    if (ambientLightChanged) {
        updateAmbientLightFromEntity(entity);
    }

    if (skyboxChanged) {
        updateKeyBackgroundFromEntity(entity);
    }

    if (hazeChanged) {
        updateHazeFromEntity(entity);
    }
}

void ZoneEntityRenderer::doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) {
    if (entity->getShapeType() == SHAPE_TYPE_SPHERE) {
        _renderTransform = getModelTransform();
        _renderTransform.postScale(SPHERE_ENTITY_SCALE);
    }
}


ItemKey ZoneEntityRenderer::getKey() {
    return ItemKey::Builder().withTypeMeta().withTagBits(render::ItemKey::TAG_BITS_0 | render::ItemKey::TAG_BITS_1).build();
}

bool ZoneEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    if (entity->keyLightPropertiesChanged() ||
        entity->ambientLightPropertiesChanged() ||
        entity->hazePropertiesChanged() ||
        entity->skyboxPropertiesChanged()) {

        return true;
    }

    if (_skyboxTextureURL != entity->getSkyboxProperties().getURL()) {
        return true;
    }

    if (entity->getWorldPosition() != _lastPosition) {
        return true;
    }
    if (entity->getScaledDimensions() != _lastDimensions) {
        return true;
    }
    if (entity->getWorldOrientation() != _lastRotation) {
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

void ZoneEntityRenderer::updateKeySunFromEntity(const TypedEntityPointer& entity) {
    setKeyLightMode((ComponentMode)entity->getKeyLightMode());

    const auto& sunLight = editSunLight();
    sunLight->setType(graphics::Light::SUN);
    sunLight->setPosition(_lastPosition);
    sunLight->setOrientation(_lastRotation);

    // Set the keylight
    sunLight->setColor(ColorUtils::toVec3(_keyLightProperties.getColor()));
    sunLight->setIntensity(_keyLightProperties.getIntensity());
    sunLight->setDirection(_keyLightProperties.getDirection());
}

void ZoneEntityRenderer::updateAmbientLightFromEntity(const TypedEntityPointer& entity) {
    setAmbientLightMode((ComponentMode)entity->getAmbientLightMode());

    const auto& ambientLight = editAmbientLight();
    ambientLight->setType(graphics::Light::AMBIENT);
    ambientLight->setPosition(_lastPosition);
    ambientLight->setOrientation(_lastRotation);


    // Set the ambient light
    ambientLight->setAmbientIntensity(_ambientLightProperties.getAmbientIntensity());

    if (_ambientLightProperties.getAmbientURL().isEmpty()) {
        setAmbientURL(_skyboxProperties.getURL());
    } else {
        setAmbientURL(_ambientLightProperties.getAmbientURL());
    }
}

void ZoneEntityRenderer::updateHazeFromEntity(const TypedEntityPointer& entity) {
    setHazeMode((ComponentMode)entity->getHazeMode());

    const auto& haze = editHaze();

    const uint32_t hazeMode = entity->getHazeMode();
    haze->setHazeActive(hazeMode == COMPONENT_MODE_ENABLED);
    haze->setAltitudeBased(_hazeProperties.getHazeAltitudeEffect());

    haze->setHazeRangeFactor(graphics::Haze::convertHazeRangeToHazeRangeFactor(_hazeProperties.getHazeRange()));
    xColor hazeColor = _hazeProperties.getHazeColor();
    haze->setHazeColor(glm::vec3(hazeColor.red / 255.0, hazeColor.green / 255.0, hazeColor.blue / 255.0));
    xColor hazeGlareColor = _hazeProperties.getHazeGlareColor();
    haze->setHazeGlareColor(glm::vec3(hazeGlareColor.red / 255.0, hazeGlareColor.green / 255.0, hazeGlareColor.blue / 255.0));
    haze->setHazeEnableGlare(_hazeProperties.getHazeEnableGlare());
    haze->setHazeGlareBlend(graphics::Haze::convertGlareAngleToPower(_hazeProperties.getHazeGlareAngle()));

    float hazeAltitude = _hazeProperties.getHazeCeiling() - _hazeProperties.getHazeBaseRef();
    haze->setHazeAltitudeFactor(graphics::Haze::convertHazeAltitudeToHazeAltitudeFactor(hazeAltitude));
    haze->setHazeBaseReference(_hazeProperties.getHazeBaseRef());

    haze->setHazeBackgroundBlend(_hazeProperties.getHazeBackgroundBlend());

    haze->setHazeAttenuateKeyLight(_hazeProperties.getHazeAttenuateKeyLight());
    haze->setHazeKeyLightRangeFactor(graphics::Haze::convertHazeRangeToHazeRangeFactor(_hazeProperties.getHazeKeyLightRange()));
    haze->setHazeKeyLightAltitudeFactor(graphics::Haze::convertHazeAltitudeToHazeAltitudeFactor(_hazeProperties.getHazeKeyLightAltitude()));

    haze->setZoneTransform(entity->getTransform().getMatrix());
}

void ZoneEntityRenderer::updateKeyBackgroundFromEntity(const TypedEntityPointer& entity) {
    setSkyboxMode((ComponentMode)entity->getSkyboxMode());

    editBackground();
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
            } else {
                qCDebug(entitiesrenderer) << "Failed to load Skybox texture:" << _skyboxTexture->getURL();
            }
        }
    }
}

void ZoneEntityRenderer::setHazeMode(ComponentMode mode) {
    _hazeMode = mode;
}

void ZoneEntityRenderer::setKeyLightMode(ComponentMode mode) {
    _keyLightMode = mode;
}

void ZoneEntityRenderer::setAmbientLightMode(ComponentMode mode) {
    _ambientLightMode = mode;
}

void ZoneEntityRenderer::setSkyboxMode(ComponentMode mode) {
    _skyboxMode = mode;
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
