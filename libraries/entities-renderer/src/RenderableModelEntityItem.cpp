//
//  RenderableModelEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <QJsonDocument>
#include <QtCore/QThread>
#include <glm/gtx/transform.hpp>

#include <AbstractViewStateInterface.h>
#include <Model.h>
#include <PerfStat.h>
#include <render/Scene.h>
#include <DependencyManager.h>

#include "EntityTreeRenderer.h"
#include "EntitiesRendererLogging.h"
#include "RenderableEntityItem.h"
#include "RenderableModelEntityItem.h"
#include "RenderableEntityItem.h"

EntityItemPointer RenderableModelEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableModelEntityItem(entityID, properties.getDimensionsInitialized()) };
    entity->setProperties(properties);
    return entity;
}

RenderableModelEntityItem::RenderableModelEntityItem(const EntityItemID& entityItemID, bool dimensionsInitialized) :
    ModelEntityItem(entityItemID),
    _dimensionsInitialized(dimensionsInitialized) {
}

RenderableModelEntityItem::~RenderableModelEntityItem() {
    assert(_myRenderer || !_model); // if we have a model, we need to know our renderer
    if (_myRenderer && _model) {
        _myRenderer->releaseModel(_model);
        _model = NULL;
    }
}

void RenderableModelEntityItem::setModelURL(const QString& url) {
    auto& currentURL = getParsedModelURL();
    ModelEntityItem::setModelURL(url);

    if (currentURL != getParsedModelURL() || !_model) {
        EntityTreePointer tree = getTree();
        if (tree) {
            QMetaObject::invokeMethod(tree.get(), "callLoader", Qt::QueuedConnection, Q_ARG(EntityItemID, getID()));
        }
    }
}

void RenderableModelEntityItem::loader() {
    _needsModelReload = true;
    EntityTreeRenderer* renderer = DependencyManager::get<EntityTreeRenderer>().data();
    assert(renderer);
    if (!_model || _needsModelReload) {
        PerformanceTimer perfTimer("getModel");
        getModel(renderer);
    }
    if (_model) {
        _model->setURL(getParsedModelURL());
        _model->setCollisionModelURL(QUrl(getCompoundShapeURL()));
    }
}

void RenderableModelEntityItem::setDimensions(const glm::vec3& value) {
    _dimensionsInitialized = true;
    ModelEntityItem::setDimensions(value);
}

bool RenderableModelEntityItem::setProperties(const EntityItemProperties& properties) {
    QString oldModelURL = getModelURL();
    bool somethingChanged = ModelEntityItem::setProperties(properties);
    if (somethingChanged && oldModelURL != getModelURL()) {
        _needsModelReload = true;
    }
    return somethingChanged;
}

int RenderableModelEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {
    QString oldModelURL = getModelURL();
    int bytesRead = ModelEntityItem::readEntitySubclassDataFromBuffer(data, bytesLeftToRead, 
                                                                      args, propertyFlags, 
                                                                      overwriteLocalData, somethingChanged);
    if (oldModelURL != getModelURL()) {
        _needsModelReload = true;
    }
    return bytesRead;
}

QVariantMap RenderableModelEntityItem::parseTexturesToMap(QString textures) {
    // If textures are unset, revert to original textures
    if (textures == "") {
        return _originalTexturesMap;
    }

    QString jsonTextures = "{\"" + textures.replace(":\"", "\":\"").replace(",\n", ",\"") + "}";
    QJsonParseError error;
    QJsonDocument texturesAsJson = QJsonDocument::fromJson(jsonTextures.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(entitiesrenderer) << "Could not evaluate textures property value:" << _textures;
    }
    QJsonObject texturesAsJsonObject = texturesAsJson.object();
    return texturesAsJsonObject.toVariantMap();
}

