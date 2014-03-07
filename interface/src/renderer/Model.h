//
//  Model.h
//  interface
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Model__
#define __interface__Model__

#include <QObject>
#include <QUrl>

#include "GeometryCache.h"
#include "InterfaceConfig.h"
#include "ProgramObject.h"
#include "TextureCache.h"

class Shape;

/// A generic 3D model displaying geometry loaded from a URL.
class Model : public QObject {
    Q_OBJECT
    
public:

    Model(QObject* parent = NULL);
    virtual ~Model();
    
    void setTranslation(const glm::vec3& translation) { _translation = translation; }
    const glm::vec3& getTranslation() const { return _translation; }
    
    void setRotation(const glm::quat& rotation) { _rotation = rotation; }
    const glm::quat& getRotation() const { return _rotation; }
    
    void setScale(const glm::vec3& scale) { _scale = scale; }
    const glm::vec3& getScale() const { return _scale; }
    
    void setOffset(const glm::vec3& offset) { _offset = offset; }
    const glm::vec3& getOffset() const { return _offset; }
    
    void setPupilDilation(float dilation) { _pupilDilation = dilation; }
    float getPupilDilation() const { return _pupilDilation; }
    
    void setBlendshapeCoefficients(const std::vector<float>& coefficients) { _blendshapeCoefficients = coefficients; }
    const std::vector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }
    
    bool isActive() const { return _geometry && _geometry->isLoaded(); }
    
    bool isRenderable() const { return !_meshStates.isEmpty(); }
    
    bool isLoadedWithTextures() const { return _geometry && _geometry->isLoadedWithTextures(); }
    
    void init();
    void reset();
    void clearShapes();
    void createCollisionShapes();
    void updateShapePositions();
    void simulate(float deltaTime, bool delayLoad = false);
    bool render(float alpha);
    
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

    /// Returns the extents of the unmovable joints of the model.
    Extents getStaticExtents() const;
    
    /// Returns a reference to the shared geometry.
    const QSharedPointer<NetworkGeometry>& getGeometry() const { return _geometry; }
    
    /// Returns the number of joint states in the model.
    int getJointStateCount() const { return _jointStates.size(); }
    
    /// Fetches the joint state at the specified index.
    /// \return whether or not the joint state is "valid" (that is, non-default)
    bool getJointState(int index, glm::quat& rotation) const;
    
    /// Sets the joint state at the specified index.
    void setJointState(int index, bool valid, const glm::quat& rotation);
    
    /// Returns the index of the left hand joint, or -1 if not found.
    int getLeftHandJointIndex() const { return isActive() ? _geometry->getFBXGeometry().leftHandJointIndex : -1; }
    
    /// Returns the index of the right hand joint, or -1 if not found.
    int getRightHandJointIndex() const { return isActive() ? _geometry->getFBXGeometry().rightHandJointIndex : -1; }
    
    /// Returns the index of the parent of the indexed joint, or -1 if not found.
    int getParentJointIndex(int jointIndex) const;
    
    /// Returns the index of the last free ancestor of the indexed joint, or -1 if not found.
    int getLastFreeJointIndex(int jointIndex) const;
    
    /// Returns the position of the head joint.
    /// \return whether or not the head was found
    bool getHeadPosition(glm::vec3& headPosition) const;
    
    /// Returns the position of the neck joint.
    /// \return whether or not the neck was found
    bool getNeckPosition(glm::vec3& neckPosition) const;
    
    /// Returns the rotation of the neck joint.
    /// \return whether or not the neck was found
    bool getNeckRotation(glm::quat& neckRotation) const;
    
    /// Retrieve the positions of up to two eye meshes.
    /// \return whether or not both eye meshes were found
    bool getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const;
    
    /// Retrieve the position of the left hand
    /// \return true whether or not the position was found
    bool getLeftHandPosition(glm::vec3& position) const;
    
    /// Retrieve the rotation of the left hand
    /// \return true whether or not the rotation was found
    bool getLeftHandRotation(glm::quat& rotation) const;
    
    /// Retrieve the position of the right hand
    /// \return true whether or not the position was found
    bool getRightHandPosition(glm::vec3& position) const;
    
    /// Retrieve the rotation of the right hand
    /// \return true whether or not the rotation was found
    bool getRightHandRotation(glm::quat& rotation) const;
    
    /// Sets the position of the left hand using inverse kinematics.
    /// \return whether or not the left hand joint was found
    bool setLeftHandPosition(const glm::vec3& position);
    
    /// Restores some percentage of the default position of the left hand.
    /// \param percent the percentage of the default position to restore
    /// \return whether or not the left hand joint was found
    bool restoreLeftHandPosition(float percent = 1.0f);
    
    /// Sets the rotation of the left hand.
    /// \return whether or not the left hand joint was found
    bool setLeftHandRotation(const glm::quat& rotation);
    
    /// Gets the position of the left shoulder.
    /// \return whether or not the left shoulder joint was found
    bool getLeftShoulderPosition(glm::vec3& position) const;
    
    /// Returns the extended length from the left hand to its last free ancestor.
    float getLeftArmLength() const;
    
    /// Sets the position of the right hand using inverse kinematics.
    /// \return whether or not the right hand joint was found
    bool setRightHandPosition(const glm::vec3& position);
    
    /// Restores some percentage of the default position of the right hand.
    /// \param percent the percentage of the default position to restore
    /// \return whether or not the right hand joint was found
    bool restoreRightHandPosition(float percent = 1.0f);
    
    /// Sets the rotation of the right hand.
    /// \return whether or not the right hand joint was found
    bool setRightHandRotation(const glm::quat& rotation);
    
    /// Gets the position of the right shoulder.
    /// \return whether or not the right shoulder joint was found
    bool getRightShoulderPosition(glm::vec3& position) const;
    
    /// Returns the extended length from the right hand to its first free ancestor.
    float getRightArmLength() const;
    
    /// Returns the average color of all meshes in the geometry.
    glm::vec4 computeAverageColor() const;

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;
    
    /// \param shapes list of pointers shapes to test against Model
    /// \param collisions list to store collision results
    /// \return true if at least one shape collided agains Model
    bool findCollisions(const QVector<const Shape*> shapes, CollisionList& collisions);

    bool findSphereCollisions(const glm::vec3& penetratorCenter, float penetratorRadius,
        CollisionList& collisions, int skipIndex = -1);
    
    void renderCollisionProxies(float alpha);

    /// \param collision details about the collisions
    /// \return true if the collision is against a moveable joint
    bool collisionHitsMoveableJoint(CollisionInfo& collision) const;

    /// \param collision details about the collision
    /// Use the collision to affect the model
    void applyCollision(CollisionInfo& collision);

