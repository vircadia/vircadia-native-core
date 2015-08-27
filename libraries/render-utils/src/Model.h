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
#include <gpu/Stream.h>
#include <gpu/Batch.h>
#include <gpu/Pipeline.h>
#include "PhysicsEntity.h"
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

inline uint qHash(const std::shared_ptr<MeshPartPayload>& a, uint seed) {
    return qHash(a.get(), seed);
}

/// A generic 3D model displaying geometry loaded from a URL.
class Model : public QObject, public PhysicsEntity {
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
    void renderPart(RenderArgs* args, int meshIndex, int partIndex, bool translucent);

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

    /// Returns a reference to the shared geometry.
    const QSharedPointer<NetworkGeometry>& getGeometry() const { return _geometry; }

    bool isActive() const { return _geometry && _geometry->isLoaded(); }

    Q_INVOKABLE void setTextureWithNameToURL(const QString& name, const QUrl& url)
        { _geometry->setTextureWithNameToURL(name, url); }

    bool convexHullContains(glm::vec3 point);

    QStringList getJointNames() const;

    /// Sets the joint state at the specified index.
    void setJointState(int index, bool valid, const glm::quat& rotation = glm::quat(), float priority = 1.0f);

    bool findRayIntersectionAgainstSubMeshes(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                             BoxFace& face, QString& extraInfo, bool pickAgainstTriangles = false);

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

    /// Returns the index of the parent of the indexed joint, or -1 if not found.
    int getParentJointIndex(int jointIndex) const;

    void inverseKinematics(int jointIndex, glm::vec3 position, const glm::quat& rotation, float priority);

    /// Returns the extents of the model in its bind pose.
    Extents getBindExtents() const;

    /// Returns the extents of the model's mesh
    Extents getMeshExtents() const;

    void setScale(const glm::vec3& scale);
    const glm::vec3& getScale() const { return _scale; }

    /// enables/disables scale to fit behavior, the model will be automatically scaled to the specified largest dimension
    bool getIsScaledToFit() const { return _scaledToFit; } /// is model scaled to fit
    const glm::vec3& getScaleToFitDimensions() const { return _scaleToFitDimensions; } /// the dimensions model is scaled to

    void setCauterizeBones(bool flag) { _cauterizeBones = flag; }
    bool getCauterizeBones() const { return _cauterizeBones; }

    const std::unordered_set<int>& getCauterizeBoneSet() const { return _cauterizeBoneSet; }
    void setCauterizeBoneSet(const std::unordered_set<int>& boneSet) { _cauterizeBoneSet = boneSet; }

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

    /// Fetches the visible joint state at the specified index.
    /// \return whether or not the joint state is "valid" (that is, non-default)
    bool getVisibleJointState(int index, glm::quat& rotation) const;

    /// Clear the joint states
    void clearJointState(int index);

    /// Returns the index of the last free ancestor of the indexed joint, or -1 if not found.
    int getLastFreeJointIndex(int jointIndex) const;

    bool getVisibleJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const;
    bool getVisibleJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const;

    /// \param jointIndex index of joint in model structure
    /// \param position[out] position of joint in model-frame
    /// \return true if joint exists
    bool getJointPosition(int jointIndex, glm::vec3& position) const;

    void setShowTrueJointTransforms(bool show) { _showTrueJointTransforms = show; }

    QSharedPointer<NetworkGeometry> _geometry;
    void setGeometry(const QSharedPointer<NetworkGeometry>& newGeometry);

    glm::vec3 _scale;
    glm::vec3 _offset;

    static float FAKE_DIMENSION_PLACEHOLDER;

    bool _scaleToFit; /// If you set scaleToFit, we will calculate scale based on MeshExtents
    glm::vec3 _scaleToFitDimensions; /// this is the dimensions that scale to fit will use
    bool _scaledToFit; /// have we scaled to fit

    bool _snapModelToRegistrationPoint; /// is the model's offset automatically adjusted to a registration point in model space
    bool _snappedToRegistrationPoint; /// are we currently snapped to a registration point
    glm::vec3 _registrationPoint = glm::vec3(0.5f); /// the point in model space our center is snapped to

