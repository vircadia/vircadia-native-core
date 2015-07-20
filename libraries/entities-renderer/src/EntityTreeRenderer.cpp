//
//  EntityTreeRenderer.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <QEventLoop>
#include <QScriptSyntaxCheckResult>

#include <AbstractScriptingServicesInterface.h>
#include <AbstractViewStateInterface.h>
#include <DeferredLightingEffect.h>
#include <Model.h>
#include <NetworkAccessManager.h>
#include <PerfStat.h>
#include <SceneScriptingInterface.h>
#include <ScriptEngine.h>
#include <TextureCache.h>

#include "EntityTreeRenderer.h"

#include "RenderableEntityItem.h"

#include "RenderableBoxEntityItem.h"
#include "RenderableLightEntityItem.h"
#include "RenderableModelEntityItem.h"
#include "RenderableParticleEffectEntityItem.h"
#include "RenderableSphereEntityItem.h"
#include "RenderableTextEntityItem.h"
#include "RenderableWebEntityItem.h"
#include "RenderableZoneEntityItem.h"
#include "RenderableLineEntityItem.h"
#include "RenderablePolyVoxEntityItem.h"
#include "EntitiesRendererLogging.h"
#include "AddressManager.h"

EntityTreeRenderer::EntityTreeRenderer(bool wantScripts, AbstractViewStateInterface* viewState,
                                            AbstractScriptingServicesInterface* scriptingServices) :
    OctreeRenderer(),
    _wantScripts(wantScripts),
    _entitiesScriptEngine(NULL),
    _sandboxScriptEngine(NULL),
    _lastMouseEventValid(false),
    _viewState(viewState),
    _scriptingServices(scriptingServices),
    _displayElementChildProxies(false),
    _displayModelBounds(false),
    _displayModelElementProxy(false),
    _dontDoPrecisionPicking(false)
{
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Model, RenderableModelEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Box, RenderableBoxEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Sphere, RenderableSphereEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Light, RenderableLightEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Text, RenderableTextEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Web, RenderableWebEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(ParticleEffect, RenderableParticleEffectEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Zone, RenderableZoneEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(Line, RenderableLineEntityItem::factory)
    REGISTER_ENTITY_TYPE_WITH_FACTORY(PolyVox, RenderablePolyVoxEntityItem::factory)
    
    _currentHoverOverEntityID = UNKNOWN_ENTITY_ID;
    _currentClickingOnEntityID = UNKNOWN_ENTITY_ID;
}

EntityTreeRenderer::~EntityTreeRenderer() {
    // NOTE: we don't need to delete _entitiesScriptEngine because it is registered with the application and has a
    // signal tied to call it's deleteLater on doneRunning
    if (_sandboxScriptEngine) {
        // TODO: consider reworking how _sandboxScriptEngine is managed. It's treated differently than _entitiesScriptEngine
        // because we don't call registerScriptEngineWithApplicationServices() for it. This implementation is confusing and
        // potentially error prone because it's not a full fledged ScriptEngine that has been fully connected to the
        // application. We did this so that scripts that were ill-formed could be evaluated but not execute against the
        // application services. But this means it's shutdown behavior is different from other ScriptEngines
        delete _sandboxScriptEngine;
        _sandboxScriptEngine = NULL;
    }
}

void EntityTreeRenderer::clear() {
    leaveAllEntities();
    foreach (const EntityItemID& entityID, _entityScripts.keys()) {
        checkAndCallUnload(entityID);
    }
    _entityScripts.clear();

    auto scene = _viewState->getMain3DScene();
    render::PendingChanges pendingChanges;
    
    foreach(auto entity, _entitiesInScene) {
        entity->removeFromScene(entity, scene, pendingChanges);
    }
    scene->enqueuePendingChanges(pendingChanges);
    _entitiesInScene.clear();

    OctreeRenderer::clear();
}

void EntityTreeRenderer::init() {
    OctreeRenderer::init();
    EntityTree* entityTree = static_cast<EntityTree*>(_tree);
    entityTree->setFBXService(this);

    if (_wantScripts) {
        _entitiesScriptEngine = new ScriptEngine(NO_SCRIPT, "Entities",
                                        _scriptingServices->getControllerScriptingInterface());
        _scriptingServices->registerScriptEngineWithApplicationServices(_entitiesScriptEngine);

        _sandboxScriptEngine = new ScriptEngine(NO_SCRIPT, "Entities Sandbox", NULL);
    }

    // make sure our "last avatar position" is something other than our current position, so that on our
    // first chance, we'll check for enter/leave entity events.
    _lastAvatarPosition = _viewState->getAvatarPosition() + glm::vec3((float)TREE_SCALE);
    
    connect(entityTree, &EntityTree::deletingEntity, this, &EntityTreeRenderer::deletingEntity, Qt::QueuedConnection);
    connect(entityTree, &EntityTree::addingEntity, this, &EntityTreeRenderer::addingEntity, Qt::QueuedConnection);
    connect(entityTree, &EntityTree::entityScriptChanging, this, &EntityTreeRenderer::entitySciptChanging, Qt::QueuedConnection);
}

void EntityTreeRenderer::shutdown() {
    _entitiesScriptEngine->disconnect(); // disconnect all slots/signals from the script engine
    _shuttingDown = true;
}

void EntityTreeRenderer::scriptContentsAvailable(const QUrl& url, const QString& scriptContents) {
    if (_waitingOnPreload.contains(url)) {
        QList<EntityItemID> entityIDs = _waitingOnPreload.values(url);
        _waitingOnPreload.remove(url);
        foreach(EntityItemID entityID, entityIDs) {
            checkAndCallPreload(entityID);
        }
    }
}

void EntityTreeRenderer::errorInLoadingScript(const QUrl& url) {
    if (_waitingOnPreload.contains(url)) {
        _waitingOnPreload.remove(url);
    }
}

QScriptValue EntityTreeRenderer::loadEntityScript(const EntityItemID& entityItemID, bool isPreload, bool reload) {
    EntityItemPointer entity = static_cast<EntityTree*>(_tree)->findEntityByEntityItemID(entityItemID);
    return loadEntityScript(entity, isPreload, reload);
}


QString EntityTreeRenderer::loadScriptContents(const QString& scriptMaybeURLorText, bool& isURL, bool& isPending, QUrl& urlOut,
        bool& reload) {
    isPending = false;
    QUrl url(scriptMaybeURLorText);
    
    // If the url is not valid, this must be script text...
    // We document "direct injection" scripts as starting with "(function...", and that would never be a valid url.
    // But QUrl thinks it is.
    if (!url.isValid() || scriptMaybeURLorText.startsWith("(")) {
        isURL = false;
        return scriptMaybeURLorText;
    }
    isURL = true;
    urlOut = url;

    QString scriptContents; // assume empty
    
    // if the scheme length is one or lower, maybe they typed in a file, let's try
    const int WINDOWS_DRIVE_LETTER_SIZE = 1;
    if (url.scheme().size() <= WINDOWS_DRIVE_LETTER_SIZE) {
        url = QUrl::fromLocalFile(scriptMaybeURLorText);
    }

    // ok, let's see if it's valid... and if so, load it
    if (url.isValid()) {
        if (url.scheme() == "file") {
            QString fileName = url.toLocalFile();
            QFile scriptFile(fileName);
            if (scriptFile.open(QFile::ReadOnly | QFile::Text)) {
                qCDebug(entitiesrenderer) << "Loading file:" << fileName;
                QTextStream in(&scriptFile);
                scriptContents = in.readAll();
            } else {
                qCDebug(entitiesrenderer) << "ERROR Loading file:" << fileName;
            }
        } else {
            auto scriptCache = DependencyManager::get<ScriptCache>();
            
            if (!scriptCache->isInBadScriptList(url)) {
                scriptContents = scriptCache->getScript(url, this, isPending, reload);
            }
        }
    }
    
    return scriptContents;
}


QScriptValue EntityTreeRenderer::loadEntityScript(EntityItemPointer entity, bool isPreload, bool reload) {
    if (_shuttingDown) {
        return QScriptValue(); // since we're shutting down, we don't load any more scripts
    }
    
    if (!entity) {
        return QScriptValue(); // no entity...
    }
    
    // NOTE: we keep local variables for the entityID and the script because
    // below in loadScriptContents() it's possible for us to execute the
    // application event loop, which may cause our entity to be deleted on
    // us. We don't really need access the entity after this point, can
    // can accomplish all we need to here with just the script "text" and the ID.
    EntityItemID entityID = entity->getEntityItemID();
    QString entityScript = entity->getScript();

    if (_entityScripts.contains(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        
        // check to make sure our script text hasn't changed on us since we last loaded it and we're not redownloading it
        if (details.scriptText == entityScript && !reload) {
            return details.scriptObject; // previously loaded
        }
        
        // if we got here, then we previously loaded a script, but the entity's script value
        // has changed and so we need to reload it.
        _entityScripts.remove(entityID);
    }
    if (entityScript.isEmpty()) {
        return QScriptValue(); // no script
    }
    
    bool isURL = false; // loadScriptContents() will tell us if this is a URL or just text.
    bool isPending = false;
    QUrl url;
    QString scriptContents = loadScriptContents(entityScript, isURL, isPending, url, reload);
    
    if (isPending && isPreload && isURL) {
        _waitingOnPreload.insert(url, entityID);
    }

    auto scriptCache = DependencyManager::get<ScriptCache>();

    if (isURL && scriptCache->isInBadScriptList(url)) {
        return QScriptValue(); // no script contents...
    }
    
    if (scriptContents.isEmpty()) {
        return QScriptValue(); // no script contents...
    }
    
    QScriptSyntaxCheckResult syntaxCheck = QScriptEngine::checkSyntax(scriptContents);
    if (syntaxCheck.state() != QScriptSyntaxCheckResult::Valid) {
        qCDebug(entitiesrenderer) << "EntityTreeRenderer::loadEntityScript() entity:" << entityID;
        qCDebug(entitiesrenderer) << "   " << syntaxCheck.errorMessage() << ":"
                          << syntaxCheck.errorLineNumber() << syntaxCheck.errorColumnNumber();
        qCDebug(entitiesrenderer) << "    SCRIPT:" << entityScript;

        scriptCache->addScriptToBadScriptList(url);
        
        return QScriptValue(); // invalid script
    }
    
    if (isURL) {
        _entitiesScriptEngine->setParentURL(entity->getScript());
    }
    QScriptValue entityScriptConstructor = _sandboxScriptEngine->evaluate(scriptContents);
    
    if (!entityScriptConstructor.isFunction()) {
        qCDebug(entitiesrenderer) << "EntityTreeRenderer::loadEntityScript() entity:" << entityID;
        qCDebug(entitiesrenderer) << "    NOT CONSTRUCTOR";
        qCDebug(entitiesrenderer) << "    SCRIPT:" << entityScript;

        scriptCache->addScriptToBadScriptList(url);

        return QScriptValue(); // invalid script
    } else {
        entityScriptConstructor = _entitiesScriptEngine->evaluate(scriptContents);
    }

    QScriptValue entityScriptObject = entityScriptConstructor.construct();
    EntityScriptDetails newDetails = { entityScript, entityScriptObject };
    _entityScripts[entityID] = newDetails;

    if (isURL) {
        _entitiesScriptEngine->setParentURL("");
    }

    return entityScriptObject; // newly constructed
}

QScriptValue EntityTreeRenderer::getPreviouslyLoadedEntityScript(const EntityItemID& entityID) {
    if (_entityScripts.contains(entityID)) {
        EntityScriptDetails details = _entityScripts[entityID];
        return details.scriptObject; // previously loaded
    }
    return QScriptValue(); // no script
}

void EntityTreeRenderer::setTree(Octree* newTree) {
    OctreeRenderer::setTree(newTree);
    static_cast<EntityTree*>(_tree)->setFBXService(this);
}

void EntityTreeRenderer::update() {
    if (_tree && !_shuttingDown) {
        EntityTree* tree = static_cast<EntityTree*>(_tree);
        tree->update();
        
        // check to see if the avatar has moved and if we need to handle enter/leave entity logic
        checkEnterLeaveEntities();

        // Even if we're not moving the mouse, if we started clicking on an entity and we have
        // not yet released the hold then this is still considered a holdingClickOnEntity event
        // and we want to simulate this message here as well as in mouse move
        if (_lastMouseEventValid && !_currentClickingOnEntityID.isInvalidID()) {
            emit holdingClickOnEntity(_currentClickingOnEntityID, _lastMouseEvent);
            QScriptValueList currentClickingEntityArgs = createMouseEventArgs(_currentClickingOnEntityID, _lastMouseEvent);
            QScriptValue currentClickingEntity = loadEntityScript(_currentClickingOnEntityID);
            if (currentClickingEntity.property("holdingClickOnEntity").isValid()) {
                currentClickingEntity.property("holdingClickOnEntity").call(currentClickingEntity, currentClickingEntityArgs);
            }
        }

    }
}

void EntityTreeRenderer::checkEnterLeaveEntities() {
    if (_tree && !_shuttingDown) {
        glm::vec3 avatarPosition = _viewState->getAvatarPosition();
        
        if (avatarPosition != _lastAvatarPosition) {
            float radius = 1.0f; // for now, assume 1 meter radius
            QVector<EntityItemPointer> foundEntities;
            QVector<EntityItemID> entitiesContainingAvatar;
            
            // find the entities near us
            _tree->lockForRead(); // don't let someone else change our tree while we search
            static_cast<EntityTree*>(_tree)->findEntities(avatarPosition, radius, foundEntities);

            // create a list of entities that actually contain the avatar's position
            foreach(EntityItemPointer entity, foundEntities) {
                if (entity->contains(avatarPosition)) {
                    entitiesContainingAvatar << entity->getEntityItemID();
                }
            }
            _tree->unlock();
            
            // Note: at this point we don't need to worry about the tree being locked, because we only deal with
            // EntityItemIDs from here. The loadEntityScript() method is robust against attempting to load scripts
            // for entity IDs that no longer exist.

            // for all of our previous containing entities, if they are no longer containing then send them a leave event
            foreach(const EntityItemID& entityID, _currentEntitiesInside) {
                if (!entitiesContainingAvatar.contains(entityID)) {
                    emit leaveEntity(entityID);
                    QScriptValueList entityArgs = createEntityArgs(entityID);
                    QScriptValue entityScript = loadEntityScript(entityID);
                    if (entityScript.property("leaveEntity").isValid()) {
                        entityScript.property("leaveEntity").call(entityScript, entityArgs);
                    }

                }
            }

            // for all of our new containing entities, if they weren't previously containing then send them an enter event
            foreach(const EntityItemID& entityID, entitiesContainingAvatar) {
                if (!_currentEntitiesInside.contains(entityID)) {
                    emit enterEntity(entityID);
                    QScriptValueList entityArgs = createEntityArgs(entityID);
                    QScriptValue entityScript = loadEntityScript(entityID);
                    if (entityScript.property("enterEntity").isValid()) {
                        entityScript.property("enterEntity").call(entityScript, entityArgs);
                    }
                }
            }
            _currentEntitiesInside = entitiesContainingAvatar;
            _lastAvatarPosition = avatarPosition;
        }
    }
}

void EntityTreeRenderer::leaveAllEntities() {
    if (_tree && !_shuttingDown) {

        // for all of our previous containing entities, if they are no longer containing then send them a leave event
        foreach(const EntityItemID& entityID, _currentEntitiesInside) {
            emit leaveEntity(entityID);
            QScriptValueList entityArgs = createEntityArgs(entityID);
            QScriptValue entityScript = loadEntityScript(entityID);
            if (entityScript.property("leaveEntity").isValid()) {
                entityScript.property("leaveEntity").call(entityScript, entityArgs);
            }
        }
        _currentEntitiesInside.clear();
        
        // make sure our "last avatar position" is something other than our current position, so that on our
        // first chance, we'll check for enter/leave entity events.
        _lastAvatarPosition = _viewState->getAvatarPosition() + glm::vec3((float)TREE_SCALE);
    }
}

void EntityTreeRenderer::applyZonePropertiesToScene(std::shared_ptr<ZoneEntityItem> zone) {
    QSharedPointer<SceneScriptingInterface> scene = DependencyManager::get<SceneScriptingInterface>();
    if (zone) {
        if (!_hasPreviousZone) {
            _previousKeyLightColor = scene->getKeyLightColor();
            _previousKeyLightIntensity = scene->getKeyLightIntensity();
            _previousKeyLightAmbientIntensity = scene->getKeyLightAmbientIntensity();
            _previousKeyLightDirection = scene->getKeyLightDirection();
            _previousStageSunModelEnabled = scene->isStageSunModelEnabled();
            _previousStageLongitude = scene->getStageLocationLongitude();
            _previousStageLatitude = scene->getStageLocationLatitude();
            _previousStageAltitude = scene->getStageLocationAltitude();
            _previousStageHour = scene->getStageDayTime();
            _previousStageDay = scene->getStageYearTime();
            _hasPreviousZone = true;
        }
        scene->setKeyLightColor(zone->getKeyLightColorVec3());
        scene->setKeyLightIntensity(zone->getKeyLightIntensity());
        scene->setKeyLightAmbientIntensity(zone->getKeyLightAmbientIntensity());
        scene->setKeyLightDirection(zone->getKeyLightDirection());
        scene->setStageSunModelEnable(zone->getStageProperties().getSunModelEnabled());
        scene->setStageLocation(zone->getStageProperties().getLongitude(), zone->getStageProperties().getLatitude(),
                                zone->getStageProperties().getAltitude());
        scene->setStageDayTime(zone->getStageProperties().calculateHour());
        scene->setStageYearTime(zone->getStageProperties().calculateDay());
        
        if (zone->getBackgroundMode() == BACKGROUND_MODE_ATMOSPHERE) {
            EnvironmentData data = zone->getEnvironmentData();
            glm::vec3 keyLightDirection = scene->getKeyLightDirection();
            glm::vec3 inverseKeyLightDirection = keyLightDirection * -1.0f;
            
            // NOTE: is this right? It seems like the "sun" should be based on the center of the
            //       atmosphere, not where the camera is.
            glm::vec3 keyLightLocation = _viewState->getAvatarPosition()
            + (inverseKeyLightDirection * data.getAtmosphereOuterRadius());
            
            data.setSunLocation(keyLightLocation);
            
            const float KEY_LIGHT_INTENSITY_TO_SUN_BRIGHTNESS_RATIO = 20.0f;
            float sunBrightness = scene->getKeyLightIntensity() * KEY_LIGHT_INTENSITY_TO_SUN_BRIGHTNESS_RATIO;
            data.setSunBrightness(sunBrightness);
            
            _viewState->overrideEnvironmentData(data);
            scene->getSkyStage()->setBackgroundMode(model::SunSkyStage::SKY_DOME);
            
        } else {
            _viewState->endOverrideEnvironmentData();
            auto stage = scene->getSkyStage();
            if (zone->getBackgroundMode() == BACKGROUND_MODE_SKYBOX) {
                stage->getSkybox()->setColor(zone->getSkyboxProperties().getColorVec3());
                if (zone->getSkyboxProperties().getURL().isEmpty()) {
                    stage->getSkybox()->setCubemap(gpu::TexturePointer());
                } else {
                    // Update the Texture of the Skybox with the one pointed by this zone
                    auto cubeMap = DependencyManager::get<TextureCache>()->getTexture(zone->getSkyboxProperties().getURL(), CUBE_TEXTURE);
                    stage->getSkybox()->setCubemap(cubeMap->getGPUTexture());
                }
                stage->setBackgroundMode(model::SunSkyStage::SKY_BOX);
            } else {
                stage->setBackgroundMode(model::SunSkyStage::SKY_DOME); // let the application atmosphere through
            }
        }
    } else {
        if (_hasPreviousZone) {
            scene->setKeyLightColor(_previousKeyLightColor);
            scene->setKeyLightIntensity(_previousKeyLightIntensity);
            scene->setKeyLightAmbientIntensity(_previousKeyLightAmbientIntensity);
            scene->setKeyLightDirection(_previousKeyLightDirection);
            scene->setStageSunModelEnable(_previousStageSunModelEnabled);
            scene->setStageLocation(_previousStageLongitude, _previousStageLatitude,
                                    _previousStageAltitude);
            scene->setStageDayTime(_previousStageHour);
            scene->setStageYearTime(_previousStageDay);
            _hasPreviousZone = false;
        }
        _viewState->endOverrideEnvironmentData();
        scene->getSkyStage()->setBackgroundMode(model::SunSkyStage::SKY_DOME);  // let the application atmosphere through
    }
}

void EntityTreeRenderer::render(RenderArgs* renderArgs) {

    if (_tree && !_shuttingDown) {
        renderArgs->_renderer = this;

        _tree->lockForRead();

        // Whenever you're in an intersection between zones, we will always choose the smallest zone.
        _bestZone = NULL; // NOTE: Is this what we want?
        _bestZoneVolume = std::numeric_limits<float>::max();
        
        // FIX ME: right now the renderOperation does the following:
        //   1) determining the best zone (not really rendering)
        //   2) render the debug cell details
        // we should clean this up
        _tree->recurseTreeWithOperation(renderOperation, renderArgs);

        applyZonePropertiesToScene(_bestZone);

        _tree->unlock();
    }
    deleteReleasedModels(); // seems like as good as any other place to do some memory cleanup
}

const FBXGeometry* EntityTreeRenderer::getGeometryForEntity(EntityItemPointer entityItem) {
    const FBXGeometry* result = NULL;
    
    if (entityItem->getType() == EntityTypes::Model) {
        std::shared_ptr<RenderableModelEntityItem> modelEntityItem =
                                                        std::dynamic_pointer_cast<RenderableModelEntityItem>(entityItem);
        assert(modelEntityItem); // we need this!!!
        Model* model = modelEntityItem->getModel(this);
        if (model) {
            result = &model->getGeometry()->getFBXGeometry();
        }
    }
    return result;
}

const Model* EntityTreeRenderer::getModelForEntityItem(EntityItemPointer entityItem) {
    const Model* result = NULL;
    if (entityItem->getType() == EntityTypes::Model) {
        std::shared_ptr<RenderableModelEntityItem> modelEntityItem =
                                                        std::dynamic_pointer_cast<RenderableModelEntityItem>(entityItem);
        result = modelEntityItem->getModel(this);
    }
    return result;
}

const FBXGeometry* EntityTreeRenderer::getCollisionGeometryForEntity(EntityItemPointer entityItem) {
    const FBXGeometry* result = NULL;
    
    if (entityItem->getType() == EntityTypes::Model) {
        std::shared_ptr<RenderableModelEntityItem> modelEntityItem =
                                                        std::dynamic_pointer_cast<RenderableModelEntityItem>(entityItem);
        if (modelEntityItem->hasCompoundShapeURL()) {
            Model* model = modelEntityItem->getModel(this);
            if (model) {
                const QSharedPointer<NetworkGeometry> collisionNetworkGeometry = model->getCollisionGeometry();
                if (collisionNetworkGeometry && collisionNetworkGeometry->isLoaded()) {
                    result = &collisionNetworkGeometry->getFBXGeometry();
                }
            }
        }
    }
    return result;
}

void EntityTreeRenderer::renderElementProxy(EntityTreeElement* entityTreeElement, RenderArgs* args) {
    auto deferredLighting = DependencyManager::get<DeferredLightingEffect>();
    Q_ASSERT(args->_batch);
    gpu::Batch& batch = *args->_batch;
    Transform transform;
    
    glm::vec3 elementCenter = entityTreeElement->getAACube().calcCenter();
    float elementSize = entityTreeElement->getScale();
    
    auto drawWireCube = [&](glm::vec3 offset, float size, glm::vec4 color) {
        transform.setTranslation(elementCenter + offset);
        batch.setModelTransform(transform);
        deferredLighting->renderWireCube(batch, size, color);
    };
    
    drawWireCube(glm::vec3(), elementSize, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    if (_displayElementChildProxies) {
        // draw the children
        float halfSize = elementSize / 2.0f;
        float quarterSize = elementSize / 4.0f;
        
        drawWireCube(glm::vec3(-quarterSize, -quarterSize, -quarterSize), halfSize, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        drawWireCube(glm::vec3(quarterSize, -quarterSize, -quarterSize), halfSize, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
        drawWireCube(glm::vec3(-quarterSize, quarterSize, -quarterSize), halfSize, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        drawWireCube(glm::vec3(-quarterSize, -quarterSize, quarterSize), halfSize, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        drawWireCube(glm::vec3(quarterSize, quarterSize, quarterSize), halfSize, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        drawWireCube(glm::vec3(-quarterSize, quarterSize, quarterSize), halfSize, glm::vec4(0.0f, 0.5f, 0.5f, 1.0f));
        drawWireCube(glm::vec3(quarterSize, -quarterSize, quarterSize), halfSize, glm::vec4(0.5f, 0.0f, 0.0f, 1.0f));
        drawWireCube(glm::vec3(quarterSize, quarterSize, -quarterSize), halfSize, glm::vec4(0.0f, 0.5f, 0.0f, 1.0f));
    }
}

void EntityTreeRenderer::renderProxies(EntityItemPointer entity, RenderArgs* args) {
    bool isShadowMode = args->_renderMode == RenderArgs::SHADOW_RENDER_MODE;
    if (!isShadowMode && _displayModelBounds) {
        PerformanceTimer perfTimer("renderProxies");

        AACube maxCube = entity->getMaximumAACube();
        AACube minCube = entity->getMinimumAACube();
        AABox entityBox = entity->getAABox();

        glm::vec3 maxCenter = maxCube.calcCenter();
        glm::vec3 minCenter = minCube.calcCenter();
        glm::vec3 entityBoxCenter = entityBox.calcCenter();
        glm::vec3 entityBoxScale = entityBox.getScale();
        
        auto deferredLighting = DependencyManager::get<DeferredLightingEffect>();
        Q_ASSERT(args->_batch);
        gpu::Batch& batch = *args->_batch;
        Transform transform;

        // draw the max bounding cube
        transform.setTranslation(maxCenter);
        batch.setModelTransform(transform);
        deferredLighting->renderWireCube(batch, maxCube.getScale(), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));

        // draw the min bounding cube
        transform.setTranslation(minCenter);
        batch.setModelTransform(transform);
        deferredLighting->renderWireCube(batch, minCube.getScale(), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        
        // draw the entityBox bounding box
        transform.setTranslation(entityBoxCenter);
        transform.setScale(entityBoxScale);
        batch.setModelTransform(transform);
        deferredLighting->renderWireCube(batch, 1.0f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

        // Rotated bounding box
        batch.setModelTransform(entity->getTransformToCenter());
        deferredLighting->renderWireCube(batch, 1.0f, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    }
}

void EntityTreeRenderer::renderElement(OctreeElement* element, RenderArgs* args) {
    // actually render it here...
    // we need to iterate the actual entityItems of the element
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

    EntityItems& entityItems = entityTreeElement->getEntities();
    

    uint16_t numberOfEntities = entityItems.size();

    bool isShadowMode = args->_renderMode == RenderArgs::SHADOW_RENDER_MODE;

    if (!isShadowMode && _displayModelElementProxy && numberOfEntities > 0) {
        renderElementProxy(entityTreeElement, args);
    }
    
    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItemPointer entityItem = entityItems[i];
        
        if (entityItem->isVisible()) {

            // NOTE: Zone Entities are a special case we handle here...
            if (entityItem->getType() == EntityTypes::Zone) {
                if (entityItem->contains(_viewState->getAvatarPosition())) {
                    float entityVolumeEstimate = entityItem->getVolumeEstimate();
                    if (entityVolumeEstimate < _bestZoneVolume) {
                        _bestZoneVolume = entityVolumeEstimate;
                        _bestZone = std::dynamic_pointer_cast<ZoneEntityItem>(entityItem);
                    } else if (entityVolumeEstimate == _bestZoneVolume) {
                        if (!_bestZone) {
                            _bestZoneVolume = entityVolumeEstimate;
                            _bestZone = std::dynamic_pointer_cast<ZoneEntityItem>(entityItem);
                        } else {
                            // in the case of the volume being equal, we will use the
                            // EntityItemID to deterministically pick one entity over the other
                            if (entityItem->getEntityItemID() < _bestZone->getEntityItemID()) {
                                _bestZoneVolume = entityVolumeEstimate;
                                _bestZone = std::dynamic_pointer_cast<ZoneEntityItem>(entityItem);
                            }
                        }
                    }
                }
            }
        }
    }
}

float EntityTreeRenderer::getSizeScale() const {
    return _viewState->getSizeScale();
}

int EntityTreeRenderer::getBoundaryLevelAdjust() const {
    return _viewState->getBoundaryLevelAdjust();
}


void EntityTreeRenderer::processEraseMessage(NLPacket& packet, const SharedNodePointer& sourceNode) {
    static_cast<EntityTree*>(_tree)->processEraseMessage(packet, sourceNode);
}

Model* EntityTreeRenderer::allocateModel(const QString& url, const QString& collisionUrl) {
    Model* model = NULL;
    // Make sure we only create and delete models on the thread that owns the EntityTreeRenderer
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "allocateModel", Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(Model*, model),
                Q_ARG(const QString&, url));

        return model;
    }
    model = new Model();
    model->init();
    model->setURL(QUrl(url));
    model->setCollisionModelURL(QUrl(collisionUrl));
    return model;
}

Model* EntityTreeRenderer::updateModel(Model* original, const QString& newUrl, const QString& collisionUrl) {
    Model* model = NULL;

    // The caller shouldn't call us if the URL doesn't need to change. But if they
    // do, we just return their original back to them.
    if (!original || (QUrl(newUrl) == original->getURL())) {
        return original;
    }

    // Before we do any creating or deleting, make sure we're on our renderer thread
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "updateModel", Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(Model*, model),
                Q_ARG(Model*, original),
                Q_ARG(const QString&, newUrl));

        return model;
    }

    // at this point we know we need to replace the model, and we know we're on the
    // correct thread, so we can do all our work.
    if (original) {
        delete original; // delete the old model...
    }

    // create the model and correctly initialize it with the new url
    model = new Model();
    model->init();
    model->setURL(QUrl(newUrl));
    model->setCollisionModelURL(QUrl(collisionUrl));
        
    return model;
}

void EntityTreeRenderer::releaseModel(Model* model) {
    // If we're not on the renderer's thread, then remember this model to be deleted later
    if (QThread::currentThread() != thread()) {
        _releasedModels << model;
    } else { // otherwise just delete it right away
        delete model;
    }
}

void EntityTreeRenderer::deleteReleasedModels() {
    if (_releasedModels.size() > 0) {
        foreach(Model* model, _releasedModels) {
            delete model;
        }
        _releasedModels.clear();
    }
}

RayToEntityIntersectionResult EntityTreeRenderer::findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType,
                                                                                    bool precisionPicking) {
    RayToEntityIntersectionResult result;
    if (_tree) {
        EntityTree* entityTree = static_cast<EntityTree*>(_tree);

        OctreeElement* element;
        EntityItemPointer intersectedEntity = NULL;
        result.intersects = entityTree->findRayIntersection(ray.origin, ray.direction, element, result.distance, result.face,
                                                                (void**)&intersectedEntity, lockType, &result.accurate,
                                                                precisionPicking);
        if (result.intersects && intersectedEntity) {
            result.entityID = intersectedEntity->getEntityItemID();
            result.properties = intersectedEntity->getProperties();
            result.intersection = ray.origin + (ray.direction * result.distance);
            result.entity = intersectedEntity;
        }
    }
    return result;
}

void EntityTreeRenderer::connectSignalsToSlots(EntityScriptingInterface* entityScriptingInterface) {
    connect(this, &EntityTreeRenderer::mousePressOnEntity, entityScriptingInterface,
        [=](const RayToEntityIntersectionResult& intersection, const QMouseEvent* event, unsigned int deviceId){
        entityScriptingInterface->mousePressOnEntity(intersection.entityID, MouseEvent(*event, deviceId));
    });
    connect(this, &EntityTreeRenderer::mouseMoveOnEntity, entityScriptingInterface,
        [=](const RayToEntityIntersectionResult& intersection, const QMouseEvent* event, unsigned int deviceId) {
        entityScriptingInterface->mouseMoveOnEntity(intersection.entityID, MouseEvent(*event, deviceId));
    });
    connect(this, &EntityTreeRenderer::mouseReleaseOnEntity, entityScriptingInterface,
        [=](const RayToEntityIntersectionResult& intersection, const QMouseEvent* event, unsigned int deviceId) {
        entityScriptingInterface->mouseReleaseOnEntity(intersection.entityID, MouseEvent(*event, deviceId));
    });

    connect(this, &EntityTreeRenderer::clickDownOnEntity, entityScriptingInterface, &EntityScriptingInterface::clickDownOnEntity);
    connect(this, &EntityTreeRenderer::holdingClickOnEntity, entityScriptingInterface, &EntityScriptingInterface::holdingClickOnEntity);
    connect(this, &EntityTreeRenderer::clickReleaseOnEntity, entityScriptingInterface, &EntityScriptingInterface::clickReleaseOnEntity);

    connect(this, &EntityTreeRenderer::hoverEnterEntity, entityScriptingInterface, &EntityScriptingInterface::hoverEnterEntity);
    connect(this, &EntityTreeRenderer::hoverOverEntity, entityScriptingInterface, &EntityScriptingInterface::hoverOverEntity);
    connect(this, &EntityTreeRenderer::hoverLeaveEntity, entityScriptingInterface, &EntityScriptingInterface::hoverLeaveEntity);

    connect(this, &EntityTreeRenderer::enterEntity, entityScriptingInterface, &EntityScriptingInterface::enterEntity);
    connect(this, &EntityTreeRenderer::leaveEntity, entityScriptingInterface, &EntityScriptingInterface::leaveEntity);
    connect(this, &EntityTreeRenderer::collisionWithEntity, entityScriptingInterface, &EntityScriptingInterface::collisionWithEntity);

    connect(DependencyManager::get<SceneScriptingInterface>().data(), &SceneScriptingInterface::shouldRenderEntitiesChanged, this, &EntityTreeRenderer::updateEntityRenderStatus, Qt::QueuedConnection);
}

QScriptValueList EntityTreeRenderer::createMouseEventArgs(const EntityItemID& entityID, QMouseEvent* event, unsigned int deviceID) {
    QScriptValueList args;
    args << entityID.toScriptValue(_entitiesScriptEngine);
    args << MouseEvent(*event, deviceID).toScriptValue(_entitiesScriptEngine);
    return args;
}

QScriptValueList EntityTreeRenderer::createMouseEventArgs(const EntityItemID& entityID, const MouseEvent& mouseEvent) {
    QScriptValueList args;
    args << entityID.toScriptValue(_entitiesScriptEngine);
    args << mouseEvent.toScriptValue(_entitiesScriptEngine);
    return args;
}


QScriptValueList EntityTreeRenderer::createEntityArgs(const EntityItemID& entityID) {
    QScriptValueList args;
    args << entityID.toScriptValue(_entitiesScriptEngine);
    return args;
}

void EntityTreeRenderer::mousePressEvent(QMouseEvent* event, unsigned int deviceID) {
    // If we don't have a tree, or we're in the process of shutting down, then don't
    // process these events.
    if (!_tree || _shuttingDown) {
        return;
    }
    PerformanceTimer perfTimer("EntityTreeRenderer::mousePressEvent");
    PickRay ray = _viewState->computePickRay(event->x(), event->y());

    bool precisionPicking = !_dontDoPrecisionPicking;
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::Lock, precisionPicking);
    if (rayPickResult.intersects) {
        //qCDebug(entitiesrenderer) << "mousePressEvent over entity:" << rayPickResult.entityID;

        QString urlString = rayPickResult.properties.getHref();
        QUrl url = QUrl(urlString, QUrl::StrictMode);
        if (url.isValid() && !url.isEmpty()){
            DependencyManager::get<AddressManager>()->handleLookupString(urlString);

        }

        emit mousePressOnEntity(rayPickResult, event, deviceID);

        QScriptValueList entityScriptArgs = createMouseEventArgs(rayPickResult.entityID, event, deviceID);
        QScriptValue entityScript = loadEntityScript(rayPickResult.entity);
        if (entityScript.property("mousePressOnEntity").isValid()) {
            entityScript.property("mousePressOnEntity").call(entityScript, entityScriptArgs);
        }
    
        _currentClickingOnEntityID = rayPickResult.entityID;
        emit clickDownOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));
        if (entityScript.property("clickDownOnEntity").isValid()) {
            entityScript.property("clickDownOnEntity").call(entityScript, entityScriptArgs);
        }
    }
    _lastMouseEvent = MouseEvent(*event, deviceID);
    _lastMouseEventValid = true;
}

void EntityTreeRenderer::mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) {
    // If we don't have a tree, or we're in the process of shutting down, then don't
    // process these events.
    if (!_tree || _shuttingDown) {
        return;
    }
    PerformanceTimer perfTimer("EntityTreeRenderer::mouseReleaseEvent");
    PickRay ray = _viewState->computePickRay(event->x(), event->y());
    bool precisionPicking = !_dontDoPrecisionPicking;
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::Lock, precisionPicking);
    if (rayPickResult.intersects) {
        //qCDebug(entitiesrenderer) << "mouseReleaseEvent over entity:" << rayPickResult.entityID;
        emit mouseReleaseOnEntity(rayPickResult, event, deviceID);

        QScriptValueList entityScriptArgs = createMouseEventArgs(rayPickResult.entityID, event, deviceID);
        QScriptValue entityScript = loadEntityScript(rayPickResult.entity);
        if (entityScript.property("mouseReleaseOnEntity").isValid()) {
            entityScript.property("mouseReleaseOnEntity").call(entityScript, entityScriptArgs);
        }
    }

    // Even if we're no longer intersecting with an entity, if we started clicking on it, and now
    // we're releasing the button, then this is considered a clickOn event
    if (!_currentClickingOnEntityID.isInvalidID()) {
        emit clickReleaseOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));

        QScriptValueList currentClickingEntityArgs = createMouseEventArgs(_currentClickingOnEntityID, event, deviceID);
        QScriptValue currentClickingEntity = loadEntityScript(_currentClickingOnEntityID);
        if (currentClickingEntity.property("clickReleaseOnEntity").isValid()) {
            currentClickingEntity.property("clickReleaseOnEntity").call(currentClickingEntity, currentClickingEntityArgs);
        }
    }

    // makes it the unknown ID, we just released so we can't be clicking on anything
    _currentClickingOnEntityID = UNKNOWN_ENTITY_ID;
    _lastMouseEvent = MouseEvent(*event, deviceID);
    _lastMouseEventValid = true;
}

