//
//  Model.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Model_h
#define hifi_Model_h

#include <QBitArray>
#include <QObject>
#include <QUrl>
#include <QMutex>

#include <unordered_map>
#include <unordered_set>
#include <functional>

#include <AABox.h>
#include <DependencyManager.h>
#include <GeometryUtil.h>
#include <gpu/Batch.h>
#include <render/Scene.h>
#include <Transform.h>

#include "GeometryCache.h"
#include "TextureCache.h"
#include "Rig.h"

class AbstractViewStateInterface;
class QScriptEngine;

#include "RenderArgs.h"
class ViewFrustum;

namespace render {
    class Scene;
    class PendingChanges;
    typedef unsigned int ItemID;
}
class MeshPartPayload;
class ModelMeshPartPayload;
class ModelRenderLocations;

inline uint qHash(const std::shared_ptr<MeshPartPayload>& a, uint seed) {
    return qHash(a.get(), seed);
}

class Model;
using ModelPointer = std::shared_ptr<Model>;
using ModelWeakPointer = std::weak_ptr<Model>;


/// A generic 3D model displaying geometry loaded from a URL.
class Model : public QObject, public std::enable_shared_from_this<Model> {
    Q_OBJECT

public:

    typedef RenderArgs::RenderMode RenderMode;

    static void setAbstractViewStateInterface(AbstractViewStateInterface* viewState) { _viewState = viewState; }

    Model(RigPointer rig, QObject* parent = nullptr);
    virtual ~Model();

    inline ModelPointer getThisPointer() const {
        return std::static_pointer_cast<Model>(std::const_pointer_cast<Model>(shared_from_this()));
    }

    /// Sets the URL of the model to render.
    // Should only be called from the model's rendering thread to avoid access violations of changed geometry.
    Q_INVOKABLE void setURL(const QUrl& url);
    const QUrl& getURL() const { return _url; }

    // new Scene/Engine rendering support
    void setVisibleInScene(bool newValue, std::shared_ptr<render::Scene> scene);
    bool needsFixupInScene() const;

    bool needsReload() const { return _needsReload; }
    bool initWhenReady(render::ScenePointer scene);
    bool addToScene(std::shared_ptr<render::Scene> scene,
                    render::PendingChanges& pendingChanges) {
        auto getters = render::Item::Status::Getters(0);
        return addToScene(scene, pendingChanges, getters);
    }
    bool addToScene(std::shared_ptr<render::Scene> scene,
                    render::PendingChanges& pendingChanges,
                    render::Item::Status::Getters& statusGetters);
    void removeFromScene(std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    void renderSetup(RenderArgs* args);
    bool isRenderable() const;

    bool isVisible() const { return _isVisible; }

    void updateRenderItems();
    void setRenderItemsNeedUpdate() { _renderItemsNeedUpdate = true; }
    bool getRenderItemsNeedUpdate() { return _renderItemsNeedUpdate; }
    AABox getRenderableMeshBound() const;

    bool maybeStartBlender();

    /// Sets blended vertices computed in a separate thread.
    void setBlendedVertices(int blendNumber, const Geometry::WeakPointer& geometry,
        const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals);

    bool isLoaded() const { return (bool)_renderGeometry; }

    void setIsWireframe(bool isWireframe) { _isWireframe = isWireframe; }
    bool isWireframe() const { return _isWireframe; }

    void init();
    void reset();

    void setScaleToFit(bool scaleToFit, const glm::vec3& dimensions);

    void setSnapModelToRegistrationPoint(bool snapModelToRegistrationPoint, const glm::vec3& registrationPoint);
    bool getSnapModelToRegistrationPoint() { return _snapModelToRegistrationPoint; }

    virtual void simulate(float deltaTime, bool fullUpdate = true);
    virtual void updateClusterMatrices(glm::vec3 modelPosition, glm::quat modelOrientation);

    /// Returns a reference to the shared geometry.
    const Geometry::Pointer& getGeometry() const { return _renderGeometry; }
    /// Returns a reference to the shared collision geometry.
    const Geometry::Pointer& getCollisionGeometry() const { return _collisionGeometry; }

    const QVariantMap getTextures() const { assert(isLoaded()); return _renderGeometry->getTextures(); }
    void setTextures(const QVariantMap& textures);

    /// Provided as a convenience, will crash if !isLoaded()
    // And so that getGeometry() isn't chained everywhere
    const FBXGeometry& getFBXGeometry() const { assert(isLoaded()); return _renderGeometry->getFBXGeometry(); }

    bool isActive() const { return isLoaded(); }

    bool didVisualGeometryRequestFail() const { return _visualGeometryRequestFailed; }
    bool didCollisionGeometryRequestFail() const { return _collisionGeometryRequestFailed; }

    bool convexHullContains(glm::vec3 point);

    QStringList getJointNames() const;

    /// Sets the joint state at the specified index.
    void setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority);
    void setJointRotation(int index, bool valid, const glm::quat& rotation, float priority);
    void setJointTranslation(int index, bool valid, const glm::vec3& translation, float priority);

