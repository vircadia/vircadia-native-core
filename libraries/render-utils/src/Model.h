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

#include "AnimationHandle.h"
#include "GeometryCache.h"
#include "JointState.h"
#include "TextureCache.h"

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
class ModelRenderLocations;

inline uint qHash(const std::shared_ptr<MeshPartPayload>& a, uint seed) {
    return qHash(a.get(), seed);
}

/// A generic 3D model displaying geometry loaded from a URL.
class Model : public QObject {
    Q_OBJECT

public:

    typedef RenderArgs::RenderMode RenderMode;

    static void setAbstractViewStateInterface(AbstractViewStateInterface* viewState) { _viewState = viewState; }

    Model(RigPointer rig, QObject* parent = nullptr);
    virtual ~Model();


    /// Sets the URL of the model to render.
    Q_INVOKABLE void setURL(const QUrl& url);
    const QUrl& getURL() const { return _url; }

    // new Scene/Engine rendering support
    void setVisibleInScene(bool newValue, std::shared_ptr<render::Scene> scene);
    bool needsFixupInScene() { return !_readyWhenAdded && readyToAddToScene(); }
    bool readyToAddToScene(RenderArgs* renderArgs = nullptr) {
        return !_needsReload && isRenderable() && isActive() && isLoaded();
    }
    bool initWhenReady(render::ScenePointer scene);
    bool addToScene(std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    bool addToScene(std::shared_ptr<render::Scene> scene,
                    render::PendingChanges& pendingChanges,
                    render::Item::Status::Getters& statusGetters);
    void removeFromScene(std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    void renderSetup(RenderArgs* args);
    bool isRenderable() const { return !_meshStates.isEmpty() || (isActive() && _geometry->getMeshes().empty()); }

    bool isVisible() const { return _isVisible; }

    AABox getPartBounds(int meshIndex, int partIndex);

    bool maybeStartBlender();

    /// Sets blended vertices computed in a separate thread.
    void setBlendedVertices(int blendNumber, const QWeakPointer<NetworkGeometry>& geometry,
        const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals);

    bool isLoaded() const { return _geometry && _geometry->isLoaded(); }
    bool isLoadedWithTextures() const { return _geometry && _geometry->isLoadedWithTextures(); }

    void setIsWireframe(bool isWireframe) { _isWireframe = isWireframe; }
    bool isWireframe() const { return _isWireframe; }

    void init();
    void reset();

    void setScaleToFit(bool scaleToFit, const glm::vec3& dimensions);

    void setSnapModelToRegistrationPoint(bool snapModelToRegistrationPoint, const glm::vec3& registrationPoint);
    bool getSnapModelToRegistrationPoint() { return _snapModelToRegistrationPoint; }

    virtual void simulate(float deltaTime, bool fullUpdate = true);
    void updateClusterMatrices();

    /// Returns a reference to the shared geometry.
    const QSharedPointer<NetworkGeometry>& getGeometry() const { return _geometry; }

    bool isActive() const { return _geometry && _geometry->isLoaded(); }

    Q_INVOKABLE void setTextureWithNameToURL(const QString& name, const QUrl& url)
        { _geometry->setTextureWithNameToURL(name, url); }

    bool convexHullContains(glm::vec3 point);

    QStringList getJointNames() const;

    /// Sets the joint state at the specified index.
    void setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority);
    void setJointRotation(int index, bool valid, const glm::quat& rotation, float priority);
    void setJointTranslation(int index, bool valid, const glm::vec3& translation, float priority);

    bool findRayIntersectionAgainstSubMeshes(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal, 
                                             QString& extraInfo, bool pickAgainstTriangles = false);

    // Set the model to use for collisions
    Q_INVOKABLE void setCollisionModelURL(const QUrl& url);
    const QUrl& getCollisionURL() const { return _collisionUrl; }

    /// Returns a reference to the shared collision geometry.
    const QSharedPointer<NetworkGeometry> getCollisionGeometry(bool delayLoad = false);

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
    int getJointStateCount() const { return _rig->getJointStateCount(); }
    bool getJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const;
    bool getJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const;
    bool getJointCombinedRotation(int jointIndex, glm::quat& rotation) const;
    /// \param jointIndex index of joint in model structure
    /// \param rotation[out] rotation of joint in model-frame
    /// \return true if joint exists
    bool getJointRotation(int jointIndex, glm::quat& rotation) const;
    bool getJointTranslation(int jointIndex, glm::vec3& translation) const;

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
    const glm::vec3& getScaleToFitDimensions() const { return _scaleToFitDimensions; } /// the dimensions model is scaled to

    void setCauterizeBones(bool flag) { _cauterizeBones = flag; }
    bool getCauterizeBones() const { return _cauterizeBones; }

    const std::unordered_set<int>& getCauterizeBoneSet() const { return _cauterizeBoneSet; }
    void setCauterizeBoneSet(const std::unordered_set<int>& boneSet) { _cauterizeBoneSet = boneSet; }

    int getBlendshapeCoefficientsNum() const { return _blendshapeCoefficients.size(); }
    float getBlendshapeCoefficient(int index) const {
        return ((index < 0) && (index >= _blendshapeCoefficients.size())) ? 0.0f : _blendshapeCoefficients.at(index);
     }

protected:

    void setPupilDilation(float dilation) { _pupilDilation = dilation; }
    float getPupilDilation() const { return _pupilDilation; }

    void setBlendshapeCoefficients(const QVector<float>& coefficients) { _blendshapeCoefficients = coefficients; }
    const QVector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }

