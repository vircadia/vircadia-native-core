//
//  Model.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Model.h"

#include <QMetaType>
#include <QRunnable>
#include <QThreadPool>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include <shared/QtHelpers.h>
#include <GeometryUtil.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <ViewFrustum.h>
#include <GLMHelpers.h>
#include <model-networking/SimpleMeshProxy.h>

#include "AbstractViewStateInterface.h"
#include "MeshPartPayload.h"

#include "RenderUtilsLogging.h"
#include <Trace.h>

using namespace std;

int nakedModelPointerTypeId = qRegisterMetaType<ModelPointer>();
int weakGeometryResourceBridgePointerTypeId = qRegisterMetaType<Geometry::WeakPointer >();
int vec3VectorTypeId = qRegisterMetaType<QVector<glm::vec3> >();
float Model::FAKE_DIMENSION_PLACEHOLDER = -1.0f;
#define HTTP_INVALID_COM "http://invalid.com"

const int NUM_COLLISION_HULL_COLORS = 24;
std::vector<model::MaterialPointer> _collisionMaterials;

void initCollisionMaterials() {
    // generates bright colors in red, green, blue, yellow, magenta, and cyan spectrums
    // (no browns, greys, or dark shades)
    float component[NUM_COLLISION_HULL_COLORS] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.2f, 0.4f, 0.6f, 0.8f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.8f, 0.6f, 0.4f, 0.2f
    };
    _collisionMaterials.reserve(NUM_COLLISION_HULL_COLORS);

    // each component gets the same cuve
    // but offset by a multiple of one third the full width
    int numComponents = 3;
    int sectionWidth = NUM_COLLISION_HULL_COLORS / numComponents;
    int greenPhase = sectionWidth;
    int bluePhase = 2 * sectionWidth;

    // we stride through the colors to scatter adjacent shades
    // so they don't tend to group together for large models
    for (int i = 0; i < sectionWidth; ++i) {
        for (int j = 0; j < numComponents; ++j) {
            model::MaterialPointer material;
            material = std::make_shared<model::Material>();
            int index = j * sectionWidth + i;
            float red = component[index];
            float green = component[(index + greenPhase) % NUM_COLLISION_HULL_COLORS];
            float blue = component[(index + bluePhase) % NUM_COLLISION_HULL_COLORS];
            material->setAlbedo(glm::vec3(red, green, blue));
            material->setMetallic(0.02f);
            material->setRoughness(0.5f);
            _collisionMaterials.push_back(material);
        }
    }
}

Model::Model(QObject* parent, SpatiallyNestable* spatiallyNestableOverride) :
    QObject(parent),
    _renderGeometry(),
    _collisionGeometry(),
    _renderWatcher(_renderGeometry),
    _spatiallyNestableOverride(spatiallyNestableOverride),
    _translation(0.0f),
    _rotation(),
    _scale(1.0f, 1.0f, 1.0f),
    _scaleToFit(false),
    _scaleToFitDimensions(1.0f),
    _scaledToFit(false),
    _snapModelToRegistrationPoint(false),
    _snappedToRegistrationPoint(false),
    _url(HTTP_INVALID_COM),
    _isVisible(true),
    _blendNumber(0),
    _appliedBlendNumber(0),
    _isWireframe(false)
{
    // we may have been created in the network thread, but we live in the main thread
    if (_viewState) {
        moveToThread(_viewState->getMainThread());
    }

    setSnapModelToRegistrationPoint(true, glm::vec3(0.5f));

    connect(&_renderWatcher, &GeometryResourceWatcher::finished, this, &Model::loadURLFinished);
}

Model::~Model() {
    deleteGeometry();
}

AbstractViewStateInterface* Model::_viewState = NULL;

bool Model::needsFixupInScene() const {
    return (_needsFixupInScene || !_addedToScene) && !_needsReload && isLoaded();
}

// TODO?: should we combine translation and rotation into single method to avoid double-work?
// (figure out where we call these)
void Model::setTranslation(const glm::vec3& translation) {
    _translation = translation;
    updateRenderItems();
}

void Model::setRotation(const glm::quat& rotation) {
    _rotation = rotation;
    updateRenderItems();
}

Transform Model::getTransform() const {
    if (_spatiallyNestableOverride) {
        bool success;
        Transform transform = _spatiallyNestableOverride->getTransform(success);
        if (success) {
            transform.setScale(getScale());
            return transform;
        }
    }

    Transform transform;
    transform.setScale(getScale());
    transform.setTranslation(getTranslation());
    transform.setRotation(getRotation());
    return transform;
}

void Model::setScale(const glm::vec3& scale) {
    setScaleInternal(scale);
    // if anyone sets scale manually, then we are no longer scaled to fit
    _scaleToFit = false;
    _scaledToFit = false;
}

const float SCALE_CHANGE_EPSILON = 0.01f;

void Model::setScaleInternal(const glm::vec3& scale) {
    if (glm::distance(_scale, scale) > SCALE_CHANGE_EPSILON) {
        _scale = scale;
        if (_scale.x == 0.0f || _scale.y == 0.0f || _scale.z == 0.0f) {
            assert(false);
        }
        simulate(0.0f, true);
    }
}

void Model::setOffset(const glm::vec3& offset) {
    _offset = offset;

    // if someone manually sets our offset, then we are no longer snapped to center
    _snapModelToRegistrationPoint = false;
    _snappedToRegistrationPoint = false;
}

void Model::calculateTextureInfo() {
    if (!_hasCalculatedTextureInfo && isLoaded() && getGeometry()->areTexturesLoaded() && !_modelMeshRenderItemsMap.isEmpty()) {
        size_t textureSize = 0;
        int textureCount = 0;
        bool allTexturesLoaded = true;
        foreach(auto renderItem, _modelMeshRenderItems) {
            auto meshPart = renderItem.get();
            textureSize += meshPart->getMaterialTextureSize();
            textureCount += meshPart->getMaterialTextureCount();
            allTexturesLoaded = allTexturesLoaded & meshPart->hasTextureInfo();
        }
        _renderInfoTextureSize = textureSize;
        _renderInfoTextureCount = textureCount;
        _hasCalculatedTextureInfo = allTexturesLoaded; // only do this once
    }
}

size_t Model::getRenderInfoTextureSize() {
    calculateTextureInfo();
    return _renderInfoTextureSize;
}

int Model::getRenderInfoTextureCount() {
    calculateTextureInfo();
    return _renderInfoTextureCount;
}