    bool _showTrueJointTransforms;

    class MeshState {
    public:
        QVector<glm::mat4> clusterMatrices;
        QVector<glm::mat4> cauterizedClusterMatrices;
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
    std::vector<Transform> _transforms;
    gpu::Batch _renderBatch;

    QVector<QVector<QSharedPointer<Texture> > > _dilatedTextures;

    QVector<float> _blendedBlendshapeCoefficients;
    int _blendNumber;
    int _appliedBlendNumber;

    class Locations {
    public:
        int tangent;
        int alphaThreshold;
        int texcoordMatrices;
        int specularTextureUnit;
        int emissiveTextureUnit;
        int emissiveParams;
        int glowIntensity;
        int normalFittingMapUnit;
        int materialBufferUnit;
        int clusterMatrices;
        int clusterIndices;
        int clusterWeights;
        int lightBufferUnit;
    };

    QHash<QPair<int,int>, AABox> _calculatedMeshPartBoxes; // world coordinate AABoxes for all sub mesh part boxes
    QHash<QPair<int,int>, qint64> _calculatedMeshPartOffset;
    bool _calculatedMeshPartOffsetValid;


    bool _calculatedMeshPartBoxesValid;
    QVector<AABox> _calculatedMeshBoxes; // world coordinate AABoxes for all sub mesh boxes
    bool _calculatedMeshBoxesValid;

    QVector< QVector<Triangle> > _calculatedMeshTriangles; // world coordinate triangles for all sub meshes
    bool _calculatedMeshTrianglesValid;
    QMutex _mutex;

    void recalculateMeshBoxes(bool pickAgainstTriangles = false);
    void recalculateMeshPartOffsets();

    void segregateMeshGroups(); // used to calculate our list of translucent vs opaque meshes

    bool _meshGroupsKnown;
    bool _isWireframe;


    // debug rendering support
    void renderDebugMeshBoxes(gpu::Batch& batch);
    int _debugMeshBoxesID = GeometryCache::UNKNOWN_ID;

    // helper functions used by render() or renderInScene()

    static void pickPrograms(gpu::Batch& batch, RenderArgs::RenderMode mode, bool translucent, float alphaThreshold,
                            bool hasLightmap, bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe, RenderArgs* args,
                            Locations*& locations);

    static AbstractViewStateInterface* _viewState;

    class RenderKey {
    public:
         enum FlagBit {
            IS_TRANSLUCENT_FLAG = 0,
            HAS_LIGHTMAP_FLAG,
            HAS_TANGENTS_FLAG,
            HAS_SPECULAR_FLAG,
            HAS_EMISSIVE_FLAG,
            IS_SKINNED_FLAG,
            IS_STEREO_FLAG,
            IS_DEPTH_ONLY_FLAG,
            IS_SHADOW_FLAG,
            IS_MIRROR_FLAG, //THis means that the mesh is rendered mirrored, not the same as "Rear view mirror"
            IS_WIREFRAME_FLAG,

            NUM_FLAGS,
        };

        enum Flag {
            IS_TRANSLUCENT = (1 << IS_TRANSLUCENT_FLAG),
            HAS_LIGHTMAP = (1 << HAS_LIGHTMAP_FLAG),
            HAS_TANGENTS = (1 << HAS_TANGENTS_FLAG),
            HAS_SPECULAR = (1 << HAS_SPECULAR_FLAG),
            HAS_EMISSIVE = (1 << HAS_EMISSIVE_FLAG),
            IS_SKINNED = (1 << IS_SKINNED_FLAG),
            IS_STEREO = (1 << IS_STEREO_FLAG),
            IS_DEPTH_ONLY = (1 << IS_DEPTH_ONLY_FLAG),
            IS_SHADOW = (1 << IS_SHADOW_FLAG),
            IS_MIRROR = (1 << IS_MIRROR_FLAG),
            IS_WIREFRAME = (1 << IS_WIREFRAME_FLAG),
        };
        typedef unsigned short Flags;



        bool isFlag(short flagNum) const { return bool((_flags & flagNum) != 0); }

