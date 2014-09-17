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

#include <PhysicsEntity.h>

#include <AnimationCache.h>

#include "GeometryCache.h"
#include "InterfaceConfig.h"
#include "JointState.h"
#include "ProgramObject.h"
#include "TextureCache.h"

class QScriptEngine;

class AnimationHandle;
class Shape;

typedef QSharedPointer<AnimationHandle> AnimationHandlePointer;
typedef QWeakPointer<AnimationHandle> WeakAnimationHandlePointer;

const int MAX_LOCAL_LIGHTS = 2;

/// A generic 3D model displaying geometry loaded from a URL.
class Model : public QObject, public PhysicsEntity {
    Q_OBJECT
    
public:

    /// Registers the script types associated with models.
    static void registerMetaTypes(QScriptEngine* engine);

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
    
    bool render(float alpha = 1.0f, RenderMode mode = DEFAULT_RENDER_MODE, bool receiveShadows = true);

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

    QStringList getJointNames() const;
    
    AnimationHandlePointer createAnimationHandle();

    const QList<AnimationHandlePointer>& getRunningAnimations() const { return _runningAnimations; }
   
    // virtual overrides from PhysicsEntity
    virtual void buildShapes();
    virtual void updateShapePositions();

    virtual void renderJointCollisionShapes(float alpha);
    
    /// Sets blended vertices computed in a separate thread.
    void setBlendedVertices(const QWeakPointer<NetworkGeometry>& geometry, const QVector<glm::vec3>& vertices,
        const QVector<glm::vec3>& normals);

    class LocalLight {
    public:
        glm::vec3 color;
        glm::vec3 direction;
    };
    
    void setLocalLights(const QVector<LocalLight>& localLights) { _localLights = localLights; }
    const QVector<LocalLight>& getLocalLights() const { return _localLights; }

    void setShowTrueJointTransforms(bool show) { _showTrueJointTransforms = show; }

    QVector<JointState>& getJointStates() { return _jointStates; }
    const QVector<JointState>& getJointStates() const { return _jointStates; }
 
protected:
    QSharedPointer<NetworkGeometry> _geometry;
    
    glm::vec3 _scale;
    glm::vec3 _offset;

    bool _scaleToFit; /// If you set scaleToFit, we will calculate scale based on MeshExtents
    glm::vec3 _scaleToFitDimensions; /// this is the dimensions that scale to fit will use
    bool _scaledToFit; /// have we scaled to fit

    bool _snapModelToRegistrationPoint; /// is the model's offset automatically adjusted to a registration point in model space
    bool _snappedToRegistrationPoint; /// are we currently snapped to a registration point
    glm::vec3 _registrationPoint; /// the point in model space our center is snapped to
    
    bool _showTrueJointTransforms;
    
    QVector<LocalLight> _localLights;
    
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

    void inverseKinematics(int jointIndex, glm::vec3 position, const glm::quat& rotation, float priority);
    
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
    void renderMeshes(RenderMode mode, bool translucent, bool receiveShadows);
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
        
    QVector<QOpenGLBuffer> _blendedVertexBuffers;
    
    QVector<QVector<QSharedPointer<Texture> > > _dilatedTextures;
    
    QVector<Model*> _attachments;

    QSet<WeakAnimationHandlePointer> _animationHandles;

    QList<AnimationHandlePointer> _runningAnimations;

    glm::vec4 _localLightColors[MAX_LOCAL_LIGHTS];
    glm::vec4 _localLightDirections[MAX_LOCAL_LIGHTS];

    bool _blenderPending;
    bool _blendRequired;

    static ProgramObject _program;
    static ProgramObject _normalMapProgram;
    static ProgramObject _specularMapProgram;
    static ProgramObject _normalSpecularMapProgram;
    
    static ProgramObject _shadowMapProgram;
    static ProgramObject _shadowNormalMapProgram;
    static ProgramObject _shadowSpecularMapProgram;
    static ProgramObject _shadowNormalSpecularMapProgram;
    
    static ProgramObject _cascadedShadowMapProgram;
    static ProgramObject _cascadedShadowNormalMapProgram;
    static ProgramObject _cascadedShadowSpecularMapProgram;
    static ProgramObject _cascadedShadowNormalSpecularMapProgram;
    
    static ProgramObject _shadowProgram;
    
    static ProgramObject _skinProgram;
    static ProgramObject _skinNormalMapProgram;
    static ProgramObject _skinSpecularMapProgram;
    static ProgramObject _skinNormalSpecularMapProgram;
    
    static ProgramObject _skinShadowMapProgram;
    static ProgramObject _skinShadowNormalMapProgram;
    static ProgramObject _skinShadowSpecularMapProgram;
    static ProgramObject _skinShadowNormalSpecularMapProgram;
    
    static ProgramObject _skinCascadedShadowMapProgram;
    static ProgramObject _skinCascadedShadowNormalMapProgram;
    static ProgramObject _skinCascadedShadowSpecularMapProgram;
    static ProgramObject _skinCascadedShadowNormalSpecularMapProgram;
    