    bool findRayIntersectionAgainstSubMeshes(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal, 
                                             QString& extraInfo, bool pickAgainstTriangles = false);

    void setOffset(const glm::vec3& offset);
    const glm::vec3& getOffset() const { return _offset; }

    void setScaleToFit(bool scaleToFit, float largestDimension = 0.0f, bool forceRescale = false);
    bool getScaleToFit() const { return _scaleToFit; } /// is scale to fit enabled

    void setSnapModelToCenter(bool snapModelToCenter) {
        setSnapModelToRegistrationPoint(snapModelToCenter, glm::vec3(0.5f,0.5f,0.5f));
    };
    bool getSnapModelToCenter() {
        return _snapModelToRegistrationPoint && _registrationPoint == glm::vec3(0.5f,0.5f,0.5f);
    }

    /// Returns the number of joint states in the model.
    int getJointStateCount() const { return (int)_rig->getJointStateCount(); }
    bool getJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const;
    bool getJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const;
    bool getJointCombinedRotation(int jointIndex, glm::quat& rotation) const;

    /// \param jointIndex index of joint in model structure
    /// \param rotation[out] rotation of joint in model-frame
    /// \return true if joint exists
    bool getJointRotation(int jointIndex, glm::quat& rotation) const;
    bool getJointTranslation(int jointIndex, glm::vec3& translation) const;

    // model frame
    bool getAbsoluteJointRotationInRigFrame(int jointIndex, glm::quat& rotationOut) const;
    bool getAbsoluteJointTranslationInRigFrame(int jointIndex, glm::vec3& translationOut) const;

    bool getRelativeDefaultJointRotation(int jointIndex, glm::quat& rotationOut) const;
    bool getRelativeDefaultJointTranslation(int jointIndex, glm::vec3& translationOut) const;

    /// Returns the index of the parent of the indexed joint, or -1 if not found.
    int getParentJointIndex(int jointIndex) const;

    void inverseKinematics(int jointIndex, glm::vec3 position, const glm::quat& rotation, float priority);

    /// Returns the extents of the model in its bind pose.
    Extents getBindExtents() const;

    /// Returns the extents of the model's mesh
    Extents getMeshExtents() const;

    void setTranslation(const glm::vec3& translation);
    void setRotation(const glm::quat& rotation);

    const glm::vec3& getTranslation() const { return _translation; }
    const glm::quat& getRotation() const { return _rotation; }

    void setScale(const glm::vec3& scale);
    const glm::vec3& getScale() const { return _scale; }

    /// enables/disables scale to fit behavior, the model will be automatically scaled to the specified largest dimension
    bool getIsScaledToFit() const { return _scaledToFit; } /// is model scaled to fit
    glm::vec3 getScaleToFitDimensions() const; /// the dimensions model is scaled to, including inferred y/z

    void setCauterizeBones(bool flag) { _cauterizeBones = flag; }
    bool getCauterizeBones() const { return _cauterizeBones; }

    const std::unordered_set<int>& getCauterizeBoneSet() const { return _cauterizeBoneSet; }
    void setCauterizeBoneSet(const std::unordered_set<int>& boneSet) { _cauterizeBoneSet = boneSet; }