void EntityTreeRenderer::mouseMoveEvent(QMouseEvent* event, unsigned int deviceID) {
    // If we don't have a tree, or we're in the process of shutting down, then don't
    // process these events.
    if (!_tree || _shuttingDown) {
        return;
    }
    PerformanceTimer perfTimer("EntityTreeRenderer::mouseMoveEvent");

    PickRay ray = _viewState->computePickRay(event->x(), event->y());

    bool precisionPicking = false; // for mouse moves we do not do precision picking
    RayToEntityIntersectionResult rayPickResult = findRayIntersectionWorker(ray, Octree::TryLock, precisionPicking);
    if (rayPickResult.intersects) {
        //qCDebug(entitiesrenderer) << "mouseReleaseEvent over entity:" << rayPickResult.entityID;
        QScriptValueList entityScriptArgs = createMouseEventArgs(rayPickResult.entityID, event, deviceID);
        // load the entity script if needed...
        QScriptValue entityScript = loadEntityScript(rayPickResult.entity);
        if (entityScript.property("mouseMoveEvent").isValid()) {
            entityScript.property("mouseMoveEvent").call(entityScript, entityScriptArgs);
        }
        emit mouseMoveOnEntity(rayPickResult, event, deviceID);
        if (entityScript.property("mouseMoveOnEntity").isValid()) {
            entityScript.property("mouseMoveOnEntity").call(entityScript, entityScriptArgs);
        }
    
        // handle the hover logic...
    
        // if we were previously hovering over an entity, and this new entity is not the same as our previous entity
        // then we need to send the hover leave.
        if (!_currentHoverOverEntityID.isInvalidID() && rayPickResult.entityID != _currentHoverOverEntityID) {
            emit hoverLeaveEntity(_currentHoverOverEntityID, MouseEvent(*event, deviceID));

            QScriptValueList currentHoverEntityArgs = createMouseEventArgs(_currentHoverOverEntityID, event, deviceID);

            QScriptValue currentHoverEntity = loadEntityScript(_currentHoverOverEntityID);
            if (currentHoverEntity.property("hoverLeaveEntity").isValid()) {
                currentHoverEntity.property("hoverLeaveEntity").call(currentHoverEntity, currentHoverEntityArgs);
            }
        }

        // If the new hover entity does not match the previous hover entity then we are entering the new one
        // this is true if the _currentHoverOverEntityID is known or unknown
        if (rayPickResult.entityID != _currentHoverOverEntityID) {
            emit hoverEnterEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
            if (entityScript.property("hoverEnterEntity").isValid()) {
                entityScript.property("hoverEnterEntity").call(entityScript, entityScriptArgs);
            }
        }

        // and finally, no matter what, if we're intersecting an entity then we're definitely hovering over it, and
        // we should send our hover over event
        emit hoverOverEntity(rayPickResult.entityID, MouseEvent(*event, deviceID));
        if (entityScript.property("hoverOverEntity").isValid()) {
            entityScript.property("hoverOverEntity").call(entityScript, entityScriptArgs);
        }

        // remember what we're hovering over
        _currentHoverOverEntityID = rayPickResult.entityID;

    } else {
        // handle the hover logic...
        // if we were previously hovering over an entity, and we're no longer hovering over any entity then we need to
        // send the hover leave for our previous entity
        if (!_currentHoverOverEntityID.isInvalidID()) {
            emit hoverLeaveEntity(_currentHoverOverEntityID, MouseEvent(*event, deviceID));

            QScriptValueList currentHoverEntityArgs = createMouseEventArgs(_currentHoverOverEntityID, event, deviceID);

            QScriptValue currentHoverEntity = loadEntityScript(_currentHoverOverEntityID);
            if (currentHoverEntity.property("hoverLeaveEntity").isValid()) {
                currentHoverEntity.property("hoverLeaveEntity").call(currentHoverEntity, currentHoverEntityArgs);
            }

            _currentHoverOverEntityID = UNKNOWN_ENTITY_ID; // makes it the unknown ID
        }
    }

    // Even if we're no longer intersecting with an entity, if we started clicking on an entity and we have
    // not yet released the hold then this is still considered a holdingClickOnEntity event
    if (!_currentClickingOnEntityID.isInvalidID()) {
        emit holdingClickOnEntity(_currentClickingOnEntityID, MouseEvent(*event, deviceID));

        QScriptValueList currentClickingEntityArgs = createMouseEventArgs(_currentClickingOnEntityID, event, deviceID);

        QScriptValue currentClickingEntity = loadEntityScript(_currentClickingOnEntityID);
        if (currentClickingEntity.property("holdingClickOnEntity").isValid()) {
            currentClickingEntity.property("holdingClickOnEntity").call(currentClickingEntity, currentClickingEntityArgs);
        }
    }
    _lastMouseEvent = MouseEvent(*event, deviceID);
    _lastMouseEventValid = true;
}

