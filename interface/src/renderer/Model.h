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

#include "Transform.h"
#include <AABox.h>
#include <AnimationCache.h>
#include <PhysicsEntity.h>

#include "AnimationHandle.h"
#include "GeometryCache.h"
#include "InterfaceConfig.h"
#include "JointState.h"
#include "ProgramObject.h"
#include "TextureCache.h"

class QScriptEngine;

class Shape;
class RenderArgs;
class ViewFrustum;


#include "gpu/Stream.h"
#include "gpu/Batch.h"

/// A generic 3D model displaying geometry loaded from a URL.
class Model : public QObject, public PhysicsEntity {
    Q_OBJECT
    
public:

    Model(QObject* parent = NULL);
    virtual ~Model();
    
    /// enables/disables scale to fit behavior, the model will be automatically scaled to the specified largest dimension
    void setScaleToFit(bool scaleToFit, float largestDimension = 0.0f);
    bool getScaleToFit() const { return _scaleToFit; } /// is scale to fit enabled
    bool getIsScaledToFit() const { return _scaledToFit; } /// is model scaled to fit
    const glm::vec3& getScaleToFitDimensions() const { return _scaleToFitDimensions; } /// the dimensions model is scaled to
    void setScaleToFit(bool scaleToFit, const glm::vec3& dimensions);

    void setSnapModelToCenter(bool snapModelToCenter) { 
        setSnapModelToRegistrationPoint(snapModelToCenter, glm::vec3(0.5f,0.5f,0.5f));
    };
    bool getSnapModelToCenter() { 
        return _snapModelToRegistrationPoint && _registrationPoint == glm::vec3(0.5f,0.5f,0.5f); 
    }

    void setSnapModelToRegistrationPoint(bool snapModelToRegistrationPoint, const glm::vec3& registrationPoint);
    bool getSnapModelToRegistrationPoint() { return _snapModelToRegistrationPoint; }
    
    void setScale(const glm::vec3& scale);
    const glm::vec3& getScale() const { return _scale; }
    
    void setOffset(const glm::vec3& offset);
    const glm::vec3& getOffset() const { return _offset; }
    
    void setPupilDilation(float dilation) { _pupilDilation = dilation; }
    float getPupilDilation() const { return _pupilDilation; }
    