    int getBlendshapeCoefficientsNum() const { return _blendshapeCoefficients.size(); }
    float getBlendshapeCoefficient(int index) const {
        return ((index < 0) && (index >= _blendshapeCoefficients.size())) ? 0.0f : _blendshapeCoefficients.at(index);
     }

    virtual RigPointer getRig() const { return _rig; }

    const glm::vec3& getRegistrationPoint() const { return _registrationPoint; }

    // returns 'true' if needs fullUpdate after geometry change
    bool updateGeometry();
    void setCollisionMesh(model::MeshPointer mesh);

    void setLoadingPriority(float priority) { _loadingPriority = priority; }

    size_t getRenderInfoVertexCount() const { return _renderInfoVertexCount; }
    size_t getRenderInfoTextureSize();
    int getRenderInfoTextureCount();
    int getRenderInfoDrawCalls() const { return _renderInfoDrawCalls; }
    bool getRenderInfoHasTransparent() const { return _renderInfoHasTransparent; }

public slots:
    void loadURLFinished(bool success);

signals:
    void setURLFinished(bool success);
    void setCollisionModelURLFinished(bool success);

protected:
    bool addedToScene() const { return _addedToScene; }

    void setPupilDilation(float dilation) { _pupilDilation = dilation; }
    float getPupilDilation() const { return _pupilDilation; }

    void setBlendshapeCoefficients(const QVector<float>& coefficients) { _blendshapeCoefficients = coefficients; }
    const QVector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }

    /// Returns the unscaled extents of the model's mesh
    Extents getUnscaledMeshExtents() const;

    /// Returns the scaled equivalent of some extents in model space.
    Extents calculateScaledOffsetExtents(const Extents& extents, glm::vec3 modelPosition, glm::quat modelOrientation) const;

    /// Returns the world space equivalent of some box in model space.
    AABox calculateScaledOffsetAABox(const AABox& box, glm::vec3 modelPosition, glm::quat modelOrientation) const;

    /// Returns the scaled equivalent of a point in model space.
    glm::vec3 calculateScaledOffsetPoint(const glm::vec3& point) const;

    /// Clear the joint states
    void clearJointState(int index);

    /// Returns the index of the last free ancestor of the indexed joint, or -1 if not found.
    int getLastFreeJointIndex(int jointIndex) const;

    /// \param jointIndex index of joint in model structure
    /// \param position[out] position of joint in model-frame
    /// \return true if joint exists
    bool getJointPosition(int jointIndex, glm::vec3& position) const;

    Geometry::Pointer _renderGeometry; // only ever set by its watcher
    Geometry::Pointer _collisionGeometry;

    GeometryResourceWatcher _renderWatcher;

    glm::vec3 _translation;
    glm::quat _rotation;
    glm::vec3 _scale;
    glm::vec3 _offset;

    static float FAKE_DIMENSION_PLACEHOLDER;

    bool _scaleToFit; /// If you set scaleToFit, we will calculate scale based on MeshExtents
    glm::vec3 _scaleToFitDimensions; /// this is the dimensions that scale to fit will use
    bool _scaledToFit; /// have we scaled to fit

    bool _snapModelToRegistrationPoint; /// is the model's offset automatically adjusted to a registration point in model space
    bool _snappedToRegistrationPoint; /// are we currently snapped to a registration point
    glm::vec3 _registrationPoint = glm::vec3(0.5f); /// the point in model space our center is snapped to

    class MeshState {
    public:
        QVector<glm::mat4> clusterMatrices;
        QVector<glm::mat4> cauterizedClusterMatrices;
        gpu::BufferPointer clusterBuffer;
        gpu::BufferPointer cauterizedClusterBuffer;

    };

    QVector<MeshState> _meshStates;
    std::unordered_set<int> _cauterizeBoneSet;
    bool _cauterizeBones;

    virtual void initJointStates();

    void setScaleInternal(const glm::vec3& scale);
    void scaleToFit();
    void snapToRegistrationPoint();

    void simulateInternal(float deltaTime);
    virtual void updateRig(float deltaTime, glm::mat4 parentTransform);

    /// Restores the indexed joint to its default position.
    /// \param fraction the fraction of the default position to apply (i.e., 0.25f to slerp one fourth of the way to
    /// the original position
    /// \return true if the joint was found
    bool restoreJointPosition(int jointIndex, float fraction = 1.0f, float priority = 0.0f);

    /// Computes and returns the extended length of the limb terminating at the specified joint and starting at the joint's
    /// first free ancestor.
    float getLimbLength(int jointIndex) const;

    /// Allow sub classes to force invalidating the bboxes
    void invalidCalculatedMeshBoxes() {
        _calculatedMeshBoxesValid = false;
        _calculatedMeshPartBoxesValid = false;
        _calculatedMeshTrianglesValid = false;
    }

    // hook for derived classes to be notified when setUrl invalidates the current model.
    virtual void onInvalidate() {};