void RenderableModelEntityItem::remapTextures() {
    if (!_model) {
        return; // nothing to do if we don't have a model
    }
    
    if (!_model->isLoaded()) {
        return; // nothing to do if the model has not yet loaded
    }
    
    if (!_originalTexturesRead) {
        const QSharedPointer<NetworkGeometry>& networkGeometry = _model->getGeometry();
        if (networkGeometry) {
            _originalTextures = networkGeometry->getTextureNames();
            _originalTexturesMap = parseTexturesToMap(_originalTextures.join(",\n"));
            _originalTexturesRead = true;
        }
    }
    
    if (_currentTextures == _textures) {
        return; // nothing to do if our recently mapped textures match our desired textures
    }
    
    // since we're changing here, we need to run through our current texture map
    // and any textures in the recently mapped texture, that is not in our desired
    // textures, we need to "unset"
    QVariantMap currentTextureMap = parseTexturesToMap(_currentTextures);
    QVariantMap textureMap = parseTexturesToMap(_textures);

    foreach(const QString& key, currentTextureMap.keys()) {
        // if the desired texture map (what we're setting the textures to) doesn't
        // contain this texture, then remove it by setting the URL to null
        if (!textureMap.contains(key)) {
            QUrl noURL;
            qCDebug(entitiesrenderer) << "Removing texture named" << key << "by replacing it with no URL";
            _model->setTextureWithNameToURL(key, noURL);
        }
    }

    // here's where we remap any textures if needed...
    foreach(const QString& key, textureMap.keys()) {
        QUrl newTextureURL = textureMap[key].toUrl();
        qCDebug(entitiesrenderer) << "Updating texture named" << key << "to texture at URL" << newTextureURL;
        _model->setTextureWithNameToURL(key, newTextureURL);
    }
    
    _currentTextures = _textures;
}

// TODO: we need a solution for changes to the postion/rotation/etc of a model...
// this current code path only addresses that in this setup case... not the changing/moving case
bool RenderableModelEntityItem::readyToAddToScene(RenderArgs* renderArgs) {
    if (!_model && renderArgs) {
        // TODO: this getModel() appears to be about 3% of model render time. We should optimize
        PerformanceTimer perfTimer("getModel");
        EntityTreeRenderer* renderer = static_cast<EntityTreeRenderer*>(renderArgs->_renderer);
        getModel(renderer);
    }
    if (renderArgs && _model && _needsInitialSimulation && _model->isActive() && _model->isLoaded()) {
        _model->setScaleToFit(true, getDimensions());
        _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
        _model->setRotation(getRotation());
        _model->setTranslation(getPosition());
    
        // make sure to simulate so everything gets set up correctly for rendering
        {
            PerformanceTimer perfTimer("_model->simulate");
            _model->simulate(0.0f);
        }
        _needsInitialSimulation = false;

        _model->renderSetup(renderArgs);
    }
    bool ready = !_needsInitialSimulation && _model && _model->readyToAddToScene(renderArgs);
    return ready; 
}

class RenderableModelEntityItemMeta {
public:
    RenderableModelEntityItemMeta(EntityItemPointer entity) : entity(entity){ }
    typedef render::Payload<RenderableModelEntityItemMeta> Payload;
    typedef Payload::DataPointer Pointer;
   
    EntityItemPointer entity;
};

namespace render {
    template <> const ItemKey payloadGetKey(const RenderableModelEntityItemMeta::Pointer& payload) { 
        return ItemKey::Builder::opaqueShape();
    }
    
    template <> const Item::Bound payloadGetBound(const RenderableModelEntityItemMeta::Pointer& payload) { 
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
    template <> void payloadRender(const RenderableModelEntityItemMeta::Pointer& payload, RenderArgs* args) {
        if (args) {
            if (payload && payload->entity) {
                PROFILE_RANGE("MetaModelRender");
                payload->entity->render(args);
            }
        }
    }
}

bool RenderableModelEntityItem::addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, 
                                            render::PendingChanges& pendingChanges) {
    _myMetaItem = scene->allocateID();
    
    auto renderData = std::make_shared<RenderableModelEntityItemMeta>(self);
    auto renderPayload = std::make_shared<RenderableModelEntityItemMeta::Payload>(renderData);
    
    pendingChanges.resetItem(_myMetaItem, renderPayload);
    
    if (_model) {
        render::Item::Status::Getters statusGetters;
        makeEntityItemStatusGetters(getThisPointer(), statusGetters);
        
        // note: we don't care if the model fails to add items, we always added our meta item and therefore we return
        // true so that the system knows our meta item is in the scene!
        _model->addToScene(scene, pendingChanges, statusGetters, _showCollisionHull);
    }

    return true;
}

void RenderableModelEntityItem::removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene,
                                                render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_myMetaItem);
    render::Item::clearID(_myMetaItem);
    if (_model) {
        _model->removeFromScene(scene, pendingChanges);
    }
}