void Model::updateRenderItems() {
    if (!_addedToScene) {
        return;
    }

    glm::vec3 scale = getScale();
    if (_collisionGeometry) {
        // _collisionGeometry is already scaled
        scale = glm::vec3(1.0f);
    }
    _needsUpdateClusterMatrices = true;
    _renderItemsNeedUpdate = false;

    // queue up this work for later processing, at the end of update and just before rendering.
    // the application will ensure only the last lambda is actually invoked.
    void* key = (void*)this;
    std::weak_ptr<Model> weakSelf = shared_from_this();
    AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [weakSelf, scale]() {

        // do nothing, if the model has already been destroyed.
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        // lazy update of cluster matrices used for rendering.
        // We need to update them here so we can correctly update the bounding box.
        self->updateClusterMatrices();

        uint32_t deleteGeometryCounter = self->_deleteGeometryCounter;

        render::Transaction transaction;
        foreach (auto itemID, self->_modelMeshRenderItemsMap.keys()) {
            transaction.updateItem<ModelMeshPartPayload>(itemID, [deleteGeometryCounter](ModelMeshPartPayload& data) {
                ModelPointer model = data._model.lock();
                if (model && model->isLoaded()) {
                    // Ensure the model geometry was not reset between frames
                    if (deleteGeometryCounter == model->_deleteGeometryCounter) {
                        Transform modelTransform = model->getTransform();
                        modelTransform.setScale(glm::vec3(1.0f));

                        const Model::MeshState& state = model->getMeshState(data._meshIndex);
                        Transform renderTransform = modelTransform;
                        if (state.clusterMatrices.size() == 1) {
                            renderTransform = modelTransform.worldTransform(Transform(state.clusterMatrices[0]));
                        }
                        data.updateTransformForSkinnedMesh(renderTransform, modelTransform, state.clusterBuffer);
                    }
                }
            });
        }

        // collision mesh does not share the same unit scale as the FBX file's mesh: only apply offset
        Transform collisionMeshOffset;
        collisionMeshOffset.setIdentity();
        Transform modelTransform = self->getTransform();
        foreach(auto itemID, self->_collisionRenderItemsMap.keys()) {
            transaction.updateItem<MeshPartPayload>(itemID, [modelTransform, collisionMeshOffset](MeshPartPayload& data) {
                // update the model transform for this render item.
                data.updateTransform(modelTransform, collisionMeshOffset);
            });
        }

        AbstractViewStateInterface::instance()->getMain3DScene()->enqueueTransaction(transaction);
    });
}

void Model::initJointTransforms() {
    if (isLoaded()) {
        glm::mat4 modelOffset = glm::scale(_scale) * glm::translate(_offset);
        _rig.setModelOffset(modelOffset);
    }
}

void Model::init() {
}

void Model::reset() {
    if (isLoaded()) {
        const FBXGeometry& geometry = getFBXGeometry();
        _rig.reset(geometry);
    }
}

bool Model::updateGeometry() {
    bool needFullUpdate = false;

    if (!isLoaded()) {
        return false;
    }

    _needsReload = false;

    // TODO: should all Models have a valid _rig?
    if (_rig.jointStatesEmpty() && getFBXGeometry().joints.size() > 0) {
        initJointStates();
        assert(_meshStates.empty());

        const FBXGeometry& fbxGeometry = getFBXGeometry();
        foreach (const FBXMesh& mesh, fbxGeometry.meshes) {
            MeshState state;
            state.clusterMatrices.resize(mesh.clusters.size());
            _meshStates.append(state);

            // Note: we add empty buffers for meshes that lack blendshapes so we can access the buffers by index
            // later in ModelMeshPayload, however the vast majority of meshes will not have them.
            // TODO? make _blendedVertexBuffers a map instead of vector and only add for meshes with blendshapes?
            auto buffer = std::make_shared<gpu::Buffer>();
            if (!mesh.blendshapes.isEmpty()) {
                buffer->resize((mesh.vertices.size() + mesh.normals.size()) * sizeof(glm::vec3));
                buffer->setSubData(0, mesh.vertices.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.vertices.constData());
                buffer->setSubData(mesh.vertices.size() * sizeof(glm::vec3),
                                   mesh.normals.size() * sizeof(glm::vec3), (gpu::Byte*) mesh.normals.constData());
            }
            _blendedVertexBuffers.push_back(buffer);
        }
        needFullUpdate = true;
    }
    return needFullUpdate;
}

// virtual
void Model::initJointStates() {
    const FBXGeometry& geometry = getFBXGeometry();
    glm::mat4 modelOffset = glm::scale(_scale) * glm::translate(_offset);

    _rig.initJointStates(geometry, modelOffset);
}

bool Model::findRayIntersectionAgainstSubMeshes(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                                    BoxFace& face, glm::vec3& surfaceNormal,
                                                    QString& extraInfo, bool pickAgainstTriangles, bool allowBackface) {

    bool intersectedSomething = false;

    // if we aren't active, we can't ray pick yet...
    if (!isActive()) {
        return intersectedSomething;
    }

    // extents is the entity relative, scaled, centered extents of the entity
    glm::vec3 position = _translation;
    glm::mat4 rotation = glm::mat4_cast(_rotation);
    glm::mat4 translation = glm::translate(position);
    glm::mat4 modelToWorldMatrix = translation * rotation;
    glm::mat4 worldToModelMatrix = glm::inverse(modelToWorldMatrix);

    Extents modelExtents = getMeshExtents(); // NOTE: unrotated

    glm::vec3 dimensions = modelExtents.maximum - modelExtents.minimum;
    glm::vec3 corner = -(dimensions * _registrationPoint); // since we're going to do the ray picking in the model frame of reference
    AABox modelFrameBox(corner, dimensions);

    glm::vec3 modelFrameOrigin = glm::vec3(worldToModelMatrix * glm::vec4(origin, 1.0f));
    glm::vec3 modelFrameDirection = glm::vec3(worldToModelMatrix * glm::vec4(direction, 0.0f));

    // we can use the AABox's ray intersection by mapping our origin and direction into the model frame
    // and testing intersection there.
    if (modelFrameBox.findRayIntersection(modelFrameOrigin, modelFrameDirection, distance, face, surfaceNormal)) {
        QMutexLocker locker(&_mutex);

        float bestDistance = std::numeric_limits<float>::max();
        int subMeshIndex = 0;
        const FBXGeometry& geometry = getFBXGeometry();

        if (!_triangleSetsValid) {
            calculateTriangleSets();
        }

        glm::mat4 meshToModelMatrix = glm::scale(_scale) * glm::translate(_offset);
        glm::mat4 meshToWorldMatrix = createMatFromQuatAndPos(_rotation, _translation) * meshToModelMatrix;
        glm::mat4 worldToMeshMatrix = glm::inverse(meshToWorldMatrix);

        glm::vec3 meshFrameOrigin = glm::vec3(worldToMeshMatrix * glm::vec4(origin, 1.0f));
        glm::vec3 meshFrameDirection = glm::vec3(worldToMeshMatrix * glm::vec4(direction, 0.0f));

        for (auto& triangleSet : _modelSpaceMeshTriangleSets) {
            float triangleSetDistance = 0.0f;
            BoxFace triangleSetFace;
            glm::vec3 triangleSetNormal;
            if (triangleSet.findRayIntersection(meshFrameOrigin, meshFrameDirection, triangleSetDistance, triangleSetFace, triangleSetNormal, pickAgainstTriangles, allowBackface)) {

                glm::vec3 meshIntersectionPoint = meshFrameOrigin + (meshFrameDirection * triangleSetDistance);
                glm::vec3 worldIntersectionPoint = glm::vec3(meshToWorldMatrix * glm::vec4(meshIntersectionPoint, 1.0f));
                float worldDistance = glm::distance(origin, worldIntersectionPoint);

                if (worldDistance < bestDistance) {
                    bestDistance = worldDistance;
                    intersectedSomething = true;
                    face = triangleSetFace;
                    surfaceNormal = glm::vec3(meshToWorldMatrix * glm::vec4(triangleSetNormal, 0.0f));
                    extraInfo = geometry.getModelNameOfMesh(subMeshIndex);
                }
            }
            subMeshIndex++;
        }

        if (intersectedSomething) {
            distance = bestDistance;
        }

        return intersectedSomething;
    }

    return intersectedSomething;
}

