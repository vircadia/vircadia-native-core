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

#include <AbstractViewStateInterface.h>
#include <DeferredLightingEffect.h>
#include <Model.h>
#include <PerfStat.h>
#include <render/Scene.h>

#include "EntityTreeRenderer.h"
#include "EntitiesRendererLogging.h"
#include "RenderableModelEntityItem.h"

EntityItemPointer RenderableModelEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderableModelEntityItem>(entityID, properties);
}

RenderableModelEntityItem::~RenderableModelEntityItem() {
    assert(_myRenderer || !_model); // if we have a model, we need to know our renderer
    if (_myRenderer && _model) {
        _myRenderer->releaseModel(_model);
        _model = NULL;
    }
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
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    QString oldModelURL = getModelURL();
    int bytesRead = ModelEntityItem::readEntitySubclassDataFromBuffer(data, bytesLeftToRead, 
                                                                        args, propertyFlags, overwriteLocalData);
    if (oldModelURL != getModelURL()) {
        _needsModelReload = true;
    }
    return bytesRead;
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
            _originalTexturesRead = true;
        }
    }
    
    if (_currentTextures == _textures) {
        return; // nothing to do if our recently mapped textures match our desired textures
    }
    
    // since we're changing here, we need to run through our current texture map
    // and any textures in the recently mapped texture, that is not in our desired
    // textures, we need to "unset"
    QJsonDocument currentTexturesAsJson = QJsonDocument::fromJson(_currentTextures.toUtf8());
    QJsonObject currentTexturesAsJsonObject = currentTexturesAsJson.object();
    QVariantMap currentTextureMap = currentTexturesAsJsonObject.toVariantMap();

    QJsonDocument texturesAsJson = QJsonDocument::fromJson(_textures.toUtf8());
    QJsonObject texturesAsJsonObject = texturesAsJson.object();
    QVariantMap textureMap = texturesAsJsonObject.toVariantMap();

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
            return payload->entity->getAABox();
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

void makeEntityItemStatusGetters(RenderableModelEntityItem* entity, render::Item::Status::Getters& statusGetters) {
    statusGetters.push_back([entity] () -> render::Item::Status::Value {
        quint64 delta = usecTimestampNow() - entity->getLastEditedFromRemote();
        const float WAIT_THRESHOLD_INV = 1.0f / (0.2f * USECS_PER_SECOND);
        float normalizedDelta = delta * WAIT_THRESHOLD_INV;
        // Status icon will scale from 1.0f down to 0.0f after WAIT_THRESHOLD
        // Color is red if last update is after WAIT_THRESHOLD, green otherwise (120 deg is green)
        return render::Item::Status::Value(1.0f - normalizedDelta, (normalizedDelta > 1.0f ? render::Item::Status::Value::GREEN : render::Item::Status::Value::RED));
    });
    statusGetters.push_back([entity] () -> render::Item::Status::Value {
        quint64 delta = usecTimestampNow() - entity->getLastBroadcast();
        const float WAIT_THRESHOLD_INV = 1.0f / (0.4f * USECS_PER_SECOND);
        float normalizedDelta = delta * WAIT_THRESHOLD_INV;
        // Status icon will scale from 1.0f down to 0.0f after WAIT_THRESHOLD
        // Color is Magenta if last update is after WAIT_THRESHOLD, cyan otherwise (180 deg is green)
        return render::Item::Status::Value(1.0f - normalizedDelta, (normalizedDelta > 1.0f ? render::Item::Status::Value::MAGENTA : render::Item::Status::Value::CYAN));
    });
}

bool RenderableModelEntityItem::addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, 
                                            render::PendingChanges& pendingChanges) {

    _myMetaItem = scene->allocateID();
    
    auto renderData = std::make_shared<RenderableModelEntityItemMeta>(self);
    auto renderPayload = std::make_shared<RenderableModelEntityItemMeta::Payload>(renderData);
    
    pendingChanges.resetItem(_myMetaItem, renderPayload);
    
    if (_model) {
        render::Item::Status::Getters statusGetters;
        makeEntityItemStatusGetters(this, statusGetters);
        
        // note: we don't care if the model fails to add items, we always added our meta item and therefore we return
        // true so that the system knows our meta item is in the scene!
        _model->addToScene(scene, pendingChanges, statusGetters);
    }

    return true;
}
    
void RenderableModelEntityItem::removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, 
                                                render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_myMetaItem);
    if (_model) {
        _model->removeFromScene(scene, pendingChanges);
    }
}