void RenderableModelEntityItem::resizeJointArrays(int newSize) {
    if (newSize < 0) {
        if (_model && _model->isActive() && _model->isLoaded() && !_needsInitialSimulation) {
            newSize = _model->getJointStateCount();
        }
    }
    ModelEntityItem::resizeJointArrays(newSize);
}

bool RenderableModelEntityItem::getAnimationFrame() {
    bool newFrame = false;

    if (!_model || !_model->isActive() || !_model->isLoaded() || _needsInitialSimulation) {
        return false;
    }

    if (!hasAnimation() || !_jointMappingCompleted) {
        return false;
    }
    AnimationPointer myAnimation = getAnimation(_animationProperties.getURL()); // FIXME: this could be optimized
    if (myAnimation && myAnimation->isLoaded()) {

        const QVector<FBXAnimationFrame>&  frames = myAnimation->getFramesReference(); // NOTE: getFrames() is too heavy
        auto& fbxJoints = myAnimation->getGeometry().joints;

        int frameCount = frames.size();
        if (frameCount > 0) {
            int animationCurrentFrame = (int)(glm::floor(getAnimationCurrentFrame())) % frameCount;
            if (animationCurrentFrame < 0 || animationCurrentFrame > frameCount) {
                animationCurrentFrame = 0;
            }

            if (animationCurrentFrame != _lastKnownCurrentFrame) {
                _lastKnownCurrentFrame = animationCurrentFrame;
                newFrame = true;

                resizeJointArrays();
                if (_jointMapping.size() != _model->getJointStateCount()) {
                    qDebug() << "RenderableModelEntityItem::getAnimationFrame -- joint count mismatch"
                             << _jointMapping.size() << _model->getJointStateCount();
                    assert(false);
                    return false;
                }

                const QVector<glm::quat>& rotations = frames[animationCurrentFrame].rotations;
                const QVector<glm::vec3>& translations = frames[animationCurrentFrame].translations;

                for (int j = 0; j < _jointMapping.size(); j++) {
                    int index = _jointMapping[j];
                    if (index >= 0) {
                        glm::mat4 translationMat;
                        if (index < translations.size()) {
                            translationMat = glm::translate(translations[index]);
                        }
                        glm::mat4 rotationMat(glm::mat4::_null);
                        if (index < rotations.size()) {
                            rotationMat = glm::mat4_cast(fbxJoints[index].preRotation * rotations[index] * fbxJoints[index].postRotation);
                        } else {
                            rotationMat = glm::mat4_cast(fbxJoints[index].preRotation * fbxJoints[index].postRotation);
                        }
                        glm::mat4 finalMat = (translationMat * fbxJoints[index].preTransform *
                                              rotationMat * fbxJoints[index].postTransform);
                        _absoluteJointTranslationsInObjectFrame[j] = extractTranslation(finalMat);
                        _absoluteJointTranslationsInObjectFrameSet[j] = true;
                        _absoluteJointTranslationsInObjectFrameDirty[j] = true;

                        _absoluteJointRotationsInObjectFrame[j] = glmExtractRotation(finalMat);

                        _absoluteJointRotationsInObjectFrameSet[j] = true;
                        _absoluteJointRotationsInObjectFrameDirty[j] = true;
                    }
                }
            }
        }
    }

    return newFrame;
}

void RenderableModelEntityItem::updateModelBounds() {
    if (!hasModel() || !_model) {
        return;
    }
    bool movingOrAnimating = isMovingRelativeToParent() || isAnimatingSomething();
    if ((movingOrAnimating ||
         _needsInitialSimulation ||
         _model->getTranslation() != getPosition() ||
         _model->getRotation() != getRotation() ||
         _model->getRegistrationPoint() != getRegistrationPoint())
        && _model->isActive() && _dimensionsInitialized) {
        _model->setScaleToFit(true, getDimensions());
        _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
        _model->setRotation(getRotation());
        _model->setTranslation(getPosition());

        // make sure to simulate so everything gets set up correctly for rendering
        {
            PerformanceTimer perfTimer("_model->simulate");
            _model->simulate(0.0f);
        }

        _needsInitialSimulation = false;
    }
}


