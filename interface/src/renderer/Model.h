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
    
    void init();
    void reset();
    void simulate(float deltaTime);
    bool render(float alpha);
    
    Q_INVOKABLE void setURL(const QUrl& url);
    const QUrl& getURL() const { return _url; }
    
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
    
    /// Returns the average color of all meshes in the geometry.
    glm::vec4 computeAverageColor() const;

protected:

    QSharedPointer<NetworkGeometry> _geometry;
    
    glm::vec3 _translation;
    glm::quat _rotation;
    glm::vec3 _scale;
    glm::vec3 _offset;
    
    class JointState {
    public:
        glm::quat rotation;
        glm::mat4 transform;
        glm::quat combinedRotation;
    };
    
    QVector<JointState> _jointStates;
    
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
    bool getJointRotation(int jointIndex, glm::quat& rotation) const;
    
    bool setJointPosition(int jointIndex, const glm::vec3& position);
    bool setJointRotation(int jointIndex, const glm::quat& rotation);
    
    /// Restores the indexed joint to its default position.
    /// \param percent the percentage of the default position to apply (i.e., 0.25f to slerp one fourth of the way to
    /// the original position
    /// \return true if the joint was found
    bool restoreJointPosition(int jointIndex, float percent = 1.0f);
    
private:
    
    void deleteGeometry();
    
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
};

#endif /* defined(__interface__Model__) */