bool Model::convexHullContains(glm::vec3 point) {
    // if we aren't active, we can't compute that yet...
    if (!isActive()) {
        return false;
    }

    // extents is the entity relative, scaled, centered extents of the entity
    glm::vec3 position = _translation;
    glm::mat4 rotation = glm::mat4_cast(_rotation);
    glm::mat4 translation = glm::translate(position);
    glm::mat4 modelToWorldMatrix = translation * rotation;
    glm::mat4 worldToModelMatrix = glm::inverse(modelToWorldMatrix);

    Extents modelExtents = getMeshExtents(); // NOTE: unrotated

    glm::vec3 dimensions = modelExtents.maximum - modelExtents.minimum;
    glm::vec3 corner = -(dimensions * _registrationPoint);
    AABox modelFrameBox(corner, dimensions);

    glm::vec3 modelFramePoint = glm::vec3(worldToModelMatrix * glm::vec4(point, 1.0f));

    // we can use the AABox's contains() by mapping our point into the model frame
    // and testing there.
    if (modelFrameBox.contains(modelFramePoint)){
        QMutexLocker locker(&_mutex);

        if (!_triangleSetsValid) {
            calculateTriangleSets();
        }

        // If we are inside the models box, then consider the submeshes...
        glm::mat4 meshToModelMatrix = glm::scale(_scale) * glm::translate(_offset);
        glm::mat4 meshToWorldMatrix = createMatFromQuatAndPos(_rotation, _translation) * meshToModelMatrix;
        glm::mat4 worldToMeshMatrix = glm::inverse(meshToWorldMatrix);
        glm::vec3 meshFramePoint = glm::vec3(worldToMeshMatrix * glm::vec4(point, 1.0f));

        for (const auto& triangleSet : _modelSpaceMeshTriangleSets) {
            const AABox& box = triangleSet.getBounds();
            if (box.contains(meshFramePoint)) {
                if (triangleSet.convexHullContains(meshFramePoint)) {
                    // It's inside this mesh, return true.
                    return true;
                }
            }
        }


    }
    // It wasn't in any mesh, return false.
    return false;
}

MeshProxyList Model::getMeshes() const {
    MeshProxyList result;
    const Geometry::Pointer& renderGeometry = getGeometry();
    const Geometry::GeometryMeshes& meshes = renderGeometry->getMeshes();

    if (!isLoaded()) {
        return result;
    }

    Transform offset;
    offset.setScale(_scale);
    offset.postTranslate(_offset);
    glm::mat4 offsetMat = offset.getMatrix();

    for (std::shared_ptr<const model::Mesh> mesh : meshes) {
        if (!mesh) {
            continue;
        }

        MeshProxy* meshProxy = new SimpleMeshProxy(
            mesh->map(
                [=](glm::vec3 position) {
                    return glm::vec3(offsetMat * glm::vec4(position, 1.0f));
                },
                [=](glm::vec3 color) { return color; },
                [=](glm::vec3 normal) {
                    return glm::normalize(glm::vec3(offsetMat * glm::vec4(normal, 0.0f)));
                },
                [&](uint32_t index) { return index; }));
        result << meshProxy;
    }

    return result;
}

void Model::calculateTriangleSets() {
    PROFILE_RANGE(render, __FUNCTION__);

    const FBXGeometry& geometry = getFBXGeometry();
    int numberOfMeshes = geometry.meshes.size();

    _triangleSetsValid = true;
    _modelSpaceMeshTriangleSets.clear();
    _modelSpaceMeshTriangleSets.resize(numberOfMeshes);

    for (int i = 0; i < numberOfMeshes; i++) {
        const FBXMesh& mesh = geometry.meshes.at(i);

        for (int j = 0; j < mesh.parts.size(); j++) {
            const FBXMeshPart& part = mesh.parts.at(j);

            const int INDICES_PER_TRIANGLE = 3;
            const int INDICES_PER_QUAD = 4;
            const int TRIANGLES_PER_QUAD = 2;

            // tell our triangleSet how many triangles to expect.
            int numberOfQuads = part.quadIndices.size() / INDICES_PER_QUAD;
            int numberOfTris = part.triangleIndices.size() / INDICES_PER_TRIANGLE;
            int totalTriangles = (numberOfQuads * TRIANGLES_PER_QUAD) + numberOfTris;
            _modelSpaceMeshTriangleSets[i].reserve(totalTriangles);

            auto meshTransform = getFBXGeometry().offset * mesh.modelTransform;

            if (part.quadIndices.size() > 0) {
                int vIndex = 0;
                for (int q = 0; q < numberOfQuads; q++) {
                    int i0 = part.quadIndices[vIndex++];
                    int i1 = part.quadIndices[vIndex++];
                    int i2 = part.quadIndices[vIndex++];
                    int i3 = part.quadIndices[vIndex++];

                    // track the model space version... these points will be transformed by the FST's offset, 
                    // which includes the scaling, rotation, and translation specified by the FST/FBX, 
                    // this can't change at runtime, so we can safely store these in our TriangleSet
                    glm::vec3 v0 = glm::vec3(meshTransform * glm::vec4(mesh.vertices[i0], 1.0f));
                    glm::vec3 v1 = glm::vec3(meshTransform * glm::vec4(mesh.vertices[i1], 1.0f));
                    glm::vec3 v2 = glm::vec3(meshTransform * glm::vec4(mesh.vertices[i2], 1.0f));
                    glm::vec3 v3 = glm::vec3(meshTransform * glm::vec4(mesh.vertices[i3], 1.0f));

                    Triangle tri1 = { v0, v1, v3 };
                    Triangle tri2 = { v1, v2, v3 };
                    _modelSpaceMeshTriangleSets[i].insert(tri1);
                    _modelSpaceMeshTriangleSets[i].insert(tri2);
                }
            }

            if (part.triangleIndices.size() > 0) {
                int vIndex = 0;
                for (int t = 0; t < numberOfTris; t++) {
                    int i0 = part.triangleIndices[vIndex++];
                    int i1 = part.triangleIndices[vIndex++];
                    int i2 = part.triangleIndices[vIndex++];

                    // track the model space version... these points will be transformed by the FST's offset, 
                    // which includes the scaling, rotation, and translation specified by the FST/FBX, 
                    // this can't change at runtime, so we can safely store these in our TriangleSet
                    glm::vec3 v0 = glm::vec3(meshTransform * glm::vec4(mesh.vertices[i0], 1.0f));
                    glm::vec3 v1 = glm::vec3(meshTransform * glm::vec4(mesh.vertices[i1], 1.0f));
                    glm::vec3 v2 = glm::vec3(meshTransform * glm::vec4(mesh.vertices[i2], 1.0f));

                    Triangle tri = { v0, v1, v2 };
                    _modelSpaceMeshTriangleSets[i].insert(tri);
                }
            }
        }
    }
}