// NOTE: this only renders the "meta" portion of the Model, namely it renders debugging items, and it handles
// the per frame simulation/update that might be required if the models properties changed.
void RenderableModelEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RMEIrender");
    assert(getType() == EntityTypes::Model);

    if (hasModel()) {
        if (_model) {
            // check if the URL has changed
            auto& currentURL = getParsedModelURL();
            if (currentURL != _model->getURL()) {
                qCDebug(entitiesrenderer).noquote() << "Updating model URL: " << currentURL.toDisplayString();
                _model->setURL(currentURL);
            }

            render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();

            // check to see if when we added our models to the scene they were ready, if they were not ready, then
            // fix them up in the scene
            bool shouldShowCollisionHull = (args->_debugFlags & (int)RenderArgs::RENDER_DEBUG_HULLS) > 0;
            if (_model->needsFixupInScene() || _showCollisionHull != shouldShowCollisionHull) {
                _showCollisionHull = shouldShowCollisionHull;
                render::PendingChanges pendingChanges;

                _model->removeFromScene(scene, pendingChanges);

                render::Item::Status::Getters statusGetters;
                makeEntityItemStatusGetters(getThisPointer(), statusGetters);
                _model->addToScene(scene, pendingChanges, statusGetters, _showCollisionHull);

                scene->enqueuePendingChanges(pendingChanges);
            }

            // FIXME: this seems like it could be optimized if we tracked our last known visible state in
            //        the renderable item. As it stands now the model checks it's visible/invisible state
            //        so most of the time we don't do anything in this function.
            _model->setVisibleInScene(getVisible(), scene);
        }


        remapTextures();
        {
            // float alpha = getLocalRenderAlpha();

            if (!_model || _needsModelReload) {
                // TODO: this getModel() appears to be about 3% of model render time. We should optimize
                PerformanceTimer perfTimer("getModel");
                EntityTreeRenderer* renderer = static_cast<EntityTreeRenderer*>(args->_renderer);
                getModel(renderer);
            }

            if (_model) {
                if (hasAnimation()) {
                    if (!jointsMapped()) {
                        QStringList modelJointNames = _model->getJointNames();
                        mapJoints(modelJointNames);
                    }
                }

                _jointDataLock.withWriteLock([&] {
                    getAnimationFrame();

                    // relay any inbound joint changes from scripts/animation/network to the model/rig
                    for (int index = 0; index < _absoluteJointRotationsInObjectFrame.size(); index++) {
                        if (_absoluteJointRotationsInObjectFrameDirty[index]) {
                            glm::quat rotation = _absoluteJointRotationsInObjectFrame[index];
                            _model->setJointRotation(index, true, rotation, 1.0f);
                            _absoluteJointRotationsInObjectFrameDirty[index] = false;
                        }
                    }
                    for (int index = 0; index < _absoluteJointTranslationsInObjectFrame.size(); index++) {
                        if (_absoluteJointTranslationsInObjectFrameDirty[index]) {
                            glm::vec3 translation = _absoluteJointTranslationsInObjectFrame[index];
                            _model->setJointTranslation(index, true, translation, 1.0f);
                            _absoluteJointTranslationsInObjectFrameDirty[index] = false;
                        }
                    }
                });
                updateModelBounds();
            }
        }
    } else {
        static glm::vec4 greenColor(0.0f, 1.0f, 0.0f, 1.0f);
        gpu::Batch& batch = *args->_batch;
        bool success;
        auto shapeTransform = getTransformToCenter(success);
        if (success) {
            batch.setModelTransform(shapeTransform); // we want to include the scale as well
            DependencyManager::get<GeometryCache>()->renderWireCubeInstance(batch, greenColor);
        }
    }
}