    void setBlendshapeCoefficients(const QVector<float>& coefficients) { _blendshapeCoefficients = coefficients; }
    const QVector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }

    bool isActive() const { return _geometry && _geometry->isLoaded(); }
    
    bool isRenderable() const { return !_meshStates.isEmpty() || (isActive() && _geometry->getMeshes().isEmpty()); }
    
    bool isLoadedWithTextures() const { return _geometry && _geometry->isLoadedWithTextures(); }
    
    void init();
    void reset();
    virtual void simulate(float deltaTime, bool fullUpdate = true);
    
    enum RenderMode { DEFAULT_RENDER_MODE, SHADOW_RENDER_MODE, DIFFUSE_RENDER_MODE, NORMAL_RENDER_MODE };
    
    bool render(float alpha = 1.0f, RenderMode mode = DEFAULT_RENDER_MODE, RenderArgs* args = NULL);
    
    // Scene rendering support
    static void startScene();
    bool renderInScene(float alpha = 1.0f, RenderArgs* args = NULL);
    static void endScene(RenderMode mode = DEFAULT_RENDER_MODE, RenderArgs* args = NULL);

    /// Sets the URL of the model to render.
    /// \param fallback the URL of a fallback model to render if the requested model fails to load
    /// \param retainCurrent if true, keep rendering the current model until the new one is loaded
    /// \param delayLoad if true, don't load the model immediately; wait until actually requested
    Q_INVOKABLE void setURL(const QUrl& url, const QUrl& fallback = QUrl(),
        bool retainCurrent = false, bool delayLoad = false);
    
    const QUrl& getURL() const { return _url; }
    
    /// Sets the distance parameter used for LOD computations.
    void setLODDistance(float distance) { _lodDistance = distance; }
    
    /// Returns the extents of the model in its bind pose.
    Extents getBindExtents() const;

    /// Returns the extents of the model's mesh
    Extents getMeshExtents() const;

    /// Returns the unscaled extents of the model's mesh
    Extents getUnscaledMeshExtents() const;

    /// Returns the scaled equivalent of some extents in model space.
    Extents calculateScaledOffsetExtents(const Extents& extents) const;

    /// Returns a reference to the shared geometry.
    const QSharedPointer<NetworkGeometry>& getGeometry() const { return _geometry; }
    
    /// Returns the number of joint states in the model.
    int getJointStateCount() const { return _jointStates.size(); }
    
    /// Fetches the joint state at the specified index.
    /// \return whether or not the joint state is "valid" (that is, non-default)
    bool getJointState(int index, glm::quat& rotation) const;

    /// Fetches the visible joint state at the specified index.
    /// \return whether or not the joint state is "valid" (that is, non-default)
    bool getVisibleJointState(int index, glm::quat& rotation) const;
    
    /// Clear the joint states
    void clearJointState(int index);
    
    /// Clear the joint animation priority
    void clearJointAnimationPriority(int index);
    
    /// Sets the joint state at the specified index.
    void setJointState(int index, bool valid, const glm::quat& rotation = glm::quat(), float priority = 1.0f);
    
    /// Returns the index of the parent of the indexed joint, or -1 if not found.
    int getParentJointIndex(int jointIndex) const;
    
    /// Returns the index of the last free ancestor of the indexed joint, or -1 if not found.
    int getLastFreeJointIndex(int jointIndex) const;
    
    bool getJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const;
    bool getJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const;
    bool getJointCombinedRotation(int jointIndex, glm::quat& rotation) const;

    bool getVisibleJointPositionInWorldFrame(int jointIndex, glm::vec3& position) const;
    bool getVisibleJointRotationInWorldFrame(int jointIndex, glm::quat& rotation) const;

    /// \param jointIndex index of joint in model structure
    /// \param position[out] position of joint in model-frame
    /// \return true if joint exists
    bool getJointPosition(int jointIndex, glm::vec3& position) const;

    /// \param jointIndex index of joint in model structure
    /// \param rotation[out] rotation of joint in model-frame
    /// \return true if joint exists
    bool getJointRotation(int jointIndex, glm::quat& rotation) const;

    QStringList getJointNames() const;
    
    AnimationHandlePointer createAnimationHandle();

    const QList<AnimationHandlePointer>& getRunningAnimations() const { return _runningAnimations; }
   
    // virtual overrides from PhysicsEntity
    virtual void buildShapes();
    virtual void updateShapePositions();

    virtual void renderJointCollisionShapes(float alpha);
    
    bool maybeStartBlender();
    
    /// Sets blended vertices computed in a separate thread.
    void setBlendedVertices(int blendNumber, const QWeakPointer<NetworkGeometry>& geometry,
        const QVector<glm::vec3>& vertices, const QVector<glm::vec3>& normals);

    void setShowTrueJointTransforms(bool show) { _showTrueJointTransforms = show; }

    QVector<JointState>& getJointStates() { return _jointStates; }
    const QVector<JointState>& getJointStates() const { return _jointStates; }
 
    void inverseKinematics(int jointIndex, glm::vec3 position, const glm::quat& rotation, float priority);
    
    Q_INVOKABLE void setTextureWithNameToURL(const QString& name, const QUrl& url)
        { _geometry->setTextureWithNameToURL(name, url); }

    bool findRayIntersectionAgainstSubMeshes(const glm::vec3& origin, const glm::vec3& direction, 
                                                float& distance, BoxFace& face, QString& extraInfo) const;

