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

#include <QMetaType>
#include <QRunnable>
#include <QThreadPool>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include <GeometryUtil.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <ViewFrustum.h>
#include <GLMHelpers.h>

#include "AbstractViewStateInterface.h"
#include "MeshPartPayload.h"
#include "Model.h"

#include "RenderUtilsLogging.h"

using namespace std;

int nakedModelPointerTypeId = qRegisterMetaType<ModelPointer>();
int weakGeometryResourceBridgePointerTypeId = qRegisterMetaType<Geometry::WeakPointer >();
int vec3VectorTypeId = qRegisterMetaType<QVector<glm::vec3> >();
float Model::FAKE_DIMENSION_PLACEHOLDER = -1.0f;
#define HTTP_INVALID_COM "http://invalid.com"

const int NUM_COLLISION_HULL_COLORS = 24;
std::vector<model::MaterialPointer> _collisionHullMaterials;

void initCollisionHullMaterials() {
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
    _collisionHullMaterials.reserve(NUM_COLLISION_HULL_COLORS);

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
            _collisionHullMaterials.push_back(material);
        }
    }
}

Model::Model(RigPointer rig, QObject* parent) :
    QObject(parent),
    _renderGeometry(),
    _collisionGeometry(),
    _renderWatcher(_renderGeometry),
    _collisionWatcher(_collisionGeometry),
    _translation(0.0f),
    _rotation(),
    _scale(1.0f, 1.0f, 1.0f),
    _scaleToFit(false),
    _scaleToFitDimensions(1.0f),
    _scaledToFit(false),
    _snapModelToRegistrationPoint(false),
    _snappedToRegistrationPoint(false),
    _cauterizeBones(false),
    _pupilDilation(0.0f),
    _url(HTTP_INVALID_COM),
    _isVisible(true),
    _blendNumber(0),
    _appliedBlendNumber(0),
    _calculatedMeshPartBoxesValid(false),
    _calculatedMeshBoxesValid(false),
    _calculatedMeshTrianglesValid(false),
    _meshGroupsKnown(false),
    _isWireframe(false),
    _rig(rig)
{
    // we may have been created in the network thread, but we live in the main thread
    if (_viewState) {
        moveToThread(_viewState->getMainThread());
    }

    setSnapModelToRegistrationPoint(true, glm::vec3(0.5f));

    // handle download failure reported by the GeometryResourceWatcher
    connect(&_renderWatcher, &GeometryResourceWatcher::resourceFailed, this, &Model::handleGeometryResourceFailure);
}

Model::~Model() {
    deleteGeometry();
}

AbstractViewStateInterface* Model::_viewState = NULL;

bool Model::needsFixupInScene() const {
    if (readyToAddToScene()) {
        if (_needsUpdateTextures && _renderGeometry->areTexturesLoaded()) {
            _needsUpdateTextures = false;
            return true;
        }
        if (!_readyWhenAdded) {
            return true;
        }
    }
    return false;
}

void Model::setTranslation(const glm::vec3& translation) {
    _translation = translation;
    updateRenderItems();
}