    /// Returns the unscaled extents of the model's mesh
    Extents getUnscaledMeshExtents() const;

    /// Returns the scaled equivalent of some extents in model space.
    Extents calculateScaledOffsetExtents(const Extents& extents) const;

    /// Returns the world space equivalent of some box in model space.
    AABox calculateScaledOffsetAABox(const AABox& box) const;

    /// Returns the scaled equivalent of a point in model space.
    glm::vec3 calculateScaledOffsetPoint(const glm::vec3& point) const;

    /// Fetches the joint state at the specified index.
    /// \return whether or not the joint state is "valid" (that is, non-default)
    bool getJointState(int index, glm::quat& rotation) const;

    /// Clear the joint states
    void clearJointState(int index);

    /// Returns the index of the last free ancestor of the indexed joint, or -1 if not found.
    int getLastFreeJointIndex(int jointIndex) const;

    /// \param jointIndex index of joint in model structure
    /// \param position[out] position of joint in model-frame
    /// \return true if joint exists
    bool getJointPosition(int jointIndex, glm::vec3& position) const;

    QSharedPointer<NetworkGeometry> _geometry;
    void setGeometry(const QSharedPointer<NetworkGeometry>& newGeometry);

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

    // returns 'true' if needs fullUpdate after geometry change
    bool updateGeometry();

    virtual void initJointStates(QVector<JointState> states);

    void setScaleInternal(const glm::vec3& scale);
    void scaleToFit();
    void snapToRegistrationPoint();

    void simulateInternal(float deltaTime);
    virtual void updateRig(float deltaTime, glm::mat4 parentTransform);

    /// \param jointIndex index of joint in model structure
    /// \param position position of joint in model-frame
    /// \param rotation rotation of joint in model-frame
    /// \param useRotation false if rotation should be ignored
    /// \param lastFreeIndex
    /// \param allIntermediatesFree
    /// \param alignment
    /// \return true if joint exists
    bool setJointPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation = glm::quat(),
        bool useRotation = false, int lastFreeIndex = -1, bool allIntermediatesFree = false,
        const glm::vec3& alignment = glm::vec3(0.0f, -1.0f, 0.0f), float priority = 1.0f);

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

private:

    void deleteGeometry();
    QVector<JointState> createJointStates(const FBXGeometry& geometry);
    void initJointTransforms();

    QSharedPointer<NetworkGeometry> _collisionGeometry;

    float _pupilDilation;
    QVector<float> _blendshapeCoefficients;

    QUrl _url;
    QUrl _collisionUrl;
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

    void segregateMeshGroups(); // used to calculate our list of translucent vs opaque meshes

    bool _meshGroupsKnown;
    bool _isWireframe;


    // debug rendering support
    void renderDebugMeshBoxes(gpu::Batch& batch);
    int _debugMeshBoxesID = GeometryCache::UNKNOWN_ID;


    static AbstractViewStateInterface* _viewState;

    bool _renderCollisionHull;


    QSet<std::shared_ptr<MeshPartPayload>> _renderItemsSet;
    QMap<render::ItemID, render::PayloadPointer> _renderItems;
    bool _readyWhenAdded = false;
    bool _needsReload = true;
    bool _needsUpdateClusterMatrices = true;

    friend class MeshPartPayload;
protected:
    RigPointer _rig;
};

Q_DECLARE_METATYPE(QPointer<Model>)
Q_DECLARE_METATYPE(QWeakPointer<NetworkGeometry>)

/// Handle management of pending models that need blending
class ModelBlender : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:

    /// Adds the specified model to the list requiring vertex blends.
    void noteRequiresBlend(Model* model);

public slots:
    void setBlendedVertices(const QPointer<Model>& model, int blendNumber, const QWeakPointer<NetworkGeometry>& geometry,
        const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals);

private:
    ModelBlender();
    virtual ~ModelBlender();

    QList<QPointer<Model> > _modelsRequiringBlends;
    int _pendingBlenders;
};


#endif // hifi_Model_h