Model* RenderableModelEntityItem::getModel(EntityTreeRenderer* renderer) {
    Model* result = NULL;

    if (!renderer) {
        return result;
    }

    // make sure our renderer is setup
    if (!_myRenderer) {
        _myRenderer = renderer;
    }
    assert(_myRenderer == renderer); // you should only ever render on one renderer
    
    if (!_myRenderer || QThread::currentThread() != _myRenderer->thread()) {
        return _model;
    }
    
    _needsModelReload = false; // this is the reload

    // if we have a URL, then we will want to end up returning a model...
    if (!getModelURL().isEmpty()) {
    
        // if we have a previously allocated model, but its URL doesn't match
        // then we need to let our renderer update our model for us.
        if (_model && (QUrl(getModelURL()) != _model->getURL() ||
                       QUrl(getCompoundShapeURL()) != _model->getCollisionURL())) {
            result = _model = _myRenderer->updateModel(_model, getModelURL(), getCompoundShapeURL());
            _needsInitialSimulation = true;
        } else if (!_model) { // if we don't yet have a model, then we want our renderer to allocate one
            result = _model = _myRenderer->allocateModel(getModelURL(), getCompoundShapeURL());
            _needsInitialSimulation = true;
        } else { // we already have the model we want...
            result = _model;
        }
    } else if (_model) {
        // remove from scene
        render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
        render::PendingChanges pendingChanges;
        _model->removeFromScene(scene, pendingChanges);
        scene->enqueuePendingChanges(pendingChanges);

        // release interest
        _myRenderer->releaseModel(_model);
        result = _model = NULL;
        _needsInitialSimulation = true;
    }

    return result;
}

bool RenderableModelEntityItem::needsToCallUpdate() const {
    return !_dimensionsInitialized || _needsInitialSimulation || ModelEntityItem::needsToCallUpdate();
}

void RenderableModelEntityItem::update(const quint64& now) {
    if (!_dimensionsInitialized && _model && _model->isActive()) {
        const QSharedPointer<NetworkGeometry> renderNetworkGeometry = _model->getGeometry();
        if (renderNetworkGeometry && renderNetworkGeometry->isLoaded()) {
            EntityItemProperties properties;
            auto extents = _model->getMeshExtents();
            properties.setDimensions(extents.maximum - extents.minimum);
            qCDebug(entitiesrenderer) << "Autoresizing:" << (!getName().isEmpty() ? getName() : getModelURL());
            QMetaObject::invokeMethod(DependencyManager::get<EntityScriptingInterface>().data(), "editEntity",
                                      Qt::QueuedConnection,
                                      Q_ARG(QUuid, getEntityItemID()),
                                      Q_ARG(EntityItemProperties, properties));
        }
    }

    ModelEntityItem::update(now);
}

EntityItemProperties RenderableModelEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = ModelEntityItem::getProperties(desiredProperties); // get the properties from our base class
    if (_originalTexturesRead) {
        properties.setTextureNames(_originalTextures);
    }
    return properties;
}

bool RenderableModelEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction, 
                         bool& keepSearching, OctreeElementPointer& element, float& distance, BoxFace& face, 
                         glm::vec3& surfaceNormal, void** intersectedObject, bool precisionPicking) const {
    if (!_model) {
        return true;
    }
    // qCDebug(entitiesrenderer) << "RenderableModelEntityItem::findDetailedRayIntersection() precisionPicking:"
    //                           << precisionPicking;

    QString extraInfo;
    return _model->findRayIntersectionAgainstSubMeshes(origin, direction, distance,
                                                       face, surfaceNormal, extraInfo, precisionPicking);
}

void RenderableModelEntityItem::setCompoundShapeURL(const QString& url) {
    auto currentCompoundShapeURL = getCompoundShapeURL();
    ModelEntityItem::setCompoundShapeURL(url);

    if (getCompoundShapeURL() != currentCompoundShapeURL || !_model) {
        EntityTreePointer tree = getTree();
        if (tree) {
            QMetaObject::invokeMethod(tree.get(), "callLoader", Qt::QueuedConnection, Q_ARG(EntityItemID, getID()));
        }
    }
}

bool RenderableModelEntityItem::isReadyToComputeShape() {
    ShapeType type = getShapeType();

    if (type == SHAPE_TYPE_COMPOUND) {
        if (!_model || _model->getCollisionURL().isEmpty()) {
            EntityTreePointer tree = getTree();
            if (tree) {
                QMetaObject::invokeMethod(tree.get(), "callLoader", Qt::QueuedConnection, Q_ARG(EntityItemID, getID()));
            }
            return false;
        }

        if (_model->getURL().isEmpty()) {
            // we need a render geometry with a scale to proceed, so give up.
            return false;
        }

        const QSharedPointer<NetworkGeometry> collisionNetworkGeometry = _model->getCollisionGeometry();
        const QSharedPointer<NetworkGeometry> renderNetworkGeometry = _model->getGeometry();

        if ((collisionNetworkGeometry && collisionNetworkGeometry->isLoaded()) &&
            (renderNetworkGeometry && renderNetworkGeometry->isLoaded())) {
            // we have both URLs AND both geometries AND they are both fully loaded.

            if (_needsInitialSimulation) {
                // the _model's offset will be wrong until _needsInitialSimulation is false
                PerformanceTimer perfTimer("_model->simulate");
                _model->simulate(0.0f);
                _needsInitialSimulation = false;
            }

            return true;
        }

        // the model is still being downloaded.
        return false;
    }
    return true;
}