void Model::setRotation(const glm::quat& rotation) {
    _rotation = rotation;
    updateRenderItems();
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

void Model::updateRenderItems() {

    _needsUpdateClusterMatrices = true;

    // queue up this work for later processing, at the end of update and just before rendering.
    // the application will ensure only the last lambda is actually invoked.
    void* key = (void*)this;
    std::weak_ptr<Model> weakSelf = shared_from_this();
    AbstractViewStateInterface::instance()->pushPostUpdateLambda(key, [weakSelf]() {

        // do nothing, if the model has already been destroyed.
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }

        render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();

        Transform modelTransform;
        modelTransform.setScale(self->_scale);
        modelTransform.setTranslation(self->_translation);
        modelTransform.setRotation(self->_rotation);

        Transform modelMeshOffset;
        if (self->isLoaded()) {
            // includes model offset and unitScale.
            modelMeshOffset = Transform(self->_rig->getGeometryToRigTransform());
        } else {
            modelMeshOffset.postTranslate(self->_offset);
        }

        // only apply offset only, collision mesh does not share the same unit scale as the FBX file's mesh.
        Transform collisionMeshOffset;
        collisionMeshOffset.postTranslate(self->_offset);

        uint32_t deleteGeometryCounter = self->_deleteGeometryCounter;

        render::PendingChanges pendingChanges;
        foreach (auto itemID, self->_modelMeshRenderItems.keys()) {
            pendingChanges.updateItem<ModelMeshPartPayload>(itemID, [modelTransform, modelMeshOffset, deleteGeometryCounter](ModelMeshPartPayload& data) {
                // Ensure the model geometry was not reset between frames
                if (data._model && data._model->isLoaded() && deleteGeometryCounter == data._model->_deleteGeometryCounter) {
                    // lazy update of cluster matrices used for rendering.  We need to update them here, so we can correctly update the bounding box.
                    data._model->updateClusterMatrices(modelTransform.getTranslation(), modelTransform.getRotation());

                    // update the model transform and bounding box for this render item.
                    const Model::MeshState& state = data._model->_meshStates.at(data._meshIndex);
                    data.updateTransformForSkinnedMesh(modelTransform, modelMeshOffset, state.clusterMatrices);
                }
            });
        }

        foreach (auto itemID, self->_collisionRenderItems.keys()) {
            pendingChanges.updateItem<MeshPartPayload>(itemID, [modelTransform, collisionMeshOffset](MeshPartPayload& data) {
                // update the model transform for this render item.
                data.updateTransform(modelTransform, collisionMeshOffset);
            });
        }

        scene->enqueuePendingChanges(pendingChanges);
    });
}

void Model::initJointTransforms() {
    if (isLoaded()) {
        glm::mat4 modelOffset = glm::scale(_scale) * glm::translate(_offset);
        _rig->setModelOffset(modelOffset);
    }
}

void Model::init() {
}

void Model::reset() {
    if (isLoaded()) {
        const FBXGeometry& geometry = getFBXGeometry();
        _rig->reset(geometry);
    }
}