void Model::setVisibleInScene(bool newValue, const render::ScenePointer& scene) {
    if (_isVisible != newValue) {
        _isVisible = newValue;

        render::Transaction transaction;
        foreach (auto item, _modelMeshRenderItemsMap.keys()) {
            transaction.resetItem(item, _modelMeshRenderItemsMap[item]);
        }
        foreach(auto item, _collisionRenderItemsMap.keys()) {
            transaction.resetItem(item, _collisionRenderItemsMap[item]);
        }
        scene->enqueueTransaction(transaction);
    }
}


void Model::setLayeredInFront(bool layered, const render::ScenePointer& scene) {
    if (_isLayeredInFront != layered) {
        _isLayeredInFront = layered;

        render::Transaction transaction;
        foreach(auto item, _modelMeshRenderItemsMap.keys()) {
            transaction.resetItem(item, _modelMeshRenderItemsMap[item]);
        }
        foreach(auto item, _collisionRenderItemsMap.keys()) {
            transaction.resetItem(item, _collisionRenderItemsMap[item]);
        }
        scene->enqueueTransaction(transaction);
    }
}

bool Model::addToScene(const render::ScenePointer& scene,
                       render::Transaction& transaction,
                       render::Item::Status::Getters& statusGetters) {
    bool readyToRender = _collisionGeometry || isLoaded();
    if (!_addedToScene && readyToRender) {
        createRenderItemSet();
    }

    bool somethingAdded = false;
    if (_collisionGeometry) {
        if (_collisionRenderItemsMap.empty()) {
            foreach (auto renderItem, _collisionRenderItems) {
                auto item = scene->allocateID();
                auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderItem);
                if (_collisionRenderItems.empty() && statusGetters.size()) {
                    renderPayload->addStatusGetters(statusGetters);
                }
                transaction.resetItem(item, renderPayload);
                _collisionRenderItemsMap.insert(item, renderPayload);
            }
            somethingAdded = !_collisionRenderItemsMap.empty();
        }
    } else {
        if (_modelMeshRenderItemsMap.empty()) {

            bool hasTransparent = false;
            size_t verticesCount = 0;
            foreach(auto renderItem, _modelMeshRenderItems) {
                auto item = scene->allocateID();
                auto renderPayload = std::make_shared<ModelMeshPartPayload::Payload>(renderItem);
                if (_modelMeshRenderItemsMap.empty() && statusGetters.size()) {
                    renderPayload->addStatusGetters(statusGetters);
                }
                transaction.resetItem(item, renderPayload);

                hasTransparent = hasTransparent || renderItem.get()->getShapeKey().isTranslucent();
                verticesCount += renderItem.get()->getVerticesCount();
                _modelMeshRenderItemsMap.insert(item, renderPayload);
                _modelMeshRenderItemIDs.emplace_back(item);
            }
            somethingAdded = !_modelMeshRenderItemsMap.empty();

            _renderInfoVertexCount = verticesCount;
            _renderInfoDrawCalls = _modelMeshRenderItemsMap.count();
            _renderInfoHasTransparent = hasTransparent;
        }
    }

    if (somethingAdded) {
        _addedToScene = true;
        updateRenderItems();
        _needsFixupInScene = false;
    }

    return somethingAdded;
}

void Model::removeFromScene(const render::ScenePointer& scene, render::Transaction& transaction) {
    foreach (auto item, _modelMeshRenderItemsMap.keys()) {
        transaction.removeItem(item);
    }
    _modelMeshRenderItemIDs.clear();
    _modelMeshRenderItemsMap.clear();
    _modelMeshRenderItems.clear();

    foreach(auto item, _collisionRenderItemsMap.keys()) {
        transaction.removeItem(item);
    }
    _collisionRenderItems.clear();
    _collisionRenderItemsMap.clear();
    _addedToScene = false;

    _renderInfoVertexCount = 0;
    _renderInfoDrawCalls = 0;
    _renderInfoTextureSize = 0;
    _renderInfoHasTransparent = false;
}

void Model::renderDebugMeshBoxes(gpu::Batch& batch) {
    int colorNdx = 0;
    _mutex.lock();

    glm::mat4 meshToModelMatrix = glm::scale(_scale) * glm::translate(_offset);
    glm::mat4 meshToWorldMatrix = createMatFromQuatAndPos(_rotation, _translation) * meshToModelMatrix;
    Transform meshToWorld(meshToWorldMatrix);
    batch.setModelTransform(meshToWorld);

    DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch, false, false, false, true, true);

    for(const auto& triangleSet : _modelSpaceMeshTriangleSets) {
        auto box = triangleSet.getBounds();

        if (_debugMeshBoxesID == GeometryCache::UNKNOWN_ID) {
            _debugMeshBoxesID = DependencyManager::get<GeometryCache>()->allocateID();
        }
        QVector<glm::vec3> points;

        glm::vec3 brn = box.getCorner();
        glm::vec3 bln = brn + glm::vec3(box.getDimensions().x, 0, 0);
        glm::vec3 brf = brn + glm::vec3(0, 0, box.getDimensions().z);
        glm::vec3 blf = brn + glm::vec3(box.getDimensions().x, 0, box.getDimensions().z);

        glm::vec3 trn = brn + glm::vec3(0, box.getDimensions().y, 0);
        glm::vec3 tln = bln + glm::vec3(0, box.getDimensions().y, 0);
        glm::vec3 trf = brf + glm::vec3(0, box.getDimensions().y, 0);
        glm::vec3 tlf = blf + glm::vec3(0, box.getDimensions().y, 0);

        points << brn << bln;
        points << brf << blf;
        points << brn << brf;
        points << bln << blf;

        points << trn << tln;
        points << trf << tlf;
        points << trn << trf;
        points << tln << tlf;

        points << brn << trn;
        points << brf << trf;
        points << bln << tln;
        points << blf << tlf;

        glm::vec4 color[] = {
            { 0.0f, 1.0f, 0.0f, 1.0f }, // green
            { 1.0f, 0.0f, 0.0f, 1.0f }, // red
            { 0.0f, 0.0f, 1.0f, 1.0f }, // blue
            { 1.0f, 0.0f, 1.0f, 1.0f }, // purple
            { 1.0f, 1.0f, 0.0f, 1.0f }, // yellow
            { 0.0f, 1.0f, 1.0f, 1.0f }, // cyan
            { 1.0f, 1.0f, 1.0f, 1.0f }, // white
            { 0.0f, 0.5f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.5f, 1.0f },
            { 0.5f, 0.0f, 0.5f, 1.0f },
            { 0.5f, 0.5f, 0.0f, 1.0f },
            { 0.0f, 0.5f, 0.5f, 1.0f } };

        DependencyManager::get<GeometryCache>()->updateVertices(_debugMeshBoxesID, points, color[colorNdx]);
        DependencyManager::get<GeometryCache>()->renderVertices(batch, gpu::LINES, _debugMeshBoxesID);
        colorNdx++;
    }
    _mutex.unlock();
}

