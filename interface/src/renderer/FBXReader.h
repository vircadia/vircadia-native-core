//
//  FBXReader.h
//  interface
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__FBXReader__
#define __interface__FBXReader__

#include <QUrl>
#include <QVarLengthArray>
#include <QVariant>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class FBXNode;

typedef QList<FBXNode> FBXNodeList;

/// A node within an FBX document.
class FBXNode {
public:
    
    QByteArray name;
    QVariantList properties;
    FBXNodeList children;
};

/// A single blendshape extracted from an FBX document.
class FBXBlendshape {
public:
    
    QVector<int> indices;
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> normals;
};

/// A single joint (transformation node) extracted from an FBX document.
class FBXJoint {
public:

    bool isFree;
    QVector<int> freeLineage;
    int parentIndex;
    glm::mat4 preTransform;
    glm::quat preRotation;
    glm::quat rotation;
    glm::quat postRotation;
    glm::mat4 postTransform;
    glm::mat4 transform;
    glm::quat inverseBindRotation;
};

/// A single binding to a joint in an FBX document.
class FBXCluster {
public:
    
    int jointIndex;
    glm::mat4 inverseBindMatrix;
};

/// A single part of a mesh (with the same material).
class FBXMeshPart {
public:
    
    QVector<int> quadIndices;
    QVector<int> triangleIndices;
    
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    float shininess;
    
    QByteArray diffuseFilename;
    QByteArray normalFilename;
};

/// A single mesh (with optional blendshapes) extracted from an FBX document.
class FBXMesh {
public:
    
    QVector<FBXMeshPart> parts;
    
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> normals;
    QVector<glm::vec3> tangents;
    QVector<glm::vec3> colors;
    QVector<glm::vec2> texCoords;
    QVector<glm::vec4> clusterIndices;
    QVector<glm::vec4> clusterWeights;
    
    QVector<FBXCluster> clusters;
    
    bool isEye;
    
    QVector<FBXBlendshape> blendshapes;
    
    float springiness;
    QVector<QPair<int, int> > springEdges;
    QVector<QVarLengthArray<QPair<int, int>, 4> > vertexConnections;
};

/// An attachment to an FBX document.
class FBXAttachment {
public:
    
    int jointIndex;
    QUrl url;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

/// A set of meshes extracted from an FBX document.
class FBXGeometry {
public:

    QVector<FBXJoint> joints;
    QHash<QString, int> jointIndices;
    
    QVector<FBXMesh> meshes;
    
    glm::mat4 offset;
    
    int leftEyeJointIndex;
    int rightEyeJointIndex;
    int neckJointIndex;
    int rootJointIndex;
    int leanJointIndex;
    int headJointIndex;
    int leftHandJointIndex;
    int rightHandJointIndex;
    
    glm::vec3 neckPivot;
    
    QVector<FBXAttachment> attachments;
};

/// Reads FBX geometry from the supplied model and mapping data.
/// \exception QString if an error occurs in parsing
FBXGeometry readFBX(const QByteArray& model, const QByteArray& mapping);

/// Reads SVO geometry from the supplied model data.
FBXGeometry readSVO(const QByteArray& model);

#endif /* defined(__interface__FBXReader__) */