void EntityTreeRenderer::deletingEntity(const EntityItemID& entityID) {
    if (_tree && !_shuttingDown) {
        checkAndCallUnload(entityID);
    }
    _entityScripts.remove(entityID);

    // here's where we remove the entity payload from the scene
    if (_entitiesInScene.contains(entityID)) {
        auto entity = _entitiesInScene.take(entityID);
        render::PendingChanges pendingChanges;
        auto scene = _viewState->getMain3DScene();
        entity->removeFromScene(entity, scene, pendingChanges);
        scene->enqueuePendingChanges(pendingChanges);
    }
}

void EntityTreeRenderer::addingEntity(const EntityItemID& entityID) {
    checkAndCallPreload(entityID);
    auto entity = static_cast<EntityTree*>(_tree)->findEntityByID(entityID);
    if (entity) {
        addEntityToScene(entity);
    }
}

void EntityTreeRenderer::addEntityToScene(EntityItemPointer entity) {
    // here's where we add the entity payload to the scene
    render::PendingChanges pendingChanges;
    auto scene = _viewState->getMain3DScene();
    if (entity->addToScene(entity, scene, pendingChanges)) {
        _entitiesInScene.insert(entity->getEntityItemID(), entity);
    }
    scene->enqueuePendingChanges(pendingChanges);
}