Extents Model::getBindExtents() const {
    if (!isActive()) {
        return Extents();
    }
    const Extents& bindExtents = getFBXGeometry().bindExtents;
    Extents scaledExtents = { bindExtents.minimum * _scale, bindExtents.maximum * _scale };
    return scaledExtents;
}

glm::vec3 Model::getNaturalDimensions() const {
    Extents modelMeshExtents = getUnscaledMeshExtents();
    return modelMeshExtents.maximum - modelMeshExtents.minimum;
}

Extents Model::getMeshExtents() const {
    if (!isActive()) {
        return Extents();
    }
    const Extents& extents = getFBXGeometry().meshExtents;

    // even though our caller asked for "unscaled" we need to include any fst scaling, translation, and rotation, which
    // is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(getFBXGeometry().offset * glm::vec4(extents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(getFBXGeometry().offset * glm::vec4(extents.maximum, 1.0f));
    Extents scaledExtents = { minimum * _scale, maximum * _scale };
    return scaledExtents;
}

Extents Model::getUnscaledMeshExtents() const {
    if (!isActive()) {
        return Extents();
    }

    const Extents& extents = getFBXGeometry().meshExtents;

    // even though our caller asked for "unscaled" we need to include any fst scaling, translation, and rotation, which
    // is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(getFBXGeometry().offset * glm::vec4(extents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(getFBXGeometry().offset * glm::vec4(extents.maximum, 1.0f));
    Extents scaledExtents = { minimum, maximum };

    return scaledExtents;
}

void Model::clearJointState(int index) {
    _rig.clearJointState(index);
}

void Model::setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority) {
    _rig.setJointState(index, valid, rotation, translation, priority);
}

void Model::setJointRotation(int index, bool valid, const glm::quat& rotation, float priority) {
    _rig.setJointRotation(index, valid, rotation, priority);
}

void Model::setJointTranslation(int index, bool valid, const glm::vec3& translation, float priority) {
    _rig.setJointTranslation(index, valid, translation, priority);
}

int Model::getParentJointIndex(int jointIndex) const {
    return (isActive() && jointIndex != -1) ? getFBXGeometry().joints.at(jointIndex).parentIndex : -1;
}

int Model::getLastFreeJointIndex(int jointIndex) const {
    return (isActive() && jointIndex != -1) ? getFBXGeometry().joints.at(jointIndex).freeLineage.last() : -1;
}

void Model::setTextures(const QVariantMap& textures) {
    if (isLoaded()) {
        _needsUpdateTextures = true;
        _needsFixupInScene = true;
        _renderGeometry->setTextures(textures);
    } else {
        // FIXME(Huffman): Disconnect previously connected lambdas so we don't set textures multiple
        // after the geometry has finished loading.
        connect(&_renderWatcher, &GeometryResourceWatcher::finished, this, [this, textures]() {
            _renderGeometry->setTextures(textures);
        });
    }
}

void Model::setURL(const QUrl& url) {
    // don't recreate the geometry if it's the same URL
    if (_url == url && _renderWatcher.getURL() == url) {
        return;
    }

    _url = url;

    {
        render::Transaction transaction;
        const render::ScenePointer& scene = AbstractViewStateInterface::instance()->getMain3DScene();
        if (scene) {
            removeFromScene(scene, transaction);
            scene->enqueueTransaction(transaction);
        } else {
            qCWarning(renderutils) << "Model::setURL(), Unexpected null scene, possibly during application shutdown";
        }
    }

    _needsReload = true;
    _needsUpdateTextures = true;
    _visualGeometryRequestFailed = false;
    _needsFixupInScene = true;
    invalidCalculatedMeshBoxes();
    deleteGeometry();

    auto resource = DependencyManager::get<ModelCache>()->getGeometryResource(url);
    if (resource) {
        resource->setLoadPriority(this, _loadingPriority);
        _renderWatcher.setResource(resource);
    }
    onInvalidate();
}

void Model::loadURLFinished(bool success) {
    if (!success) {
        _visualGeometryRequestFailed = true;
    }
    emit setURLFinished(success);
}

bool Model::getJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const {
    return _rig.getJointPositionInWorldFrame(jointIndex, position, _translation, _rotation);
}

bool Model::getJointPosition(int jointIndex, glm::vec3& position) const {
    return _rig.getJointPosition(jointIndex, position);
}

bool Model::getJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const {
    return _rig.getJointRotationInWorldFrame(jointIndex, rotation, _rotation);
}

bool Model::getJointRotation(int jointIndex, glm::quat& rotation) const {
    return _rig.getJointRotation(jointIndex, rotation);
}

bool Model::getJointTranslation(int jointIndex, glm::vec3& translation) const {
    return _rig.getJointTranslation(jointIndex, translation);
}

bool Model::getAbsoluteJointRotationInRigFrame(int jointIndex, glm::quat& rotationOut) const {
    return _rig.getAbsoluteJointRotationInRigFrame(jointIndex, rotationOut);
}

bool Model::getAbsoluteJointTranslationInRigFrame(int jointIndex, glm::vec3& translationOut) const {
    return _rig.getAbsoluteJointTranslationInRigFrame(jointIndex, translationOut);
}

bool Model::getRelativeDefaultJointRotation(int jointIndex, glm::quat& rotationOut) const {
    return _rig.getRelativeDefaultJointRotation(jointIndex, rotationOut);
}

bool Model::getRelativeDefaultJointTranslation(int jointIndex, glm::vec3& translationOut) const {
    return _rig.getRelativeDefaultJointTranslation(jointIndex, translationOut);
}