bool Model::updateGeometry() {
    PROFILE_RANGE(__FUNCTION__);
    bool needFullUpdate = false;

    if (!isLoaded()) {
        return false;
    }

    _needsReload = false;

    if (_rig->jointStatesEmpty() && getFBXGeometry().joints.size() > 0) {
        initJointStates();

        const FBXGeometry& fbxGeometry = getFBXGeometry();
        foreach (const FBXMesh& mesh, fbxGeometry.meshes) {
            MeshState state;
            state.clusterMatrices.resize(mesh.clusters.size());
            state.cauterizedClusterMatrices.resize(mesh.clusters.size());

            _meshStates.append(state);

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

    _rig->initJointStates(geometry, modelOffset);
}

bool Model::findRayIntersectionAgainstSubMeshes(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                                    BoxFace& face, glm::vec3& surfaceNormal,
                                                    QString& extraInfo, bool pickAgainstTriangles) {

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
        float bestDistance = std::numeric_limits<float>::max();

        float distanceToSubMesh;
        BoxFace subMeshFace;
        glm::vec3 subMeshSurfaceNormal;
        int subMeshIndex = 0;

        const FBXGeometry& geometry = getFBXGeometry();

        // If we hit the models box, then consider the submeshes...
        _mutex.lock();
        if (!_calculatedMeshBoxesValid || (pickAgainstTriangles && !_calculatedMeshTrianglesValid)) {
            recalculateMeshBoxes(pickAgainstTriangles);
        }

        for (const auto& subMeshBox : _calculatedMeshBoxes) {

            if (subMeshBox.findRayIntersection(origin, direction, distanceToSubMesh, subMeshFace, subMeshSurfaceNormal)) {
                if (distanceToSubMesh < bestDistance) {
                    if (pickAgainstTriangles) {
                        // check our triangles here....
                        const QVector<Triangle>& meshTriangles = _calculatedMeshTriangles[subMeshIndex];
                        for(const auto& triangle : meshTriangles) {
                            float thisTriangleDistance;
                            if (findRayTriangleIntersection(origin, direction, triangle, thisTriangleDistance)) {
                                if (thisTriangleDistance < bestDistance) {
                                    bestDistance = thisTriangleDistance;
                                    intersectedSomething = true;
                                    face = subMeshFace;
                                    surfaceNormal = triangle.getNormal();
                                    extraInfo = geometry.getModelNameOfMesh(subMeshIndex);
                                }
                            }
                        }
                    } else {
                        // this is the non-triangle picking case...
                        bestDistance = distanceToSubMesh;
                        intersectedSomething = true;
                        face = subMeshFace;
                        surfaceNormal = subMeshSurfaceNormal;
                        extraInfo = geometry.getModelNameOfMesh(subMeshIndex);
                    }
                }
            }
            subMeshIndex++;
        }
        _mutex.unlock();

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
        _mutex.lock();
        if (!_calculatedMeshTrianglesValid) {
            recalculateMeshBoxes(true);
        }

        // If we are inside the models box, then consider the submeshes...
        int subMeshIndex = 0;
        foreach(const AABox& subMeshBox, _calculatedMeshBoxes) {
            if (subMeshBox.contains(point)) {
                bool insideMesh = true;
                // To be inside the sub mesh, we need to be behind every triangles' planes
                const QVector<Triangle>& meshTriangles = _calculatedMeshTriangles[subMeshIndex];
                foreach (const Triangle& triangle, meshTriangles) {
                    if (!isPointBehindTrianglesPlane(point, triangle.v0, triangle.v1, triangle.v2)) {
                        // it's not behind at least one so we bail
                        insideMesh = false;
                        break;
                    }

                }
                if (insideMesh) {
                    // It's inside this mesh, return true.
                    _mutex.unlock();
                    return true;
                }
            }
            subMeshIndex++;
        }
        _mutex.unlock();
    }
    // It wasn't in any mesh, return false.
    return false;
}

// TODO: we seem to call this too often when things haven't actually changed... look into optimizing this
// Any script might trigger findRayIntersectionAgainstSubMeshes (and maybe convexHullContains), so these
// can occur multiple times. In addition, rendering does it's own ray picking in order to decide which
// entity-scripts to call.  I think it would be best to do the picking once-per-frame (in cpu, or gpu if possible)
// and then the calls use the most recent such result.
void Model::recalculateMeshBoxes(bool pickAgainstTriangles) {
    PROFILE_RANGE(__FUNCTION__);
    bool calculatedMeshTrianglesNeeded = pickAgainstTriangles && !_calculatedMeshTrianglesValid;

    if (!_calculatedMeshBoxesValid || calculatedMeshTrianglesNeeded || (!_calculatedMeshPartBoxesValid && pickAgainstTriangles) ) {
        const FBXGeometry& geometry = getFBXGeometry();
        int numberOfMeshes = geometry.meshes.size();
        _calculatedMeshBoxes.resize(numberOfMeshes);
        _calculatedMeshTriangles.clear();
        _calculatedMeshTriangles.resize(numberOfMeshes);
        _calculatedMeshPartBoxes.clear();
        for (int i = 0; i < numberOfMeshes; i++) {
            const FBXMesh& mesh = geometry.meshes.at(i);
            Extents scaledMeshExtents = calculateScaledOffsetExtents(mesh.meshExtents, _translation, _rotation);

            _calculatedMeshBoxes[i] = AABox(scaledMeshExtents);

            if (pickAgainstTriangles) {
                QVector<Triangle> thisMeshTriangles;
                for (int j = 0; j < mesh.parts.size(); j++) {
                    const FBXMeshPart& part = mesh.parts.at(j);

                    bool atLeastOnePointInBounds = false;
                    AABox thisPartBounds;

                    const int INDICES_PER_TRIANGLE = 3;
                    const int INDICES_PER_QUAD = 4;

                    if (part.quadIndices.size() > 0) {
                        int numberOfQuads = part.quadIndices.size() / INDICES_PER_QUAD;
                        int vIndex = 0;
                        for (int q = 0; q < numberOfQuads; q++) {
                            int i0 = part.quadIndices[vIndex++];
                            int i1 = part.quadIndices[vIndex++];
                            int i2 = part.quadIndices[vIndex++];
                            int i3 = part.quadIndices[vIndex++];

                            glm::vec3 mv0 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i0], 1.0f));
                            glm::vec3 mv1 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i1], 1.0f));
                            glm::vec3 mv2 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i2], 1.0f));
                            glm::vec3 mv3 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i3], 1.0f));

                            // track the mesh parts in model space
                            if (!atLeastOnePointInBounds) {
                                thisPartBounds.setBox(mv0, 0.0f);
                                atLeastOnePointInBounds = true;
                            } else {
                                thisPartBounds += mv0;
                            }
                            thisPartBounds += mv1;
                            thisPartBounds += mv2;
                            thisPartBounds += mv3;

                            glm::vec3 v0 = calculateScaledOffsetPoint(mv0);
                            glm::vec3 v1 = calculateScaledOffsetPoint(mv1);
                            glm::vec3 v2 = calculateScaledOffsetPoint(mv2);
                            glm::vec3 v3 = calculateScaledOffsetPoint(mv3);

                            // Sam's recommended triangle slices
                            Triangle tri1 = { v0, v1, v3 };
                            Triangle tri2 = { v1, v2, v3 };

                            // NOTE: Random guy on the internet's recommended triangle slices
                            //Triangle tri1 = { v0, v1, v2 };
                            //Triangle tri2 = { v2, v3, v0 };

                            thisMeshTriangles.push_back(tri1);
                            thisMeshTriangles.push_back(tri2);

                        }
                    }

                    if (part.triangleIndices.size() > 0) {
                        int numberOfTris = part.triangleIndices.size() / INDICES_PER_TRIANGLE;
                        int vIndex = 0;
                        for (int t = 0; t < numberOfTris; t++) {
                            int i0 = part.triangleIndices[vIndex++];
                            int i1 = part.triangleIndices[vIndex++];
                            int i2 = part.triangleIndices[vIndex++];

                            glm::vec3 mv0 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i0], 1.0f));
                            glm::vec3 mv1 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i1], 1.0f));
                            glm::vec3 mv2 = glm::vec3(mesh.modelTransform * glm::vec4(mesh.vertices[i2], 1.0f));

                            // track the mesh parts in model space
                            if (!atLeastOnePointInBounds) {
                                thisPartBounds.setBox(mv0, 0.0f);
                                atLeastOnePointInBounds = true;
                            } else {
                                thisPartBounds += mv0;
                            }
                            thisPartBounds += mv1;
                            thisPartBounds += mv2;

                            glm::vec3 v0 = calculateScaledOffsetPoint(mv0);
                            glm::vec3 v1 = calculateScaledOffsetPoint(mv1);
                            glm::vec3 v2 = calculateScaledOffsetPoint(mv2);

                            Triangle tri = { v0, v1, v2 };

                            thisMeshTriangles.push_back(tri);
                        }
                    }
                    _calculatedMeshPartBoxes[QPair<int,int>(i, j)] = thisPartBounds;
                }
                _calculatedMeshTriangles[i] = thisMeshTriangles;
                _calculatedMeshPartBoxesValid = true;
            }
        }
        _calculatedMeshBoxesValid = true;
        _calculatedMeshTrianglesValid = pickAgainstTriangles;
    }
}