    static ProgramObject _skinShadowProgram;
    
    static int _normalMapTangentLocation;
    static int _normalSpecularMapTangentLocation;
    static int _shadowNormalMapTangentLocation;
    static int _shadowNormalSpecularMapTangentLocation;
    static int _cascadedShadowNormalMapTangentLocation;
    static int _cascadedShadowNormalSpecularMapTangentLocation;
    
    static int _cascadedShadowMapDistancesLocation;
    static int _cascadedShadowNormalMapDistancesLocation;
    static int _cascadedShadowSpecularMapDistancesLocation;
    static int _cascadedShadowNormalSpecularMapDistancesLocation;
    
    class Locations {
    public:
        int localLightColors;
        int localLightDirections; 
        int tangent;
        int shadowDistances;
    };
    
    static Locations _locations;
    static Locations _normalMapLocations;
    static Locations _specularMapLocations;
    static Locations _normalSpecularMapLocations;
    static Locations _shadowMapLocations;
    static Locations _shadowNormalMapLocations;
    static Locations _shadowSpecularMapLocations;
    static Locations _shadowNormalSpecularMapLocations;
    static Locations _cascadedShadowMapLocations;
    static Locations _cascadedShadowNormalMapLocations;
    static Locations _cascadedShadowSpecularMapLocations;
    static Locations _cascadedShadowNormalSpecularMapLocations;
    
    static void initProgram(ProgramObject& program, Locations& locations,
        int specularTextureUnit = 1, int shadowTextureUnit = 1);
        
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
    static SkinLocations _skinShadowMapLocations;
    static SkinLocations _skinShadowNormalMapLocations;
    static SkinLocations _skinShadowSpecularMapLocations;
    static SkinLocations _skinShadowNormalSpecularMapLocations;
    static SkinLocations _skinCascadedShadowMapLocations;
    static SkinLocations _skinCascadedShadowNormalMapLocations;
    static SkinLocations _skinCascadedShadowSpecularMapLocations;
    static SkinLocations _skinCascadedShadowNormalSpecularMapLocations;
    static SkinLocations _skinShadowLocations;
    
    static void initSkinProgram(ProgramObject& program, SkinLocations& locations,
        int specularTextureUnit = 1, int shadowTextureUnit = 1);
};

Q_DECLARE_METATYPE(QPointer<Model>)
Q_DECLARE_METATYPE(QWeakPointer<NetworkGeometry>)
Q_DECLARE_METATYPE(QVector<glm::vec3>)
Q_DECLARE_METATYPE(Model::LocalLight)
Q_DECLARE_METATYPE(QVector<Model::LocalLight>)

/// Represents a handle to a model animation.
class AnimationHandle : public QObject {
    Q_OBJECT
    
public:

    void setRole(const QString& role) { _role = role; }
    const QString& getRole() const { return _role; }

    void setURL(const QUrl& url);
    const QUrl& getURL() const { return _url; }
    
    void setFPS(float fps) { _fps = fps; }
    float getFPS() const { return _fps; }

    void setPriority(float priority);
    float getPriority() const { return _priority; }
    
    void setLoop(bool loop) { _loop = loop; }
    bool getLoop() const { return _loop; }
    
    void setHold(bool hold) { _hold = hold; }
    bool getHold() const { return _hold; }
    
    void setStartAutomatically(bool startAutomatically);
    bool getStartAutomatically() const { return _startAutomatically; }
    
    void setFirstFrame(float firstFrame) { _firstFrame = firstFrame; }
    float getFirstFrame() const { return _firstFrame; }
    
    void setLastFrame(float lastFrame) { _lastFrame = lastFrame; }
    float getLastFrame() const { return _lastFrame; }
    
    void setMaskedJoints(const QStringList& maskedJoints);
    const QStringList& getMaskedJoints() const { return _maskedJoints; }
    
    void setRunning(bool running);
    bool isRunning() const { return _running; }

    void setFrameIndex(float frameIndex) { _frameIndex = glm::clamp(_frameIndex, _firstFrame, _lastFrame); }
    float getFrameIndex() const { return _frameIndex; }

    AnimationDetails getAnimationDetails() const;

signals:
    
    void runningChanged(bool running);

public slots:

    void start() { setRunning(true); }
    void stop() { setRunning(false); }
    
private:

    friend class Model;

    AnimationHandle(Model* model);
        
    void simulate(float deltaTime);
    void applyFrame(float frameIndex);
    void replaceMatchingPriorities(float newPriority);
    
    Model* _model;
    WeakAnimationHandlePointer _self;
    AnimationPointer _animation;
    QString _role;
    QUrl _url;
    float _fps;
    float _priority;
    bool _loop;
    bool _hold;
    bool _startAutomatically;
    float _firstFrame;
    float _lastFrame;
    QStringList _maskedJoints;
    bool _running;
    QVector<int> _jointMappings;
    float _frameIndex;
};

#endif // hifi_Model_h