QStringList Model::getJointNames() const {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        BLOCKING_INVOKE_METHOD(const_cast<Model*>(this), "getJointNames",
            Q_RETURN_ARG(QStringList, result));
        return result;
    }
    return isActive() ? getFBXGeometry().getJointNames() : QStringList();
}

class Blender : public QRunnable {
public:

    Blender(ModelPointer model, int blendNumber, const Geometry::WeakPointer& geometry,
        const QVector<FBXMesh>& meshes, const QVector<float>& blendshapeCoefficients);

    virtual void run() override;

private:

    ModelPointer _model;
    int _blendNumber;
    Geometry::WeakPointer _geometry;
    QVector<FBXMesh> _meshes;
    QVector<float> _blendshapeCoefficients;
};

Blender::Blender(ModelPointer model, int blendNumber, const Geometry::WeakPointer& geometry,
        const QVector<FBXMesh>& meshes, const QVector<float>& blendshapeCoefficients) :
    _model(model),
    _blendNumber(blendNumber),
    _geometry(geometry),
    _meshes(meshes),
    _blendshapeCoefficients(blendshapeCoefficients) {
}

void Blender::run() {
    PROFILE_RANGE_EX(simulation_animation, __FUNCTION__, 0xFFFF0000, 0, { { "url", _model->getURL().toString() } });
    QVector<glm::vec3> vertices, normals;
    if (_model) {
        int offset = 0;
        foreach (const FBXMesh& mesh, _meshes) {
            if (mesh.blendshapes.isEmpty()) {
                continue;
            }
            vertices += mesh.vertices;
            normals += mesh.normals;
            glm::vec3* meshVertices = vertices.data() + offset;
            glm::vec3* meshNormals = normals.data() + offset;
            offset += mesh.vertices.size();
            const float NORMAL_COEFFICIENT_SCALE = 0.01f;
            for (int i = 0, n = qMin(_blendshapeCoefficients.size(), mesh.blendshapes.size()); i < n; i++) {
                float vertexCoefficient = _blendshapeCoefficients.at(i);
                const float EPSILON = 0.0001f;
                if (vertexCoefficient < EPSILON) {
                    continue;
                }
                float normalCoefficient = vertexCoefficient * NORMAL_COEFFICIENT_SCALE;
                const FBXBlendshape& blendshape = mesh.blendshapes.at(i);
                for (int j = 0; j < blendshape.indices.size(); j++) {
                    int index = blendshape.indices.at(j);
                    meshVertices[index] += blendshape.vertices.at(j) * vertexCoefficient;
                    meshNormals[index] += blendshape.normals.at(j) * normalCoefficient;
                }
            }
        }
    }
    // post the result to the geometry cache, which will dispatch to the model if still alive
    QMetaObject::invokeMethod(DependencyManager::get<ModelBlender>().data(), "setBlendedVertices",
        Q_ARG(ModelPointer, _model), Q_ARG(int, _blendNumber),
        Q_ARG(const Geometry::WeakPointer&, _geometry), Q_ARG(const QVector<glm::vec3>&, vertices),
        Q_ARG(const QVector<glm::vec3>&, normals));
}

void Model::setScaleToFit(bool scaleToFit, const glm::vec3& dimensions, bool forceRescale) {
    if (forceRescale || _scaleToFit != scaleToFit || _scaleToFitDimensions != dimensions) {
        _scaleToFit = scaleToFit;
        _scaleToFitDimensions = dimensions;
        _scaledToFit = false; // force rescaling
    }
}

void Model::setScaleToFit(bool scaleToFit, float largestDimension, bool forceRescale) {
    // NOTE: if the model is not active, then it means we don't actually know the true/natural dimensions of the
    // mesh, and so we can't do the needed calculations for scaling to fit to a single largest dimension. In this
    // case we will record that we do want to do this, but we will stick our desired single dimension into the
    // first element of the vec3 for the non-fixed aspect ration dimensions
    if (!isActive()) {
        _scaleToFit = scaleToFit;
        if (scaleToFit) {
            _scaleToFitDimensions = glm::vec3(largestDimension, FAKE_DIMENSION_PLACEHOLDER, FAKE_DIMENSION_PLACEHOLDER);
        }
        return;
    }

    if (forceRescale || _scaleToFit != scaleToFit || glm::length(_scaleToFitDimensions) != largestDimension) {
        _scaleToFit = scaleToFit;

        // we only need to do this work if we're "turning on" scale to fit.
        if (scaleToFit) {
            Extents modelMeshExtents = getUnscaledMeshExtents();
            float maxDimension = glm::distance(modelMeshExtents.maximum, modelMeshExtents.minimum);
            float maxScale = largestDimension / maxDimension;
            glm::vec3 modelMeshDimensions = modelMeshExtents.maximum - modelMeshExtents.minimum;
            glm::vec3 dimensions = modelMeshDimensions * maxScale;

            _scaleToFitDimensions = dimensions;
            _scaledToFit = false; // force rescaling
        }
    }
}

glm::vec3 Model::getScaleToFitDimensions() const {
    if (_scaleToFitDimensions.y == FAKE_DIMENSION_PLACEHOLDER &&
        _scaleToFitDimensions.z == FAKE_DIMENSION_PLACEHOLDER) {
        return glm::vec3(_scaleToFitDimensions.x);
    }
    return _scaleToFitDimensions;
}

void Model::scaleToFit() {
    // If our _scaleToFitDimensions.y/z are FAKE_DIMENSION_PLACEHOLDER then it means our
    // user asked to scale us in a fixed aspect ratio to a single largest dimension, but
    // we didn't yet have an active mesh. We can only enter this scaleToFit() in this state
    // if we now do have an active mesh, so we take this opportunity to actually determine
    // the correct scale.
    if (_scaleToFit && _scaleToFitDimensions.y == FAKE_DIMENSION_PLACEHOLDER
            && _scaleToFitDimensions.z == FAKE_DIMENSION_PLACEHOLDER) {
        setScaleToFit(_scaleToFit, _scaleToFitDimensions.x);
    }
    Extents modelMeshExtents = getUnscaledMeshExtents();

    // size is our "target size in world space"
    // we need to set our model scale so that the extents of the mesh, fit in a box that size...
    glm::vec3 meshDimensions = modelMeshExtents.maximum - modelMeshExtents.minimum;
    glm::vec3 rescaleDimensions = _scaleToFitDimensions / meshDimensions;
    setScaleInternal(rescaleDimensions);
    _scaledToFit = true;
}

void Model::setSnapModelToRegistrationPoint(bool snapModelToRegistrationPoint, const glm::vec3& registrationPoint) {
    glm::vec3 clampedRegistrationPoint = glm::clamp(registrationPoint, 0.0f, 1.0f);
    if (_snapModelToRegistrationPoint != snapModelToRegistrationPoint || _registrationPoint != clampedRegistrationPoint) {
        _snapModelToRegistrationPoint = snapModelToRegistrationPoint;
        _registrationPoint = clampedRegistrationPoint;
        _snappedToRegistrationPoint = false; // force re-centering
    }
}