protected:
    QSharedPointer<NetworkGeometry> _geometry;
    
    glm::vec3 _scale;
    glm::vec3 _offset;

    static float FAKE_DIMENSION_PLACEHOLDER;

    bool _scaleToFit; /// If you set scaleToFit, we will calculate scale based on MeshExtents
    glm::vec3 _scaleToFitDimensions; /// this is the dimensions that scale to fit will use
    bool _scaledToFit; /// have we scaled to fit

    bool _snapModelToRegistrationPoint; /// is the model's offset automatically adjusted to a registration point in model space
    bool _snappedToRegistrationPoint; /// are we currently snapped to a registration point
    glm::vec3 _registrationPoint; /// the point in model space our center is snapped to
    
    bool _showTrueJointTransforms;
    
    QVector<JointState> _jointStates;

    class MeshState {
    public:
        QVector<glm::mat4> clusterMatrices;
    };
    
    QVector<MeshState> _meshStates;
    
    // returns 'true' if needs fullUpdate after geometry change
    bool updateGeometry();

    virtual void setJointStates(QVector<JointState> states);
    
    void setScaleInternal(const glm::vec3& scale);
    void scaleToFit();
    void snapToRegistrationPoint();

    void simulateInternal(float deltaTime);

    /// Updates the state of the joint at the specified index.
    virtual void updateJointState(int index);

    virtual void updateVisibleJointStates();
    
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
    