void EntityTreeRenderer::entitySciptChanging(const EntityItemID& entityID, const bool reload) {
    if (_tree && !_shuttingDown) {
        checkAndCallUnload(entityID);
        checkAndCallPreload(entityID, reload);
    }
}

void EntityTreeRenderer::checkAndCallPreload(const EntityItemID& entityID, const bool reload) {
    if (_tree && !_shuttingDown) {
        // load the entity script if needed...
        QScriptValue entityScript = loadEntityScript(entityID, true, reload); // is preload!
        if (entityScript.property("preload").isValid()) {
            QScriptValueList entityArgs = createEntityArgs(entityID);
            entityScript.property("preload").call(entityScript, entityArgs);
        }
    }
}

void EntityTreeRenderer::checkAndCallUnload(const EntityItemID& entityID) {
    if (_tree && !_shuttingDown) {
        QScriptValue entityScript = getPreviouslyLoadedEntityScript(entityID);
        if (entityScript.property("unload").isValid()) {
            QScriptValueList entityArgs = createEntityArgs(entityID);
            entityScript.property("unload").call(entityScript, entityArgs);
        }
    }
}

void EntityTreeRenderer::playEntityCollisionSound(const QUuid& myNodeID, EntityTree* entityTree, const EntityItemID& id, const Collision& collision) {
    EntityItemPointer entity = entityTree->findEntityByEntityItemID(id);
    if (!entity) {
        return;
    }
    QUuid simulatorID = entity->getSimulatorID();
    if (simulatorID.isNull()) {
        // Can be null if it has never moved since being created or coming out of persistence.
        // However, for there to be a collission, one of the two objects must be moving.
        const EntityItemID& otherID = (id == collision.idA) ? collision.idB : collision.idA;
        EntityItemPointer otherEntity = entityTree->findEntityByEntityItemID(otherID);
        if (!otherEntity) {
            return;
        }
        simulatorID = otherEntity->getSimulatorID();
    }

    if (simulatorID.isNull() || (simulatorID != myNodeID)) {
        return; // Only one injector per simulation, please.
    }
    const QString& collisionSoundURL = entity->getCollisionSoundURL();
    if (collisionSoundURL.isEmpty()) {
        return;
    }
    const float mass = entity->computeMass();
    const float COLLISION_PENETRATION_TO_VELOCITY = 50; // as a subsitute for RELATIVE entity->getVelocity()
    // The collision.penetration is a pretty good indicator of changed velocity AFTER the initial contact,
    // but that first contact depends on exactly where we hit in the physics step.
    // We can get a more consistent initial-contact energy reading by using the changed velocity.
    // Note that velocityChange is not a good indicator for continuing collisions, because it does not distinguish
    // between bounce and sliding along a surface.
    const float linearVelocity = (collision.type == CONTACT_EVENT_TYPE_START) ?
        glm::length(collision.velocityChange) :
        glm::length(collision.penetration) * COLLISION_PENETRATION_TO_VELOCITY;
    const float energy = mass * linearVelocity * linearVelocity / 2.0f;
    const glm::vec3 position = collision.contactPoint;
    const float COLLISION_ENERGY_AT_FULL_VOLUME = (collision.type == CONTACT_EVENT_TYPE_START) ? 150.0f : 5.0f;
    const float COLLISION_MINIMUM_VOLUME = 0.005f;
    const float energyFactorOfFull = fmin(1.0f, energy / COLLISION_ENERGY_AT_FULL_VOLUME);
    if (energyFactorOfFull < COLLISION_MINIMUM_VOLUME) {
        return;
    }
    // Quiet sound aren't really heard at all, so we can compress everything to the range [1-c, 1], if we play it all.
    const float COLLISION_SOUND_COMPRESSION_RANGE = 1.0f; // This section could be removed when the value is 1, but let's see how it goes.
    const float volume = (energyFactorOfFull * COLLISION_SOUND_COMPRESSION_RANGE) + (1.0f - COLLISION_SOUND_COMPRESSION_RANGE);


    // Shift the pitch down by ln(1 + (size / COLLISION_SIZE_FOR_STANDARD_PITCH)) / ln(2)
    const float COLLISION_SIZE_FOR_STANDARD_PITCH = 0.2f;
    const float stretchFactor = log(1.0f + (entity->getMinimumAACube().getLargestDimension() / COLLISION_SIZE_FOR_STANDARD_PITCH)) / log(2);
    AudioInjector::playSound(collisionSoundURL, volume, stretchFactor, position);
}