void Model::renderSetup(RenderArgs* args) {
    // set up dilated textures on first render after load/simulate
    const FBXGeometry& geometry = getFBXGeometry();
    if (_dilatedTextures.isEmpty()) {
        foreach (const FBXMesh& mesh, geometry.meshes) {
            QVector<QSharedPointer<Texture> > dilated;
            dilated.resize(mesh.parts.size());
            _dilatedTextures.append(dilated);
        }
    }

    if (!_meshGroupsKnown && isLoaded()) {
        segregateMeshGroups();
    }
}

void Model::setVisibleInScene(bool newValue, std::shared_ptr<render::Scene> scene) {
    if (_isVisible != newValue) {
        _isVisible = newValue;

        render::PendingChanges pendingChanges;
        foreach (auto item, _modelMeshRenderItems.keys()) {
            pendingChanges.resetItem(item, _modelMeshRenderItems[item]);
        }
        foreach (auto item, _collisionRenderItems.keys()) {
            pendingChanges.resetItem(item, _collisionRenderItems[item]);
        }
        scene->enqueuePendingChanges(pendingChanges);
    }
}

bool Model::addToScene(std::shared_ptr<render::Scene> scene,
                       render::PendingChanges& pendingChanges,
                       render::Item::Status::Getters& statusGetters,
                       bool showCollisionHull) {
    if ((!_meshGroupsKnown || showCollisionHull != _showCollisionHull) && isLoaded()) {
        _showCollisionHull = showCollisionHull;
        segregateMeshGroups();
    }

    bool somethingAdded = false;

    if (_modelMeshRenderItems.empty()) {
        foreach (auto renderItem, _modelMeshRenderItemsSet) {
            auto item = scene->allocateID();
            auto renderPayload = std::make_shared<ModelMeshPartPayload::Payload>(renderItem);
            if (statusGetters.size()) {
                renderPayload->addStatusGetters(statusGetters);
            }
            pendingChanges.resetItem(item, renderPayload);
            _modelMeshRenderItems.insert(item, renderPayload);
            somethingAdded = true;
        }
    }
    if (_collisionRenderItems.empty()) {
        foreach (auto renderItem, _collisionRenderItemsSet) {
            auto item = scene->allocateID();
            auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderItem);
            if (statusGetters.size()) {
                renderPayload->addStatusGetters(statusGetters);
            }
            pendingChanges.resetItem(item, renderPayload);
            _collisionRenderItems.insert(item, renderPayload);
            somethingAdded = true;
        }
    }

    updateRenderItems();

    _readyWhenAdded = readyToAddToScene();

    return somethingAdded;
}