private:
    
    friend class AnimationHandle;
    
    void applyNextGeometry();
    void deleteGeometry();
    QVector<JointState> createJointStates(const FBXGeometry& geometry);
    void initJointTransforms();
    
    QSharedPointer<NetworkGeometry> _baseGeometry; ///< reference required to prevent collection of base
    QSharedPointer<NetworkGeometry> _nextBaseGeometry;
    QSharedPointer<NetworkGeometry> _nextGeometry;
    float _lodDistance;
    float _lodHysteresis;
    float _nextLODHysteresis;
    
    float _pupilDilation;
    QVector<float> _blendshapeCoefficients;
    
    QUrl _url;

    gpu::Buffers _blendedVertexBuffers;
    gpu::Transforms _transforms;
    gpu::Batch _renderBatch;

    QVector<QVector<QSharedPointer<Texture> > > _dilatedTextures;
    
    QVector<Model*> _attachments;

    QSet<WeakAnimationHandlePointer> _animationHandles;

    QList<AnimationHandlePointer> _runningAnimations;

    QVector<float> _blendedBlendshapeCoefficients;
    int _blendNumber;
    int _appliedBlendNumber;

    static ProgramObject _program;
    static ProgramObject _normalMapProgram;
    static ProgramObject _specularMapProgram;
    static ProgramObject _normalSpecularMapProgram;
    static ProgramObject _translucentProgram;
    
    static ProgramObject _shadowProgram;
    
    static ProgramObject _skinProgram;
    static ProgramObject _skinNormalMapProgram;
    static ProgramObject _skinSpecularMapProgram;
    static ProgramObject _skinNormalSpecularMapProgram;
    static ProgramObject _skinTranslucentProgram;
    
    static ProgramObject _skinShadowProgram;
    
    static int _normalMapTangentLocation;
    static int _normalSpecularMapTangentLocation;
    
    class Locations {
    public:
        int tangent;
        int alphaThreshold;
    };
    
    static Locations _locations;
    static Locations _normalMapLocations;
    static Locations _specularMapLocations;
    static Locations _normalSpecularMapLocations;
    static Locations _translucentLocations;
    
    static void initProgram(ProgramObject& program, Locations& locations, int specularTextureUnit = 1);
        
    class SkinLocations : public Locations {
    public:
        int clusterMatrices;
        int clusterIndices;
        int clusterWeights;    
    };
    
    static SkinLocations _skinLocations;
    static SkinLocations _skinNormalMapLocations;
    static SkinLocations _skinSpecularMapLocations;
    static SkinLocations _skinNormalSpecularMapLocations;    
    static SkinLocations _skinShadowLocations;
    static SkinLocations _skinTranslucentLocations;
        
    static void initSkinProgram(ProgramObject& program, SkinLocations& locations, int specularTextureUnit = 1);

    QVector<AABox> _calculatedMeshBoxes;
    bool _calculatedMeshBoxesValid;

    void recalcuateMeshBoxes();

    void segregateMeshGroups(); // used to calculate our list of translucent vs opaque meshes

    bool _meshGroupsKnown;

    QMap<QString, int> _unsortedMeshesTranslucent;
    QMap<QString, int> _unsortedMeshesTranslucentTangents;
    QMap<QString, int> _unsortedMeshesTranslucentTangentsSpecular;
    QMap<QString, int> _unsortedMeshesTranslucentSpecular;

    QMap<QString, int> _unsortedMeshesTranslucentSkinned;
    QMap<QString, int> _unsortedMeshesTranslucentTangentsSkinned;
    QMap<QString, int> _unsortedMeshesTranslucentTangentsSpecularSkinned;
    QMap<QString, int> _unsortedMeshesTranslucentSpecularSkinned;

    QMap<QString, int> _unsortedMeshesOpaque;
    QMap<QString, int> _unsortedMeshesOpaqueTangents;
    QMap<QString, int> _unsortedMeshesOpaqueTangentsSpecular;
    QMap<QString, int> _unsortedMeshesOpaqueSpecular;

    QMap<QString, int> _unsortedMeshesOpaqueSkinned;
    QMap<QString, int> _unsortedMeshesOpaqueTangentsSkinned;
    QMap<QString, int> _unsortedMeshesOpaqueTangentsSpecularSkinned;
    QMap<QString, int> _unsortedMeshesOpaqueSpecularSkinned;

    QVector<int> _meshesTranslucent;
    QVector<int> _meshesTranslucentTangents;
    QVector<int> _meshesTranslucentTangentsSpecular;
    QVector<int> _meshesTranslucentSpecular;

    QVector<int> _meshesTranslucentSkinned;
    QVector<int> _meshesTranslucentTangentsSkinned;
    QVector<int> _meshesTranslucentTangentsSpecularSkinned;
    QVector<int> _meshesTranslucentSpecularSkinned;

    QVector<int> _meshesOpaque;
    QVector<int> _meshesOpaqueTangents;
    QVector<int> _meshesOpaqueTangentsSpecular;
    QVector<int> _meshesOpaqueSpecular;

    QVector<int> _meshesOpaqueSkinned;
    QVector<int> _meshesOpaqueTangentsSkinned;
    QVector<int> _meshesOpaqueTangentsSpecularSkinned;
    QVector<int> _meshesOpaqueSpecularSkinned;

    // Scene rendering support
    static QVector<Model*> _modelsInScene;
    static gpu::Batch _sceneRenderBatch;

    static void endSceneSimple(RenderMode mode = DEFAULT_RENDER_MODE, RenderArgs* args = NULL);
    static void endSceneSplitPass(RenderMode mode = DEFAULT_RENDER_MODE, RenderArgs* args = NULL);

    // helper functions used by render() or renderInScene()
    void renderSetup(RenderArgs* args);
    bool renderCore(float alpha, RenderMode mode, RenderArgs* args);
    int renderMeshes(gpu::Batch& batch, RenderMode mode, bool translucent, float alphaThreshold, 
                        bool hasTangents, bool hasSpecular, bool isSkinned, RenderArgs* args = NULL);
    void setupBatchTransform(gpu::Batch& batch);
    QVector<int>* pickMeshList(bool translucent, float alphaThreshold, bool hasTangents, bool hasSpecular, bool isSkinned);

    int renderMeshesFromList(QVector<int>& list, gpu::Batch& batch, RenderMode mode, bool translucent, float alphaThreshold,
                                        RenderArgs* args, SkinLocations* skinLocations, GLenum specularTextureUnit);

    static void pickPrograms(gpu::Batch& batch, RenderMode mode, bool translucent, float alphaThreshold,
                            bool hasTangents, bool hasSpecular, bool isSkinned, RenderArgs* args,
                            SkinLocations*& skinLocations, GLenum& specularTextureUnit);

    static int renderMeshesForModelsInScene(gpu::Batch& batch, RenderMode mode, bool translucent, float alphaThreshold,
                            bool hasTangents, bool hasSpecular, bool isSkinned, RenderArgs* args);


};

Q_DECLARE_METATYPE(QPointer<Model>)
Q_DECLARE_METATYPE(QWeakPointer<NetworkGeometry>)
Q_DECLARE_METATYPE(QVector<glm::vec3>)


#endif // hifi_Model_h