void EntityTreeRenderer::entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB,
                                                    const Collision& collision) {
    // If we don't have a tree, or we're in the process of shutting down, then don't
    // process these events.
    if (!_tree || _shuttingDown) {
        return;
    }
    // Don't respond to small continuous contacts.
    const float COLLISION_MINUMUM_PENETRATION = 0.002f;
    if ((collision.type != CONTACT_EVENT_TYPE_START) && (glm::length(collision.penetration) < COLLISION_MINUMUM_PENETRATION)) {
        return;
    }

    // See if we should play sounds
    EntityTree* entityTree = static_cast<EntityTree*>(_tree);
    const QUuid& myNodeID = DependencyManager::get<NodeList>()->getSessionUUID();
    playEntityCollisionSound(myNodeID, entityTree, idA, collision);
    playEntityCollisionSound(myNodeID, entityTree, idB, collision);

    // And now the entity scripts
    emit collisionWithEntity(idA, idB, collision);
    QScriptValue entityScriptA = loadEntityScript(idA);
    if (entityScriptA.property("collisionWithEntity").isValid()) {
        QScriptValueList args;
        args << idA.toScriptValue(_entitiesScriptEngine);
        args << idB.toScriptValue(_entitiesScriptEngine);
        args << collisionToScriptValue(_entitiesScriptEngine, collision);
        entityScriptA.property("collisionWithEntity").call(entityScriptA, args);
    }

    emit collisionWithEntity(idB, idA, collision);
    QScriptValue entityScriptB = loadEntityScript(idB);
    if (entityScriptB.property("collisionWithEntity").isValid()) {
        QScriptValueList args;
        args << idB.toScriptValue(_entitiesScriptEngine);
        args << idA.toScriptValue(_entitiesScriptEngine);
        args << collisionToScriptValue(_entitiesScriptEngine, collision);
        entityScriptB.property("collisionWithEntity").call(entityScriptA, args);
    }
}

void EntityTreeRenderer::updateEntityRenderStatus(bool shouldRenderEntities) {
    if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
        for (auto entityID : _entityIDsLastInScene) {
            addingEntity(entityID);
        }
        _entityIDsLastInScene.clear();
    } else {
        _entityIDsLastInScene = _entitiesInScene.keys();
        for (auto entityID : _entityIDsLastInScene) {
            // FIXME - is this really right? do we want to do the deletingEntity() code or just remove from the scene.
            deletingEntity(entityID);
        }
    }
}