void Model::removeFromScene(std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges) {
    foreach (auto item, _modelMeshRenderItems.keys()) {
        pendingChanges.removeItem(item);
    }
    _modelMeshRenderItems.clear();
    _modelMeshRenderItemsSet.clear();
    foreach (auto item, _collisionRenderItems.keys()) {
        pendingChanges.removeItem(item);
    }
    _collisionRenderItems.clear();
    _collisionRenderItemsSet.clear();
    _meshGroupsKnown = false;
    _readyWhenAdded = false;
}

void Model::renderDebugMeshBoxes(gpu::Batch& batch) {
    int colorNdx = 0;
    _mutex.lock();
    foreach(AABox box, _calculatedMeshBoxes) {
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
            { 1.0f, 0.0f, 0.0f, 1.0f }, // red
            { 0.0f, 1.0f, 0.0f, 1.0f }, // green
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

Extents Model::calculateScaledOffsetExtents(const Extents& extents,
                                            glm::vec3 modelPosition, glm::quat modelOrientation) const {
    // we need to include any fst scaling, translation, and rotation, which is captured in the offset matrix
    glm::vec3 minimum = glm::vec3(getFBXGeometry().offset * glm::vec4(extents.minimum, 1.0f));
    glm::vec3 maximum = glm::vec3(getFBXGeometry().offset * glm::vec4(extents.maximum, 1.0f));

    Extents scaledOffsetExtents = { ((minimum + _offset) * _scale),
                                    ((maximum + _offset) * _scale) };

    Extents rotatedExtents = scaledOffsetExtents.getRotated(modelOrientation);

    Extents translatedExtents = { rotatedExtents.minimum + modelPosition,
                                  rotatedExtents.maximum + modelPosition };

    return translatedExtents;
}

/// Returns the world space equivalent of some box in model space.
AABox Model::calculateScaledOffsetAABox(const AABox& box, glm::vec3 modelPosition, glm::quat modelOrientation) const {
    return AABox(calculateScaledOffsetExtents(Extents(box), modelPosition, modelOrientation));
}

glm::vec3 Model::calculateScaledOffsetPoint(const glm::vec3& point) const {
    // we need to include any fst scaling, translation, and rotation, which is captured in the offset matrix
    glm::vec3 offsetPoint = glm::vec3(getFBXGeometry().offset * glm::vec4(point, 1.0f));
    glm::vec3 scaledPoint = ((offsetPoint + _offset) * _scale);
    glm::vec3 rotatedPoint = _rotation * scaledPoint;
    glm::vec3 translatedPoint = rotatedPoint + _translation;
    return translatedPoint;
}

void Model::clearJointState(int index) {
    _rig->clearJointState(index);
}

void Model::setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority) {
    _rig->setJointState(index, valid, rotation, translation, priority);
}

void Model::setJointRotation(int index, bool valid, const glm::quat& rotation, float priority) {
    _rig->setJointRotation(index, valid, rotation, priority);
}

void Model::setJointTranslation(int index, bool valid, const glm::vec3& translation, float priority) {
    _rig->setJointTranslation(index, valid, translation, priority);
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
        _renderGeometry->setTextures(textures);
    }
}