        bool isTranslucent() const { return isFlag(IS_TRANSLUCENT); }
        bool hasLightmap() const { return isFlag(HAS_LIGHTMAP); }
        bool hasTangents() const { return isFlag(HAS_TANGENTS); }
        bool hasSpecular() const { return isFlag(HAS_SPECULAR); }
        bool hasEmissive() const { return isFlag(HAS_EMISSIVE); }
        bool isSkinned() const { return isFlag(IS_SKINNED); }
        bool isStereo() const { return isFlag(IS_STEREO); }
        bool isDepthOnly() const { return isFlag(IS_DEPTH_ONLY); }
        bool isShadow() const { return isFlag(IS_SHADOW); } // = depth only but with back facing
        bool isMirror() const { return isFlag(IS_MIRROR); }
        bool isWireFrame() const { return isFlag(IS_WIREFRAME); }

        Flags _flags = 0;
        short _spare = 0;

        int getRaw() { return *reinterpret_cast<int*>(this); }


        RenderKey(
            bool translucent, bool hasLightmap,
            bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe) :
            RenderKey(  (translucent ? IS_TRANSLUCENT : 0)
                      | (hasLightmap ? HAS_LIGHTMAP : 0)
                      | (hasTangents ? HAS_TANGENTS : 0)
                      | (hasSpecular ? HAS_SPECULAR : 0)
                      | (isSkinned ? IS_SKINNED : 0)
                      | (isWireframe ? IS_WIREFRAME : 0)
                     ) {}

        RenderKey(RenderArgs::RenderMode mode,
            bool translucent, float alphaThreshold, bool hasLightmap,
            bool hasTangents, bool hasSpecular, bool isSkinned, bool isWireframe) :
            RenderKey( ((translucent && (alphaThreshold == 0.0f) && (mode != RenderArgs::SHADOW_RENDER_MODE)) ? IS_TRANSLUCENT : 0)
                      | (hasLightmap && (mode != RenderArgs::SHADOW_RENDER_MODE) ? HAS_LIGHTMAP : 0) // Lightmap, tangents and specular don't matter for depthOnly
                      | (hasTangents && (mode != RenderArgs::SHADOW_RENDER_MODE) ? HAS_TANGENTS : 0)
                      | (hasSpecular && (mode != RenderArgs::SHADOW_RENDER_MODE) ? HAS_SPECULAR : 0)
                      | (isSkinned ? IS_SKINNED : 0)
                      | (isWireframe ? IS_WIREFRAME : 0)
                      | ((mode == RenderArgs::SHADOW_RENDER_MODE) ? IS_DEPTH_ONLY : 0)
                      | ((mode == RenderArgs::SHADOW_RENDER_MODE) ? IS_SHADOW : 0)
                      | ((mode == RenderArgs::MIRROR_RENDER_MODE) ? IS_MIRROR :0)
                     ) {}

        RenderKey(int bitmask) : _flags(bitmask) {}
    };


    class RenderPipeline {
    public:
        gpu::PipelinePointer _pipeline;
        std::shared_ptr<Locations> _locations;
        RenderPipeline(gpu::PipelinePointer pipeline, std::shared_ptr<Locations> locations) :
            _pipeline(pipeline), _locations(locations) {}
    };

    typedef std::unordered_map<int, RenderPipeline> BaseRenderPipelineMap;
    class RenderPipelineLib : public BaseRenderPipelineMap {
    public:
        typedef RenderKey Key;


        void addRenderPipeline(Key key, gpu::ShaderPointer& vertexShader, gpu::ShaderPointer& pixelShader);

        void initLocations(gpu::ShaderPointer& program, Locations& locations);
    };
    static RenderPipelineLib _renderPipelineLib;

    bool _renderCollisionHull;


    QSet<std::shared_ptr<MeshPartPayload>> _transparentRenderItems;
    QSet<std::shared_ptr<MeshPartPayload>> _opaqueRenderItems;
    QMap<render::ItemID, render::PayloadPointer> _renderItems;
    bool _readyWhenAdded = false;
    bool _needsReload = true;

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