void RenderableModelEntityItem::computeShapeInfo(ShapeInfo& info) {
    ShapeType type = getShapeType();
    if (type != SHAPE_TYPE_COMPOUND) {
        ModelEntityItem::computeShapeInfo(info);
        info.setParams(type, 0.5f * getDimensions());
        adjustShapeInfoByRegistration(info);
    } else {
        updateModelBounds();
        const QSharedPointer<NetworkGeometry> collisionNetworkGeometry = _model->getCollisionGeometry();

        // should never fall in here when collision model not fully loaded
        // hence we assert collisionNetworkGeometry is not NULL
        assert(collisionNetworkGeometry);

        const FBXGeometry& collisionGeometry = collisionNetworkGeometry->getFBXGeometry();
        const QSharedPointer<NetworkGeometry> renderNetworkGeometry = _model->getGeometry();
        const FBXGeometry& renderGeometry = renderNetworkGeometry->getFBXGeometry();

        _points.clear();
        unsigned int i = 0;

        // the way OBJ files get read, each section under a "g" line is its own meshPart.  We only expect
        // to find one actual "mesh" (with one or more meshParts in it), but we loop over the meshes, just in case.
        foreach (const FBXMesh& mesh, collisionGeometry.meshes) {
            // each meshPart is a convex hull
            foreach (const FBXMeshPart &meshPart, mesh.parts) {
                QVector<glm::vec3> pointsInPart;

                // run through all the triangles and (uniquely) add each point to the hull
                unsigned int triangleCount = meshPart.triangleIndices.size() / 3;
                for (unsigned int j = 0; j < triangleCount; j++) {
                    unsigned int p0Index = meshPart.triangleIndices[j*3];
                    unsigned int p1Index = meshPart.triangleIndices[j*3+1];
                    unsigned int p2Index = meshPart.triangleIndices[j*3+2];
                    glm::vec3 p0 = mesh.vertices[p0Index];
                    glm::vec3 p1 = mesh.vertices[p1Index];
                    glm::vec3 p2 = mesh.vertices[p2Index];
                    if (!pointsInPart.contains(p0)) {
                        pointsInPart << p0;
                    }
                    if (!pointsInPart.contains(p1)) {
                        pointsInPart << p1;
                    }
                    if (!pointsInPart.contains(p2)) {
                        pointsInPart << p2;
                    }
                }

                // run through all the quads and (uniquely) add each point to the hull
                unsigned int quadCount = meshPart.quadIndices.size() / 4;
                assert((unsigned int)meshPart.quadIndices.size() == quadCount*4);
                for (unsigned int j = 0; j < quadCount; j++) {
                    unsigned int p0Index = meshPart.quadIndices[j*4];
                    unsigned int p1Index = meshPart.quadIndices[j*4+1];
                    unsigned int p2Index = meshPart.quadIndices[j*4+2];
                    unsigned int p3Index = meshPart.quadIndices[j*4+3];
                    glm::vec3 p0 = mesh.vertices[p0Index];
                    glm::vec3 p1 = mesh.vertices[p1Index];
                    glm::vec3 p2 = mesh.vertices[p2Index];
                    glm::vec3 p3 = mesh.vertices[p3Index];
                    if (!pointsInPart.contains(p0)) {
                        pointsInPart << p0;
                    }
                    if (!pointsInPart.contains(p1)) {
                        pointsInPart << p1;
                    }
                    if (!pointsInPart.contains(p2)) {
                        pointsInPart << p2;
                    }
                    if (!pointsInPart.contains(p3)) {
                        pointsInPart << p3;
                    }
                }

                if (pointsInPart.size() == 0) {
                    qCDebug(entitiesrenderer) << "Warning -- meshPart has no faces";
                    continue;
                }

                // add next convex hull
                QVector<glm::vec3> newMeshPoints;
                _points << newMeshPoints;
                // add points to the new convex hull
                _points[i++] << pointsInPart;
            }
        }

        // We expect that the collision model will have the same units and will be displaced
        // from its origin in the same way the visual model is.  The visual model has
        // been centered and probably scaled.  We take the scaling and offset which were applied
        // to the visual model and apply them to the collision model (without regard for the
        // collision model's extents).

        glm::vec3 scale = getDimensions() / renderGeometry.getUnscaledMeshExtents().size();
        // multiply each point by scale before handing the point-set off to the physics engine.
        // also determine the extents of the collision model.
        AABox box;
        for (int i = 0; i < _points.size(); i++) {
            for (int j = 0; j < _points[i].size(); j++) {
                // compensate for registration
                _points[i][j] += _model->getOffset();
                // scale so the collision points match the model points
                _points[i][j] *= scale;
                // this next subtraction is done so we can give info the offset, which will cause
                // the shape-key to change.
                _points[i][j] -= _model->getOffset();
                box += _points[i][j];
            }
        }

        glm::vec3 collisionModelDimensions = box.getDimensions();
        info.setParams(type, collisionModelDimensions, _compoundShapeURL);
        info.setConvexHulls(_points);
        info.setOffset(_model->getOffset());
    }
}