void Model::setURL(const QUrl& url) {
    // don't recreate the geometry if it's the same URL
    if (_url == url && _renderWatcher.getURL() == url) {
        return;
    }

    _url = url;

    {
        render::PendingChanges pendingChanges;
        render::ScenePointer scene = AbstractViewStateInterface::instance()->getMain3DScene();
        removeFromScene(scene, pendingChanges);
        scene->enqueuePendingChanges(pendingChanges);
    }

    _needsReload = true;
    _needsUpdateTextures = true;
    _meshGroupsKnown = false;
    _geometryRequestFailed = false;
    invalidCalculatedMeshBoxes();
    deleteGeometry();

    _renderWatcher.setResource(DependencyManager::get<ModelCache>()->getGeometryResource(url));
    onInvalidate();
}

void Model::setCollisionModelURL(const QUrl& url) {
    if (_collisionUrl == url && _collisionWatcher.getURL() == url) {
        return;
    }
    _collisionUrl = url;
    _collisionWatcher.setResource(DependencyManager::get<ModelCache>()->getGeometryResource(url));
}

bool Model::getJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const {
    return _rig->getJointPositionInWorldFrame(jointIndex, position, _translation, _rotation);
}

bool Model::getJointPosition(int jointIndex, glm::vec3& position) const {
    return _rig->getJointPosition(jointIndex, position);
}

bool Model::getJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const {
    return _rig->getJointRotationInWorldFrame(jointIndex, rotation, _rotation);
}

bool Model::getJointRotation(int jointIndex, glm::quat& rotation) const {
    return _rig->getJointRotation(jointIndex, rotation);
}

bool Model::getJointTranslation(int jointIndex, glm::vec3& translation) const {
    return _rig->getJointTranslation(jointIndex, translation);
}

bool Model::getAbsoluteJointRotationInRigFrame(int jointIndex, glm::quat& rotationOut) const {
    return _rig->getAbsoluteJointRotationInRigFrame(jointIndex, rotationOut);
}

bool Model::getAbsoluteJointTranslationInRigFrame(int jointIndex, glm::vec3& translationOut) const {
    return _rig->getAbsoluteJointTranslationInRigFrame(jointIndex, translationOut);
}

bool Model::getRelativeDefaultJointRotation(int jointIndex, glm::quat& rotationOut) const {
    return _rig->getRelativeDefaultJointRotation(jointIndex, rotationOut);
}

bool Model::getRelativeDefaultJointTranslation(int jointIndex, glm::vec3& translationOut) const {
    return _rig->getRelativeDefaultJointTranslation(jointIndex, translationOut);
}

bool Model::getJointCombinedRotation(int jointIndex, glm::quat& rotation) const {
    return _rig->getJointCombinedRotation(jointIndex, rotation, _rotation);
}

QStringList Model::getJointNames() const {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        QMetaObject::invokeMethod(const_cast<Model*>(this), "getJointNames", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QStringList, result));
        return result;
    }
    return isActive() ? getFBXGeometry().getJointNames() : QStringList();
}

