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
#include <set>

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
    {
        PerformanceTimer perfTimer("getModel");
        getModel(renderer);
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
    if (textures.isEmpty()) {
        return _originalTextures;
    }

    // Legacy: a ,\n-delimited list of filename:"texturepath"
    if (*textures.cbegin() != '{') {
        textures = "{\"" + textures.replace(":\"", "\":\"").replace(",\n", ",\"") + "}";
    }

    QJsonParseError error;
    QJsonDocument texturesJson = QJsonDocument::fromJson(textures.toUtf8(), &error);
    // If textures are invalid, revert to original textures
    if (error.error != QJsonParseError::NoError) {
        qCWarning(entitiesrenderer) << "Could not evaluate textures property value:" << textures;
        return _originalTextures;
    }

    QVariantMap texturesMap = texturesJson.toVariant().toMap();
    // If textures are unset, revert to original textures
    if (texturesMap.isEmpty()) {
        return _originalTextures;
    }

    return texturesJson.toVariant().toMap();
}

void RenderableModelEntityItem::remapTextures() {
    if (!_model) {
        return; // nothing to do if we don't have a model
    }
    
    if (!_model->isLoaded()) {
        return; // nothing to do if the model has not yet loaded
    }

    if (!_originalTexturesRead) {
        _originalTextures = _model->getTextures();
        _originalTexturesRead = true;

        // Default to _originalTextures to avoid remapping immediately and lagging on load
        _currentTextures = _originalTextures;
    }

    auto textures = getTextures();
    if (textures == _lastTextures) {
        return;
    }

    _lastTextures = textures;
    auto newTextures = parseTexturesToMap(textures);

    if (newTextures != _currentTextures) {
        _model->setTextures(newTextures);
        _currentTextures = newTextures;
    }
}