void Model::snapToRegistrationPoint() {
    Extents modelMeshExtents = getUnscaledMeshExtents();
    glm::vec3 dimensions = (modelMeshExtents.maximum - modelMeshExtents.minimum);
    glm::vec3 offset = -modelMeshExtents.minimum - (dimensions * _registrationPoint);
    _offset = offset;
    _snappedToRegistrationPoint = true;
}

void Model::simulate(float deltaTime, bool fullUpdate) {
    PROFILE_RANGE(simulation_detail, __FUNCTION__);
    PerformanceTimer perfTimer("Model::simulate");
    fullUpdate = updateGeometry() || fullUpdate || (_scaleToFit && !_scaledToFit)
                    || (_snapModelToRegistrationPoint && !_snappedToRegistrationPoint);

    if (isActive() && fullUpdate) {
        onInvalidate();

        // check for scale to fit
        if (_scaleToFit && !_scaledToFit) {
            scaleToFit();
        }
        if (_snapModelToRegistrationPoint && !_snappedToRegistrationPoint) {
            snapToRegistrationPoint();
        }
        // update the world space transforms for all joints
        glm::mat4 parentTransform = glm::scale(_scale) * glm::translate(_offset);
        updateRig(deltaTime, parentTransform);

        computeMeshPartLocalBounds();
    }
}

//virtual
void Model::updateRig(float deltaTime, glm::mat4 parentTransform) {
    _needsUpdateClusterMatrices = true;
    glm::mat4 rigToWorldTransform = createMatFromQuatAndPos(getRotation(), getTranslation());
    _rig.updateAnimations(deltaTime, parentTransform, rigToWorldTransform);
}

void Model::computeMeshPartLocalBounds() {
    for (auto& part : _modelMeshRenderItems) {
        const Model::MeshState& state = _meshStates.at(part->_meshIndex);
        part->computeAdjustedLocalBound(state.clusterMatrices);
    }
}

// virtual
void Model::updateClusterMatrices() {
    PerformanceTimer perfTimer("Model::updateClusterMatrices");

    if (!_needsUpdateClusterMatrices || !isLoaded()) {
        return;
    }
    _needsUpdateClusterMatrices = false;
    const FBXGeometry& geometry = getFBXGeometry();
    for (int i = 0; i < _meshStates.size(); i++) {
        MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);
        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);
            auto jointMatrix = _rig.getJointTransform(cluster.jointIndex);
            glm_mat4u_mul(jointMatrix, cluster.inverseBindMatrix, state.clusterMatrices[j]);
        }

        // Once computed the cluster matrices, update the buffer(s)
        if (mesh.clusters.size() > 1) {
            if (!state.clusterBuffer) {
                state.clusterBuffer = std::make_shared<gpu::Buffer>(state.clusterMatrices.size() * sizeof(glm::mat4),
                                                                    (const gpu::Byte*) state.clusterMatrices.constData());
            } else {
                state.clusterBuffer->setSubData(0, state.clusterMatrices.size() * sizeof(glm::mat4),
                                                (const gpu::Byte*) state.clusterMatrices.constData());
            }
        }
    }

    // post the blender if we're not currently waiting for one to finish
    if (geometry.hasBlendedMeshes() && _blendshapeCoefficients != _blendedBlendshapeCoefficients) {
        _blendedBlendshapeCoefficients = _blendshapeCoefficients;
        DependencyManager::get<ModelBlender>()->noteRequiresBlend(getThisPointer());
    }
}

void Model::inverseKinematics(int endIndex, glm::vec3 targetPosition, const glm::quat& targetRotation, float priority) {
    const FBXGeometry& geometry = getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(endIndex).freeLineage;
    glm::mat4 parentTransform = glm::scale(_scale) * glm::translate(_offset);
    _rig.inverseKinematics(endIndex, targetPosition, targetRotation, priority, freeLineage, parentTransform);
}

bool Model::restoreJointPosition(int jointIndex, float fraction, float priority) {
    const FBXGeometry& geometry = getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    return _rig.restoreJointPosition(jointIndex, fraction, priority, freeLineage);
}

float Model::getLimbLength(int jointIndex) const {
    const FBXGeometry& geometry = getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    return _rig.getLimbLength(jointIndex, freeLineage, _scale, geometry.joints);
}

bool Model::maybeStartBlender() {
    if (isLoaded()) {
        const FBXGeometry& fbxGeometry = getFBXGeometry();
        if (fbxGeometry.hasBlendedMeshes()) {
            QThreadPool::globalInstance()->start(new Blender(getThisPointer(), ++_blendNumber, _renderGeometry,
                fbxGeometry.meshes, _blendshapeCoefficients));
            return true;
        }
    }
    return false;
}

void Model::setBlendedVertices(int blendNumber, const Geometry::WeakPointer& geometry,
        const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals) {
    auto geometryRef = geometry.lock();
    if (!geometryRef || _renderGeometry != geometryRef || _blendedVertexBuffers.empty() || blendNumber < _appliedBlendNumber) {
        return;
    }
    _appliedBlendNumber = blendNumber;
    const FBXGeometry& fbxGeometry = getFBXGeometry();
    int index = 0;
    for (int i = 0; i < fbxGeometry.meshes.size(); i++) {
        const FBXMesh& mesh = fbxGeometry.meshes.at(i);
        if (mesh.blendshapes.isEmpty()) {
            continue;
        }

        gpu::BufferPointer& buffer = _blendedVertexBuffers[i];
        buffer->setSubData(0, mesh.vertices.size() * sizeof(glm::vec3), (gpu::Byte*) vertices.constData() + index*sizeof(glm::vec3));
        buffer->setSubData(mesh.vertices.size() * sizeof(glm::vec3),
            mesh.normals.size() * sizeof(glm::vec3), (gpu::Byte*) normals.constData() + index*sizeof(glm::vec3));

        index += mesh.vertices.size();
    }
}

void Model::deleteGeometry() {
    _deleteGeometryCounter++;
    _blendedVertexBuffers.clear();
    _meshStates.clear();
    _rig.destroyAnimGraph();
    _blendedBlendshapeCoefficients.clear();
    _renderGeometry.reset();
    _collisionGeometry.reset();
}

AABox Model::getRenderableMeshBound() const {
    if (!isLoaded()) {
        return AABox();
    } else {
        // Build a bound using the last known bound from all the renderItems.
        AABox totalBound;
        for (auto& renderItem : _modelMeshRenderItems) {
            totalBound += renderItem->getBound();
        }
        return totalBound;
    }
}

const render::ItemIDs& Model::fetchRenderItemIDs() const {
    return _modelMeshRenderItemIDs;
}