// NOTE: this only renders the "meta" portion of the Model, namely it renders debugging items, and it handles
// the per frame simulation/update that might be required if the models properties changed.
void RenderableModelEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RMEIrender");
    assert(getType() == EntityTypes::Model);
    
    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();

    if (hasModel()) {
        if (_model) {
            if (getModelURL() != _model->getURL().toString()) {
                qDebug() << "Updating model URL: " << getModelURL();
                _model->setURL(getModelURL());
            }

            render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();

            // check to see if when we added our models to the scene they were ready, if they were not ready, then
            // fix them up in the scene
            if (_model->needsFixupInScene()) {
                render::PendingChanges pendingChanges;

                _model->removeFromScene(scene, pendingChanges);

                render::Item::Status::Getters statusGetters;
                makeEntityItemStatusGetters(this, statusGetters);
                _model->addToScene(scene, pendingChanges, statusGetters);

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
                // handle animations..
                if (hasAnimation()) {
                    if (!jointsMapped()) {
                        QStringList modelJointNames = _model->getJointNames();
                        mapJoints(modelJointNames);
                    }

                    if (jointsMapped()) {
                        bool newFrame;
                        auto frameData = getAnimationFrame(newFrame);
                        if (newFrame) {
                            for (int i = 0; i < frameData.size(); i++) {
                                _model->setJointState(i, true, frameData[i]);
                            }
                        }
                    }
                }

                glm::quat rotation = getRotation();
                bool movingOrAnimating = isMoving() || isAnimatingSomething();
                if ((movingOrAnimating || _needsInitialSimulation) && _model->isActive()) {
                    _model->setScaleToFit(true, dimensions);
                    _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
                    _model->setRotation(rotation);
                    _model->setTranslation(position);
                    
                    // make sure to simulate so everything gets set up correctly for rendering
                    {
                        PerformanceTimer perfTimer("_model->simulate");
                        _model->simulate(0.0f);
                    }
                    _needsInitialSimulation = false;
                }
            }
        }
    } else {
        glm::vec4 greenColor(0.0f, 1.0f, 0.0f, 1.0f);
        RenderableDebugableEntityItem::renderBoundingBox(this, args, 0.0f, greenColor);
    }

    RenderableDebugableEntityItem::render(this, args);
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
        if (_model && QUrl(getModelURL()) != _model->getURL()) {
            result = _model = _myRenderer->updateModel(_model, getModelURL(), getCompoundShapeURL());
            _needsInitialSimulation = true;
        } else if (!_model) { // if we don't yet have a model, then we want our renderer to allocate one
            result = _model = _myRenderer->allocateModel(getModelURL(), getCompoundShapeURL());
            _needsInitialSimulation = true;
        } else { // we already have the model we want...
            result = _model;
        }
    } else { // if our desired URL is empty, we may need to delete our existing model
        if (_model) {
            _myRenderer->releaseModel(_model);
            result = _model = NULL;
            _needsInitialSimulation = true;
        }
    }
    
    return result;
}

bool RenderableModelEntityItem::needsToCallUpdate() const {
    return _needsInitialSimulation || ModelEntityItem::needsToCallUpdate();
}

EntityItemProperties RenderableModelEntityItem::getProperties() const {
    EntityItemProperties properties = ModelEntityItem::getProperties(); // get the properties from our base class
    if (_originalTexturesRead) {
        properties.setTextureNames(_originalTextures);
    }
    return properties;
}

bool RenderableModelEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face, 
                         void** intersectedObject, bool precisionPicking) const {
    if (!_model) {
        return true;
    }
    //qCDebug(entitiesrenderer) << "RenderableModelEntityItem::findDetailedRayIntersection() precisionPicking:" << precisionPicking;
    
    QString extraInfo;
    return _model->findRayIntersectionAgainstSubMeshes(origin, direction, distance, face, extraInfo, precisionPicking);
}

void RenderableModelEntityItem::setCompoundShapeURL(const QString& url) {
    ModelEntityItem::setCompoundShapeURL(url);
    if (_model) {
        _model->setCollisionModelURL(QUrl(url));
    }
}

bool RenderableModelEntityItem::isReadyToComputeShape() {
    ShapeType type = getShapeType();
    if (type == SHAPE_TYPE_COMPOUND) {

        if (!_model) {
            return false; // hmm...
        }

        if (_needsInitialSimulation) {
            // the _model's offset will be wrong until _needsInitialSimulation is false
            return false;
        }

        assert(!_model->getCollisionURL().isEmpty());
    
        if (_model->getURL().isEmpty()) {
            // we need a render geometry with a scale to proceed, so give up.
            return false;
        }
    
        const QSharedPointer<NetworkGeometry> collisionNetworkGeometry = _model->getCollisionGeometry();
        const QSharedPointer<NetworkGeometry> renderNetworkGeometry = _model->getGeometry();
    
        if ((collisionNetworkGeometry && collisionNetworkGeometry->isLoaded()) &&
            (renderNetworkGeometry && renderNetworkGeometry->isLoaded())) {
            // we have both URLs AND both geometries AND they are both fully loaded.
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
    } else {
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
                // compensate for registraion
                _points[i][j] += _model->getOffset();
                // scale so the collision points match the model points
                _points[i][j] *= scale;
                box += _points[i][j];
            }
        }

        glm::vec3 collisionModelDimensions = box.getDimensions();
        info.setParams(type, collisionModelDimensions, _compoundShapeURL);
        info.setConvexHulls(_points);
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