protected:

    QSharedPointer<NetworkGeometry> _geometry;
    
    glm::vec3 _translation;
    glm::quat _rotation;
    glm::vec3 _scale;
    glm::vec3 _offset;
    
    class JointState {
    public:
        glm::vec3 translation;  // translation relative to parent
        glm::quat rotation;     // rotation relative to parent
        glm::mat4 transform;    // rotation to world frame + translation in model frame
        glm::quat combinedRotation; // rotation from joint local to world frame
    };
    
    bool _shapesAreDirty;
    QVector<JointState> _jointStates;
    QVector<Shape*> _shapes;
    
    class MeshState {
    public:
        QVector<glm::mat4> clusterMatrices;
        QVector<glm::vec3> worldSpaceVertices;
        QVector<glm::vec3> vertexVelocities;
        QVector<glm::vec3> worldSpaceNormals;
    };
    
    QVector<MeshState> _meshStates;
    
    /// Updates the state of the joint at the specified index.
    virtual void updateJointState(int index);
    
    virtual void maybeUpdateLeanRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    virtual void maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    virtual void maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    
    bool getJointPosition(int jointIndex, glm::vec3& position) const;
    bool getJointRotation(int jointIndex, glm::quat& rotation, bool fromBind = false) const;
    
    bool setJointPosition(int jointIndex, const glm::vec3& position, int lastFreeIndex = -1,
        bool allIntermediatesFree = false, const glm::vec3& alignment = glm::vec3(0.0f, -1.0f, 0.0f));
    bool setJointRotation(int jointIndex, const glm::quat& rotation, bool fromBind = false);
    
    void setJointTranslation(int jointIndex, const glm::vec3& translation);
    
    /// Restores the indexed joint to its default position.
    /// \param percent the percentage of the default position to apply (i.e., 0.25f to slerp one fourth of the way to
    /// the original position
    /// \return true if the joint was found
    bool restoreJointPosition(int jointIndex, float percent = 1.0f);
    
    /// Computes and returns the extended length of the limb terminating at the specified joint and starting at the joint's
    /// first free ancestor.
    float getLimbLength(int jointIndex) const;
    
    void applyRotationDelta(int jointIndex, const glm::quat& delta, bool constrain = true);
    
private:
    
    QVector<JointState> updateGeometry(bool delayLoad);
    void applyNextGeometry();
    void deleteGeometry();
    void renderMeshes(float alpha, bool translucent);
    
    QSharedPointer<NetworkGeometry> _baseGeometry; ///< reference required to prevent collection of base
    QSharedPointer<NetworkGeometry> _nextBaseGeometry;
    QSharedPointer<NetworkGeometry> _nextGeometry;
    float _lodDistance;
    float _lodHysteresis;
    float _nextLODHysteresis;
    
    float _pupilDilation;
    std::vector<float> _blendshapeCoefficients;
    
    QUrl _url;
        
    QVector<GLuint> _blendedVertexBufferIDs;
    QVector<QVector<QSharedPointer<Texture> > > _dilatedTextures;
    bool _resetStates;
    
    QVector<glm::vec3> _blendedVertices;
    QVector<glm::vec3> _blendedNormals;
    
    QVector<Model*> _attachments;
    
    static ProgramObject _program;
    static ProgramObject _normalMapProgram;
    static ProgramObject _skinProgram;
    static ProgramObject _skinNormalMapProgram;
    
    static int _normalMapTangentLocation;
    
    class SkinLocations {
    public:
        int clusterMatrices;
        int clusterIndices;
        int clusterWeights;
        int tangent;
    };
    
    static SkinLocations _skinLocations;
    static SkinLocations _skinNormalMapLocations;
    
    static void initSkinProgram(ProgramObject& program, SkinLocations& locations);
    static QVector<JointState> createJointStates(const FBXGeometry& geometry);
};

#endif /* defined(__interface__Model__) */