void Model::createRenderItemSet() {
    if (_collisionGeometry) {
        if (_collisionRenderItems.empty()) {
            createCollisionRenderItemSet();
        }
    } else {
        if (_modelMeshRenderItems.empty()) {
            createVisibleRenderItemSet();
        }
    }
};

void Model::createVisibleRenderItemSet() {
    assert(isLoaded());
    const auto& meshes = _renderGeometry->getMeshes();

    // all of our mesh vectors must match in size
    if ((int)meshes.size() != _meshStates.size()) {
        qCDebug(renderutils) << "WARNING!!!! Mesh Sizes don't match! We will not segregate mesh groups yet.";
        return;
    }

    // We should not have any existing renderItems if we enter this section of code
    Q_ASSERT(_modelMeshRenderItems.isEmpty());

    _modelMeshRenderItems.clear();

    Transform transform;
    transform.setTranslation(_translation);
    transform.setRotation(_rotation);

    Transform offset;
    offset.setScale(_scale);
    offset.postTranslate(_offset);

    // Run through all of the meshes, and place them into their segregated, but unsorted buckets
    int shapeID = 0;
    uint32_t numMeshes = (uint32_t)meshes.size();
    for (uint32_t i = 0; i < numMeshes; i++) {
        const auto& mesh = meshes.at(i);
        if (!mesh) {
            continue;
        }

        // Create the render payloads
        int numParts = (int)mesh->getNumParts();
        for (int partIndex = 0; partIndex < numParts; partIndex++) {
            _modelMeshRenderItems << std::make_shared<ModelMeshPartPayload>(shared_from_this(), i, partIndex, shapeID, transform, offset);
            shapeID++;
        }
    }
    computeMeshPartLocalBounds();
}

void Model::createCollisionRenderItemSet() {
    assert((bool)_collisionGeometry);
    if (_collisionMaterials.empty()) {
        initCollisionMaterials();
    }

    const auto& meshes = _collisionGeometry->getMeshes();

    // We should not have any existing renderItems if we enter this section of code
    Q_ASSERT(_collisionRenderItems.isEmpty());

    Transform identity;
    identity.setIdentity();
    Transform offset;
    offset.postTranslate(_offset);

    // Run through all of the meshes, and place them into their segregated, but unsorted buckets
    uint32_t numMeshes = (uint32_t)meshes.size();
    for (uint32_t i = 0; i < numMeshes; i++) {
        const auto& mesh = meshes.at(i);
        if (!mesh) {
            continue;
        }

        // Create the render payloads
        int numParts = (int)mesh->getNumParts();
        for (int partIndex = 0; partIndex < numParts; partIndex++) {
            model::MaterialPointer& material = _collisionMaterials[partIndex % NUM_COLLISION_HULL_COLORS];
            auto payload = std::make_shared<MeshPartPayload>(mesh, partIndex, material);
            payload->updateTransform(identity, offset);
            _collisionRenderItems << payload;
        }
    }
}

bool Model::isRenderable() const {
    return !_meshStates.isEmpty() || (isLoaded() && _renderGeometry->getMeshes().empty());
}

bool Model::initWhenReady(const render::ScenePointer& scene) {
    // NOTE: this only called by SkeletonModel
    if (_addedToScene || !isRenderable()) {
        return false;
    }

    createRenderItemSet();

    render::Transaction transaction;

    bool addedTransaction = false;
    if (_collisionGeometry) {
        foreach (auto renderItem, _collisionRenderItems) {
            auto item = scene->allocateID();
            auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderItem);
            _collisionRenderItemsMap.insert(item, renderPayload);
            transaction.resetItem(item, renderPayload);
        }
        addedTransaction = !_collisionRenderItems.empty();
    } else {
        bool hasTransparent = false;
        size_t verticesCount = 0;
        foreach (auto renderItem, _modelMeshRenderItems) {
            auto item = scene->allocateID();
            auto renderPayload = std::make_shared<ModelMeshPartPayload::Payload>(renderItem);

            hasTransparent = hasTransparent || renderItem.get()->getShapeKey().isTranslucent();
            verticesCount += renderItem.get()->getVerticesCount();
            _modelMeshRenderItemsMap.insert(item, renderPayload);
            transaction.resetItem(item, renderPayload);
        }
        addedTransaction = !_modelMeshRenderItemsMap.empty();
        _renderInfoVertexCount = verticesCount;
        _renderInfoDrawCalls = _modelMeshRenderItemsMap.count();
        _renderInfoHasTransparent = hasTransparent;
    }
    _addedToScene = addedTransaction;
    if (addedTransaction) {
        scene->enqueueTransaction(transaction);
        // NOTE: updateRender items enqueues identical transaction (using a lambda)
        // so it looks like we're doing double work here, but I don't want to remove the call
        // for fear there is some side effect we'll miss. -- Andrew 2016.07.21
        // TODO: figure out if we really need this call to updateRenderItems() or not.
        updateRenderItems();
    }

    return true;
}

class CollisionRenderGeometry : public Geometry {
public:
    CollisionRenderGeometry(model::MeshPointer mesh) {
        _fbxGeometry = std::make_shared<FBXGeometry>();
        std::shared_ptr<GeometryMeshes> meshes = std::make_shared<GeometryMeshes>();
        meshes->push_back(mesh);
        _meshes = meshes;
        _meshParts = std::shared_ptr<const GeometryMeshParts>();
    }
};

void Model::setCollisionMesh(model::MeshPointer mesh) {
    if (mesh) {
        _collisionGeometry = std::make_shared<CollisionRenderGeometry>(mesh);
    } else {
        _collisionGeometry.reset();
    }
    _needsFixupInScene = true;
}

ModelBlender::ModelBlender() :
    _pendingBlenders(0) {
}

ModelBlender::~ModelBlender() {
}

void ModelBlender::noteRequiresBlend(ModelPointer model) {
    if (_pendingBlenders < QThread::idealThreadCount()) {
        if (model->maybeStartBlender()) {
            _pendingBlenders++;
        }
        return;
    }

    {
        Lock lock(_mutex);
        _modelsRequiringBlends.insert(model);
    }
}

void ModelBlender::setBlendedVertices(ModelPointer model, int blendNumber,
        const Geometry::WeakPointer& geometry, const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals) {
    if (model) {
        model->setBlendedVertices(blendNumber, geometry, vertices, normals);
    }
    _pendingBlenders--;
    {
        Lock lock(_mutex);
        for (auto i = _modelsRequiringBlends.begin(); i != _modelsRequiringBlends.end();) {
            auto weakPtr = *i;
            _modelsRequiringBlends.erase(i++); // remove front of the set
            ModelPointer nextModel = weakPtr.lock();
            if (nextModel && nextModel->maybeStartBlender()) {
                _pendingBlenders++;
                return;
            }
        }
    }
}