void RenderableModelEntityItem::doInitialModelSimulation() {
    // The machinery for updateModelBounds will give existing models the opportunity to fix their
    // translation/rotation/scale/registration.  The first two are straightforward, but the latter two have guards to
    // make sure they don't happen after they've already been set.  Here we reset those guards. This doesn't cause the
    // entity values to change -- it just allows the model to match once it comes in.
    _model->setScaleToFit(false, getDimensions());
    _model->setSnapModelToRegistrationPoint(false, getRegistrationPoint());

    // now recalculate the bounds and registration
    _model->setScaleToFit(true, getDimensions());
    _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
    _model->setRotation(getRotation());
    _model->setTranslation(getPosition());
    {
        PerformanceTimer perfTimer("_model->simulate");
        _model->simulate(0.0f);
    }
    _needsInitialSimulation = false;
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
        // make sure to simulate so everything gets set up correctly for rendering
        doInitialModelSimulation();
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

    if (!hasRenderAnimation() || !_jointMappingCompleted) {
        return false;
    }

    if (_animation && _animation->isLoaded()) {

        const QVector<FBXAnimationFrame>&  frames = _animation->getFramesReference(); // NOTE: getFrames() is too heavy
        auto& fbxJoints = _animation->getGeometry().joints;

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
    if (!_dimensionsInitialized || !_model->isActive()) {
        return;
    }

    bool movingOrAnimating = isMovingRelativeToParent() || isAnimatingSomething();
    glm::vec3 dimensions = getDimensions();
    bool success;
    auto transform = getTransform(success);

    if (movingOrAnimating ||
         _needsInitialSimulation ||
         _needsJointSimulation ||
         _model->getTranslation() != transform.getTranslation() ||
         _model->getScaleToFitDimensions() != dimensions ||
         _model->getRotation() != transform.getRotation() ||
         _model->getRegistrationPoint() != getRegistrationPoint()) {
        doInitialModelSimulation();
        _needsJointSimulation = false;
    }
}


// NOTE: this only renders the "meta" portion of the Model, namely it renders debugging items, and it handles
// the per frame simulation/update that might be required if the models properties changed.
void RenderableModelEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RMEIrender");
    assert(getType() == EntityTypes::Model);

    if (hasModel()) {
        // Prepare the current frame
        {
            if (!_model || _needsModelReload) {
                // TODO: this getModel() appears to be about 3% of model render time. We should optimize
                PerformanceTimer perfTimer("getModel");
                EntityTreeRenderer* renderer = static_cast<EntityTreeRenderer*>(args->_renderer);
                getModel(renderer);

                // Remap textures immediately after loading to avoid flicker
                remapTextures();
            }

            if (_model) {
                if (hasRenderAnimation()) {
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

        // Enqueue updates for the next frame
        if (_model) {

            render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();

            // FIXME: this seems like it could be optimized if we tracked our last known visible state in
            //        the renderable item. As it stands now the model checks it's visible/invisible state
            //        so most of the time we don't do anything in this function.
            _model->setVisibleInScene(getVisible(), scene);

            // Remap textures for the next frame to avoid flicker
            remapTextures();

            // check to see if when we added our models to the scene they were ready, if they were not ready, then
            // fix them up in the scene
            bool shouldShowCollisionHull = (args->_debugFlags & (int)RenderArgs::RENDER_DEBUG_HULLS) > 0
                && getShapeType() == SHAPE_TYPE_COMPOUND;
            if (_model->needsFixupInScene() || _showCollisionHull != shouldShowCollisionHull) {
                _showCollisionHull = shouldShowCollisionHull;
                render::PendingChanges pendingChanges;

                _model->removeFromScene(scene, pendingChanges);

                render::Item::Status::Getters statusGetters;
                makeEntityItemStatusGetters(getThisPointer(), statusGetters);
                _model->addToScene(scene, pendingChanges, statusGetters, _showCollisionHull);

                scene->enqueuePendingChanges(pendingChanges);
            }

            auto& currentURL = getParsedModelURL();
            if (currentURL != _model->getURL()) {
                // Defer setting the url to the render thread
                getModel(_myRenderer);
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

ModelPointer RenderableModelEntityItem::getModel(EntityTreeRenderer* renderer) {
    if (!renderer) {
        return nullptr;
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

    // If we have a URL, then we will want to end up returning a model...
    if (!getModelURL().isEmpty()) {
        // If we don't have a model, allocate one *immediately*
        if (!_model) {
            _model = _myRenderer->allocateModel(getModelURL(), getCompoundShapeURL());
            _needsInitialSimulation = true;
        // If we need to change URLs, update it *after rendering* (to avoid access violations)
        } else if ((QUrl(getModelURL()) != _model->getURL() || QUrl(getCompoundShapeURL()) != _model->getCollisionURL())) {
            QMetaObject::invokeMethod(_myRenderer, "updateModel", Qt::QueuedConnection,
                Q_ARG(ModelPointer, _model),
                Q_ARG(const QString&, getModelURL()),
                Q_ARG(const QString&, getCompoundShapeURL()));
            _needsInitialSimulation = true;
        }
        // Else we can just return the _model
    // If we have no URL, then we can delete any model we do have...
    } else if (_model) {
        // remove from scene
        render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
        render::PendingChanges pendingChanges;
        _model->removeFromScene(scene, pendingChanges);
        scene->enqueuePendingChanges(pendingChanges);

        // release interest
        _myRenderer->releaseModel(_model);
        _model = nullptr;
        _needsInitialSimulation = true;
    }

    return _model;
}

bool RenderableModelEntityItem::needsToCallUpdate() const {
    return !_dimensionsInitialized || _needsInitialSimulation || ModelEntityItem::needsToCallUpdate();
}

void RenderableModelEntityItem::update(const quint64& now) {
    if (!_dimensionsInitialized && _model && _model->isActive()) {
        if (_model->isLoaded()) {
            EntityItemProperties properties;
            properties.setLastEdited(usecTimestampNow()); // we must set the edit time since we're editing it
            auto extents = _model->getMeshExtents();
            properties.setDimensions(extents.maximum - extents.minimum);
            qCDebug(entitiesrenderer) << "Autoresizing:" << (!getName().isEmpty() ? getName() : getModelURL());
            QMetaObject::invokeMethod(DependencyManager::get<EntityScriptingInterface>().data(), "editEntity",
                                      Qt::QueuedConnection,
                                      Q_ARG(QUuid, getEntityItemID()),
                                      Q_ARG(EntityItemProperties, properties));
        }
    }

    // make a copy of the animation properites
    _renderAnimationProperties = _animationProperties;

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

        if (_model->isLoaded() && _model->isCollisionLoaded()) {
            // we have both URLs AND both geometries AND they are both fully loaded.
            if (_needsInitialSimulation) {
                // the _model's offset will be wrong until _needsInitialSimulation is false
                PerformanceTimer perfTimer("_model->simulate");
                doInitialModelSimulation();
            }

            return true;
        }

        // the model is still being downloaded.
        return false;
    } else if (type >= SHAPE_TYPE_SIMPLE_HULL && type <= SHAPE_TYPE_STATIC_MESH) {
        return (_model && _model->isLoaded());
    }
    return true;
}

void RenderableModelEntityItem::computeShapeInfo(ShapeInfo& info) {
    ShapeType type = getShapeType();
    glm::vec3 dimensions = getDimensions();
    if (type == SHAPE_TYPE_COMPOUND) {
        updateModelBounds();

        // should never fall in here when collision model not fully loaded
        // hence we assert that all geometries exist and are loaded
        assert(_model && _model->isLoaded() && _model->isCollisionLoaded());
        const FBXGeometry& collisionGeometry = _model->getCollisionFBXGeometry();

        ShapeInfo::PointCollection& pointCollection = info.getPointCollection();
        pointCollection.clear();
        uint32_t i = 0;

        // the way OBJ files get read, each section under a "g" line is its own meshPart.  We only expect
        // to find one actual "mesh" (with one or more meshParts in it), but we loop over the meshes, just in case.
        const uint32_t TRIANGLE_STRIDE = 3;
        const uint32_t QUAD_STRIDE = 4;
        foreach (const FBXMesh& mesh, collisionGeometry.meshes) {
            // each meshPart is a convex hull
            foreach (const FBXMeshPart &meshPart, mesh.parts) {
                pointCollection.push_back(QVector<glm::vec3>());
                ShapeInfo::PointList& pointsInPart = pointCollection[i];

                // run through all the triangles and (uniquely) add each point to the hull
                uint32_t numIndices = (uint32_t)meshPart.triangleIndices.size();
                assert(numIndices % TRIANGLE_STRIDE == 0);
                for (uint32_t j = 0; j < numIndices; j += TRIANGLE_STRIDE) {
                    glm::vec3 p0 = mesh.vertices[meshPart.triangleIndices[j]];
                    glm::vec3 p1 = mesh.vertices[meshPart.triangleIndices[j + 1]];
                    glm::vec3 p2 = mesh.vertices[meshPart.triangleIndices[j + 2]];
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
                numIndices = (uint32_t)meshPart.quadIndices.size();
                assert(numIndices % QUAD_STRIDE == 0);
                for (uint32_t j = 0; j < numIndices; j += QUAD_STRIDE) {
                    glm::vec3 p0 = mesh.vertices[meshPart.quadIndices[j]];
                    glm::vec3 p1 = mesh.vertices[meshPart.quadIndices[j + 1]];
                    glm::vec3 p2 = mesh.vertices[meshPart.quadIndices[j + 2]];
                    glm::vec3 p3 = mesh.vertices[meshPart.quadIndices[j + 3]];
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
                    pointCollection.pop_back();
                    continue;
                }
                ++i;
            }
        }

        // We expect that the collision model will have the same units and will be displaced
        // from its origin in the same way the visual model is.  The visual model has
        // been centered and probably scaled.  We take the scaling and offset which were applied
        // to the visual model and apply them to the collision model (without regard for the
        // collision model's extents).

        glm::vec3 scaleToFit = dimensions / _model->getFBXGeometry().getUnscaledMeshExtents().size();
        // multiply each point by scale before handing the point-set off to the physics engine.
        // also determine the extents of the collision model.
        for (int32_t i = 0; i < pointCollection.size(); i++) {
            for (int32_t j = 0; j < pointCollection[i].size(); j++) {
                // compensate for registration
                pointCollection[i][j] += _model->getOffset();
                // scale so the collision points match the model points
                pointCollection[i][j] *= scaleToFit;
            }
        }
        info.setParams(type, dimensions, _compoundShapeURL);
    } else if (type >= SHAPE_TYPE_SIMPLE_HULL && type <= SHAPE_TYPE_STATIC_MESH) {
        // should never fall in here when model not fully loaded
        assert(_model && _model->isLoaded());

        updateModelBounds();
        _model->updateGeometry();

        // compute meshPart local transforms
        QVector<glm::mat4> localTransforms;
        const FBXGeometry& fbxGeometry = _model->getFBXGeometry();
        int numFbxMeshes = fbxGeometry.meshes.size();
        int totalNumVertices = 0;
        for (int i = 0; i < numFbxMeshes; i++) {
            const FBXMesh& mesh = fbxGeometry.meshes.at(i);
            if (mesh.clusters.size() > 0) {
                const FBXCluster& cluster = mesh.clusters.at(0);
                auto jointMatrix = _model->getRig()->getJointTransform(cluster.jointIndex);
                localTransforms.push_back(jointMatrix * cluster.inverseBindMatrix);
            } else {
                glm::mat4 identity;
                localTransforms.push_back(identity);
            }
            totalNumVertices += mesh.vertices.size();
        }
        const int32_t MAX_VERTICES_PER_STATIC_MESH = 1e6;
        if (totalNumVertices > MAX_VERTICES_PER_STATIC_MESH) {
            qWarning() << "model" << getModelURL() << "has too many vertices" << totalNumVertices << "and will collide as a box.";
            info.setParams(SHAPE_TYPE_BOX, 0.5f * dimensions);
            return;
        }

        auto& meshes = _model->getGeometry()->getMeshes();
        int32_t numMeshes = (int32_t)(meshes.size());

        ShapeInfo::PointCollection& pointCollection = info.getPointCollection();
        pointCollection.clear();
        if (type == SHAPE_TYPE_SIMPLE_COMPOUND) {
            pointCollection.resize(numMeshes);
        } else {
            pointCollection.resize(1);
        }

        ShapeInfo::TriangleIndices& triangleIndices = info.getTriangleIndices();
        triangleIndices.clear();

        Extents extents;
        int32_t meshCount = 0;
        int32_t pointListIndex = 0;
        for (auto& mesh : meshes) {
            const gpu::BufferView& vertices = mesh->getVertexBuffer();
            const gpu::BufferView& indices = mesh->getIndexBuffer();
            const gpu::BufferView& parts = mesh->getPartBuffer();

            ShapeInfo::PointList& points = pointCollection[pointListIndex];

            // reserve room
            int32_t sizeToReserve = (int32_t)(vertices.getNumElements());
            if (type == SHAPE_TYPE_SIMPLE_COMPOUND) {
                // a list of points for each mesh
                pointListIndex++;
            } else {
                // only one list of points
                sizeToReserve += (int32_t)((gpu::Size)points.size());
            }
            points.reserve(sizeToReserve);

            // copy points
            uint32_t meshIndexOffset = (uint32_t)points.size();
            const glm::mat4& localTransform = localTransforms[meshCount];
            gpu::BufferView::Iterator<const glm::vec3> vertexItr = vertices.cbegin<const glm::vec3>();
            while (vertexItr != vertices.cend<const glm::vec3>()) {
                glm::vec3 point = extractTranslation(localTransform * glm::translate(*vertexItr));
                points.push_back(point);
                extents.addPoint(point);
                ++vertexItr;
            }

            if (type == SHAPE_TYPE_STATIC_MESH) {
                // copy into triangleIndices
                triangleIndices.reserve((int32_t)((gpu::Size)(triangleIndices.size()) + indices.getNumElements()));
                gpu::BufferView::Iterator<const model::Mesh::Part> partItr = parts.cbegin<const model::Mesh::Part>();
                while (partItr != parts.cend<const model::Mesh::Part>()) {
                    if (partItr->_topology == model::Mesh::TRIANGLES) {
                        assert(partItr->_numIndices % 3 == 0);
                        auto indexItr = indices.cbegin<const gpu::BufferView::Index>() + partItr->_startIndex;
                        auto indexEnd = indexItr + partItr->_numIndices;
                        while (indexItr != indexEnd) {
                            triangleIndices.push_back(*indexItr + meshIndexOffset);
                            ++indexItr;
                        }
                    } else if (partItr->_topology == model::Mesh::TRIANGLE_STRIP) {
                        assert(partItr->_numIndices > 2);
                        uint32_t approxNumIndices = 3 * partItr->_numIndices;
                        if (approxNumIndices > (uint32_t)(triangleIndices.capacity() - triangleIndices.size())) {
                            // we underestimated the final size of triangleIndices so we pre-emptively expand it
                            triangleIndices.reserve(triangleIndices.size() + approxNumIndices);
                        }

                        auto indexItr = indices.cbegin<const gpu::BufferView::Index>() + partItr->_startIndex;
                        auto indexEnd = indexItr + (partItr->_numIndices - 2);

                        // first triangle uses the first three indices
                        triangleIndices.push_back(*(indexItr++) + meshIndexOffset);
                        triangleIndices.push_back(*(indexItr++) + meshIndexOffset);
                        triangleIndices.push_back(*(indexItr++) + meshIndexOffset);

                        // the rest use previous and next index
                        uint32_t triangleCount = 1;
                        while (indexItr != indexEnd) {
                            if ((*indexItr) != model::Mesh::PRIMITIVE_RESTART_INDEX) {
                                if (triangleCount % 2 == 0) {
                                    // even triangles use first two indices in order
                                    triangleIndices.push_back(*(indexItr - 2) + meshIndexOffset);
                                    triangleIndices.push_back(*(indexItr - 1) + meshIndexOffset);
                                } else {
                                    // odd triangles swap order of first two indices
                                    triangleIndices.push_back(*(indexItr - 1) + meshIndexOffset);
                                    triangleIndices.push_back(*(indexItr - 2) + meshIndexOffset);
                                }
                                triangleIndices.push_back(*indexItr + meshIndexOffset);
                                ++triangleCount;
                            }
                            ++indexItr;
                        }
                    }
                    ++partItr;
                }
            } else if (type == SHAPE_TYPE_SIMPLE_COMPOUND) {
                // for each mesh copy unique part indices, separated by special bogus (flag) index values
                gpu::BufferView::Iterator<const model::Mesh::Part> partItr = parts.cbegin<const model::Mesh::Part>();
                while (partItr != parts.cend<const model::Mesh::Part>()) {
                    // collect unique list of indices for this part
                    std::set<int32_t> uniqueIndices;
                    if (partItr->_topology == model::Mesh::TRIANGLES) {
                        assert(partItr->_numIndices % 3 == 0);
                        auto indexItr = indices.cbegin<const gpu::BufferView::Index>() + partItr->_startIndex;
                        auto indexEnd = indexItr + partItr->_numIndices;
                        while (indexItr != indexEnd) {
                            uniqueIndices.insert(*indexItr);
                            ++indexItr;
                        }
                    } else if (partItr->_topology == model::Mesh::TRIANGLE_STRIP) {
                        assert(partItr->_numIndices > 2);
                        auto indexItr = indices.cbegin<const gpu::BufferView::Index>() + partItr->_startIndex;
                        auto indexEnd = indexItr + (partItr->_numIndices - 2);

                        // first triangle uses the first three indices
                        uniqueIndices.insert(*(indexItr++));
                        uniqueIndices.insert(*(indexItr++));
                        uniqueIndices.insert(*(indexItr++));

                        // the rest use previous and next index
                        uint32_t triangleCount = 1;
                        while (indexItr != indexEnd) {
                            if ((*indexItr) != model::Mesh::PRIMITIVE_RESTART_INDEX) {
                                if (triangleCount % 2 == 0) {
                                    // even triangles use first two indices in order
                                    uniqueIndices.insert(*(indexItr - 2));
                                    uniqueIndices.insert(*(indexItr - 1));
                                } else {
                                    // odd triangles swap order of first two indices
                                    uniqueIndices.insert(*(indexItr - 1));
                                    uniqueIndices.insert(*(indexItr - 2));
                                }
                                uniqueIndices.insert(*indexItr);
                                ++triangleCount;
                            }
                            ++indexItr;
                        }
                    }

                    // store uniqueIndices in triangleIndices
                    triangleIndices.reserve(triangleIndices.size() + (int32_t)uniqueIndices.size());
                    for (auto index : uniqueIndices) {
                        triangleIndices.push_back(index);
                    }
                    // flag end of part
                    triangleIndices.push_back(END_OF_MESH_PART);

                    ++partItr;
                }
                // flag end of mesh
                triangleIndices.push_back(END_OF_MESH);
            }
            ++meshCount;
        }

        // scale and shift
        glm::vec3 extentsSize = extents.size();
        glm::vec3 scaleToFit = dimensions / extentsSize;
        for (int32_t i = 0; i < 3; ++i) {
            if (extentsSize[i] < 1.0e-6f) {
                scaleToFit[i] = 1.0f;
            }
        }
        for (auto points : pointCollection) {
            for (int32_t i = 0; i < points.size(); ++i) {
                points[i] = (points[i] * scaleToFit);
            }
        }

        info.setParams(type, 0.5f * dimensions, _modelURL);
    } else {
        ModelEntityItem::computeShapeInfo(info);
        info.setParams(type, 0.5f * dimensions);
        adjustShapeInfoByRegistration(info);
    }
}

bool RenderableModelEntityItem::contains(const glm::vec3& point) const {
    if (EntityItem::contains(point) && _model && _model->isCollisionLoaded()) {
        return _model->getCollisionFBXGeometry().convexHullContains(worldToEntity(point));
    }

    return false;
}

bool RenderableModelEntityItem::shouldBePhysical() const {
    // If we have a model, make sure it hasn't failed to download.
    // If it has, we'll report back that we shouldn't be physical so that physics aren't held waiting for us to be ready.
    if (_model && getShapeType() == SHAPE_TYPE_COMPOUND && _model->didCollisionGeometryRequestFail()) {
        return false;
    } else if (_model && getShapeType() != SHAPE_TYPE_NONE && _model->didVisualGeometryRequestFail()) {
        return false;
    } else {
        return ModelEntityItem::shouldBePhysical();
    }
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
        _jointRotationsExplicitlySet = true;
        resizeJointArrays();
        if (index >= 0 && index < _absoluteJointRotationsInObjectFrame.size() &&
            _absoluteJointRotationsInObjectFrame[index] != rotation) {
            _absoluteJointRotationsInObjectFrame[index] = rotation;
            _absoluteJointRotationsInObjectFrameSet[index] = true;
            _absoluteJointRotationsInObjectFrameDirty[index] = true;
            result = true;
            _needsJointSimulation = true;
        }
    });
    return result;
}

bool RenderableModelEntityItem::setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) {
    bool result = false;
    _jointDataLock.withWriteLock([&] {
        _jointTranslationsExplicitlySet = true;
        resizeJointArrays();
        if (index >= 0 && index < _absoluteJointTranslationsInObjectFrame.size() &&
            _absoluteJointTranslationsInObjectFrame[index] != translation) {
            _absoluteJointTranslationsInObjectFrame[index] = translation;
            _absoluteJointTranslationsInObjectFrameSet[index] = true;
            _absoluteJointTranslationsInObjectFrameDirty[index] = true;
            result = true;
            _needsJointSimulation = true;
        }
    });
    return result;
}

void RenderableModelEntityItem::setJointRotations(const QVector<glm::quat>& rotations) {
    ModelEntityItem::setJointRotations(rotations);
    _needsJointSimulation = true;
}

void RenderableModelEntityItem::setJointRotationsSet(const QVector<bool>& rotationsSet) {
    ModelEntityItem::setJointRotationsSet(rotationsSet);
    _needsJointSimulation = true;
}

void RenderableModelEntityItem::setJointTranslations(const QVector<glm::vec3>& translations) {
    ModelEntityItem::setJointTranslations(translations);
    _needsJointSimulation = true;
}

void RenderableModelEntityItem::setJointTranslationsSet(const QVector<bool>& translationsSet) {
    ModelEntityItem::setJointTranslationsSet(translationsSet);
    _needsJointSimulation = true;
}


void RenderableModelEntityItem::locationChanged(bool tellPhysics) {
    EntityItem::locationChanged(tellPhysics);
    if (_model && _model->isActive()) {
        _model->setRotation(getRotation());
        _model->setTranslation(getPosition());

        void* key = (void*)this;
        std::weak_ptr<RenderableModelEntityItem> weakSelf =
            std::static_pointer_cast<RenderableModelEntityItem>(getThisPointer());

        AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [weakSelf]() {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }

            render::ItemID myMetaItem = self->getMetaRenderItem();

            if (myMetaItem == render::Item::INVALID_ITEM_ID) {
                return;
            }

            render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
            render::PendingChanges pendingChanges;

            pendingChanges.updateItem(myMetaItem);
            scene->enqueuePendingChanges(pendingChanges);
        });
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