bool RenderableModelEntityItem::contains(const glm::vec3& point) const {
    if (EntityItem::contains(point) && _model && _model->getCollisionGeometry()) {
        const QSharedPointer<NetworkGeometry> collisionNetworkGeometry = _model->getCollisionGeometry();
        const FBXGeometry& collisionGeometry = collisionNetworkGeometry->getFBXGeometry();
        return collisionGeometry.convexHullContains(worldToEntity(point));
    }

    return false;
}

glm::quat RenderableModelEntityItem::getAbsoluteJointRotationInObjectFrame(int index) const {
    if (_model) {
        glm::quat result;
        if (_model->getAbsoluteJointRotationInRigFrame(index, result)) {
            return result;
        }
    }
    return glm::quat();
}

glm::vec3 RenderableModelEntityItem::getAbsoluteJointTranslationInObjectFrame(int index) const {
    if (_model) {
        glm::vec3 result;
        if (_model->getAbsoluteJointTranslationInRigFrame(index, result)) {
            return result;
        }
    }
    return glm::vec3(0.0f);
}

bool RenderableModelEntityItem::setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) {
    bool result = false;
    _jointDataLock.withWriteLock([&] {
        resizeJointArrays();
        if (index >= 0 && index < _absoluteJointRotationsInObjectFrame.size() &&
            _absoluteJointRotationsInObjectFrame[index] != rotation) {
            _absoluteJointRotationsInObjectFrame[index] = rotation;
            _absoluteJointRotationsInObjectFrameSet[index] = true;
            _absoluteJointRotationsInObjectFrameDirty[index] = true;
            result = true;
        }
    });
    return result;
}

bool RenderableModelEntityItem::setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) {
    bool result = false;
    _jointDataLock.withWriteLock([&] {
        resizeJointArrays();
        if (index >= 0 && index < _absoluteJointTranslationsInObjectFrame.size() &&
            _absoluteJointTranslationsInObjectFrame[index] != translation) {
            _absoluteJointTranslationsInObjectFrame[index] = translation;
            _absoluteJointTranslationsInObjectFrameSet[index] = true;
            _absoluteJointTranslationsInObjectFrameDirty[index] = true;
            result = true;
        }
    });
    return result;
}

void RenderableModelEntityItem::locationChanged() {
    EntityItem::locationChanged();
    if (_model && _model->isActive()) {
        _model->setRotation(getRotation());
        _model->setTranslation(getPosition());
    }
}

int RenderableModelEntityItem::getJointIndex(const QString& name) const {
    if (_model && _model->isActive()) {
        RigPointer rig = _model->getRig();
        return rig->indexOfJoint(name);
    }
    return -1;
}

QStringList RenderableModelEntityItem::getJointNames() const {
    QStringList result;
    if (_model && _model->isActive()) {
        RigPointer rig = _model->getRig();
        int jointCount = rig->getJointStateCount();
        for (int jointIndex = 0; jointIndex < jointCount; jointIndex++) {
            result << rig->nameOfJoint(jointIndex);
        }
    }
    return result;
}