protected:

    void deleteGeometry();
    void initJointTransforms();

    float _pupilDilation;
    QVector<float> _blendshapeCoefficients;

    QUrl _url;
    bool _isVisible;

    gpu::Buffers _blendedVertexBuffers;

    QVector<QVector<QSharedPointer<Texture> > > _dilatedTextures;

    QVector<float> _blendedBlendshapeCoefficients;
    int _blendNumber;
    int _appliedBlendNumber;

    QHash<QPair<int,int>, AABox> _calculatedMeshPartBoxes; // world coordinate AABoxes for all sub mesh part boxes

    bool _calculatedMeshPartBoxesValid;
    QVector<AABox> _calculatedMeshBoxes; // world coordinate AABoxes for all sub mesh boxes
    bool _calculatedMeshBoxesValid;

    QVector< QVector<Triangle> > _calculatedMeshTriangles; // world coordinate triangles for all sub meshes
    bool _calculatedMeshTrianglesValid;
    QMutex _mutex;

    void recalculateMeshBoxes(bool pickAgainstTriangles = false);

    void createRenderItemSet();
    void createVisibleRenderItemSet();
    void createCollisionRenderItemSet();

    bool _isWireframe;


    // debug rendering support
    void renderDebugMeshBoxes(gpu::Batch& batch);
    int _debugMeshBoxesID = GeometryCache::UNKNOWN_ID;


    static AbstractViewStateInterface* _viewState;

    QSet<std::shared_ptr<MeshPartPayload>> _collisionRenderItemsSet;
    QMap<render::ItemID, render::PayloadPointer> _collisionRenderItems;

    QSet<std::shared_ptr<ModelMeshPartPayload>> _modelMeshRenderItemsSet;
    QMap<render::ItemID, render::PayloadPointer> _modelMeshRenderItems;

    bool _addedToScene { false }; // has been added to scene
    bool _needsFixupInScene { true }; // needs to be removed/re-added to scene
    bool _needsReload { true };
    bool _needsUpdateClusterMatrices { true };
    mutable bool _needsUpdateTextures { true };

    friend class ModelMeshPartPayload;
    RigPointer _rig;

    uint32_t _deleteGeometryCounter { 0 };

    bool _visualGeometryRequestFailed { false };
    bool _collisionGeometryRequestFailed { false };

    bool _renderItemsNeedUpdate { false };

    size_t _renderInfoVertexCount { 0 };
    int _renderInfoTextureCount { 0 };
    size_t _renderInfoTextureSize { 0 };
    bool _hasCalculatedTextureInfo { false };
    int _renderInfoDrawCalls { 0 };
    int _renderInfoHasTransparent { false };

private:
    float _loadingPriority { 0.0f };

    void calculateTextureInfo();
};

Q_DECLARE_METATYPE(ModelPointer)
Q_DECLARE_METATYPE(Geometry::WeakPointer)

/// Handle management of pending models that need blending
class ModelBlender : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:

    /// Adds the specified model to the list requiring vertex blends.
    void noteRequiresBlend(ModelPointer model);

public slots:
    void setBlendedVertices(ModelPointer model, int blendNumber, const Geometry::WeakPointer& geometry,
        const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals);

private:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    ModelBlender();
    virtual ~ModelBlender();

    std::set<ModelWeakPointer, std::owner_less<ModelWeakPointer>> _modelsRequiringBlends;
    int _pendingBlenders;
    Mutex _mutex;
};


#endif // hifi_Model_h
