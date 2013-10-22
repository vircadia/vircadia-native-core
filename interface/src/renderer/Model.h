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

    Model();
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
    
    /// Updates the state of the joint at the specified index.
    virtual void updateJointState(int index);
    
    virtual void maybeUpdateLeanRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    virtual void maybeUpdateNeckRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    virtual void maybeUpdateEyeRotation(const JointState& parentState, const FBXJoint& joint, JointState& state);
    
private:
    
    bool getJointPosition(int jointIndex, glm::vec3& position) const;
    bool getJointRotation(int jointIndex, glm::quat& rotation) const;
    
    void deleteGeometry();
    
    float _pupilDilation;
    std::vector<float> _blendshapeCoefficients;
    
    QUrl _url;
    
    class MeshState {
    public:
        QVector<glm::mat4> clusterMatrices;
        QVector<glm::vec3> worldSpaceVertices;
        QVector<glm::vec3> vertexVelocities;
        QVector<glm::vec3> worldSpaceNormals;
    };
    
    QVector<MeshState> _meshStates;
    QVector<GLuint> _blendedVertexBufferIDs;
    QVector<QVector<QSharedPointer<Texture> > > _dilatedTextures;
    bool _resetStates;
    
    QVector<glm::vec3> _blendedVertices;
    QVector<glm::vec3> _blendedNormals;
    
    static ProgramObject _program;
    static ProgramObject _skinProgram;
    static int _clusterMatricesLocation;
    static int _clusterIndicesLocation;
    static int _clusterWeightsLocation;
};

#endif /* defined(__interface__Model__) */