class Blender : public QRunnable {
public:

    Blender(ModelPointer model, int blendNumber, const Geometry::WeakPointer& geometry,
        const QVector<FBXMesh>& meshes, const QVector<float>& blendshapeCoefficients);

    virtual void run();

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
    PROFILE_RANGE(__FUNCTION__);
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

void Model::setScaleToFit(bool scaleToFit, const glm::vec3& dimensions) {
    if (_scaleToFit != scaleToFit || _scaleToFitDimensions != dimensions) {
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
    PROFILE_RANGE(__FUNCTION__);
    fullUpdate = updateGeometry() || fullUpdate || (_scaleToFit && !_scaledToFit)
                    || (_snapModelToRegistrationPoint && !_snappedToRegistrationPoint);

    if (isActive() && fullUpdate) {
        // NOTE: This is overly aggressive and we are invalidating the MeshBoxes when in fact they may not be invalid
        //       they really only become invalid if something about the transform to world space has changed. This is
        //       not too bad at this point, because it doesn't impact rendering. However it does slow down ray picking
        //       because ray picking needs valid boxes to work
        _calculatedMeshBoxesValid = false;
        _calculatedMeshTrianglesValid = false;
        onInvalidate();

        // check for scale to fit
        if (_scaleToFit && !_scaledToFit) {
            scaleToFit();
        }
        if (_snapModelToRegistrationPoint && !_snappedToRegistrationPoint) {
            snapToRegistrationPoint();
        }
        simulateInternal(deltaTime);
    }
}

//virtual
void Model::updateRig(float deltaTime, glm::mat4 parentTransform) {
    _needsUpdateClusterMatrices = true;
    _rig->updateAnimations(deltaTime, parentTransform);
}

void Model::simulateInternal(float deltaTime) {
    // update the world space transforms for all joints
    glm::mat4 parentTransform = glm::scale(_scale) * glm::translate(_offset);
    updateRig(deltaTime, parentTransform);
}

// virtual
void Model::updateClusterMatrices(glm::vec3 modelPosition, glm::quat modelOrientation) {
    PerformanceTimer perfTimer("Model::updateClusterMatrices");

    if (!_needsUpdateClusterMatrices || !isLoaded()) {
        return;
    }
    _needsUpdateClusterMatrices = false;
    const FBXGeometry& geometry = getFBXGeometry();
    glm::mat4 zeroScale(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    auto cauterizeMatrix = _rig->getJointTransform(geometry.neckJointIndex) * zeroScale;

    glm::mat4 modelToWorld = glm::mat4_cast(modelOrientation);
    for (int i = 0; i < _meshStates.size(); i++) {
        MeshState& state = _meshStates[i];
        const FBXMesh& mesh = geometry.meshes.at(i);

        for (int j = 0; j < mesh.clusters.size(); j++) {
            const FBXCluster& cluster = mesh.clusters.at(j);
            auto jointMatrix = _rig->getJointTransform(cluster.jointIndex);
            state.clusterMatrices[j] = modelToWorld * jointMatrix * cluster.inverseBindMatrix;

            // as an optimization, don't build cautrizedClusterMatrices if the boneSet is empty.
            if (!_cauterizeBoneSet.empty()) {
                if (_cauterizeBoneSet.find(cluster.jointIndex) != _cauterizeBoneSet.end()) {
                    jointMatrix = cauterizeMatrix;
                }
                state.cauterizedClusterMatrices[j] = modelToWorld * jointMatrix * cluster.inverseBindMatrix;
            }
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

            if (!_cauterizeBoneSet.empty() && (state.cauterizedClusterMatrices.size() > 1)) {
                if (!state.cauterizedClusterBuffer) {
                    state.cauterizedClusterBuffer =
                        std::make_shared<gpu::Buffer>(state.cauterizedClusterMatrices.size() * sizeof(glm::mat4),
                                                      (const gpu::Byte*) state.cauterizedClusterMatrices.constData());
                } else {
                    state.cauterizedClusterBuffer->setSubData(0, state.cauterizedClusterMatrices.size() * sizeof(glm::mat4),
                                                              (const gpu::Byte*) state.cauterizedClusterMatrices.constData());
                }
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
    _rig->inverseKinematics(endIndex, targetPosition, targetRotation, priority, freeLineage, parentTransform);
}

bool Model::restoreJointPosition(int jointIndex, float fraction, float priority) {
    const FBXGeometry& geometry = getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    return _rig->restoreJointPosition(jointIndex, fraction, priority, freeLineage);
}

float Model::getLimbLength(int jointIndex) const {
    const FBXGeometry& geometry = getFBXGeometry();
    const QVector<int>& freeLineage = geometry.joints.at(jointIndex).freeLineage;
    return _rig->getLimbLength(jointIndex, freeLineage, _scale, geometry.joints);
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
    _rig->destroyAnimGraph();
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
        for (auto& renderItem : _modelMeshRenderItemsSet) {
            totalBound += renderItem->getBound();
        }
        return totalBound;
    }
}

void Model::segregateMeshGroups() {
    Geometry::Pointer geometry;
    bool showingCollisionHull = false;
    if (_showCollisionHull && _collisionGeometry) {
        if (isCollisionLoaded()) {
            geometry = _collisionGeometry;
            showingCollisionHull = true;
        } else {
            return;
        }
    } else {
        assert(isLoaded());
        geometry = _renderGeometry;
    }
    const auto& meshes = geometry->getMeshes();

    // all of our mesh vectors must match in size
    if ((int)meshes.size() != _meshStates.size()) {
        qDebug() << "WARNING!!!! Mesh Sizes don't match! We will not segregate mesh groups yet.";
        return;
    }

    // We should not have any existing renderItems if we enter this section of code
    Q_ASSERT(_modelMeshRenderItems.isEmpty());
    Q_ASSERT(_modelMeshRenderItemsSet.isEmpty());
    Q_ASSERT(_collisionRenderItems.isEmpty());
    Q_ASSERT(_collisionRenderItemsSet.isEmpty());

    _modelMeshRenderItemsSet.clear();
    _collisionRenderItemsSet.clear();

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
        if (mesh) {

            // Create the render payloads
            int numParts = (int)mesh->getNumParts();
            for (int partIndex = 0; partIndex < numParts; partIndex++) {
                if (showingCollisionHull) {
                    if (_collisionHullMaterials.empty()) {
                        initCollisionHullMaterials();
                    }
                    _collisionRenderItemsSet << std::make_shared<MeshPartPayload>(mesh, partIndex, _collisionHullMaterials[partIndex % NUM_COLLISION_HULL_COLORS], transform, offset);
                } else {
                    _modelMeshRenderItemsSet << std::make_shared<ModelMeshPartPayload>(this, i, partIndex, shapeID, transform, offset);
                }

                shapeID++;
            }
        }
    }
    _meshGroupsKnown = true;
}

bool Model::initWhenReady(render::ScenePointer scene) {
    if (isActive() && isRenderable() && !_meshGroupsKnown && isLoaded()) {
        segregateMeshGroups();

        render::PendingChanges pendingChanges;

        Transform transform;
        transform.setTranslation(_translation);
        transform.setRotation(_rotation);

        Transform offset;
        offset.setScale(_scale);
        offset.postTranslate(_offset);

        foreach (auto renderItem, _modelMeshRenderItemsSet) {
            auto item = scene->allocateID();
            auto renderPayload = std::make_shared<ModelMeshPartPayload::Payload>(renderItem);
            _modelMeshRenderItems.insert(item, renderPayload);
            pendingChanges.resetItem(item, renderPayload);
        }
        foreach (auto renderItem, _collisionRenderItemsSet) {
            auto item = scene->allocateID();
            auto renderPayload = std::make_shared<MeshPartPayload::Payload>(renderItem);
            _collisionRenderItems.insert(item, renderPayload);
            pendingChanges.resetItem(item, renderPayload);
        }
        scene->enqueuePendingChanges(pendingChanges);
        updateRenderItems();

        _readyWhenAdded = true;
        return true;
    }
    return false;
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

