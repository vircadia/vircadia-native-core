//
//  FBXReader.h
//  interface
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__FBXReader__
#define __interface__FBXReader__

#include <QVarLengthArray>
#include <QVariant>
#include <QVector>

#include <glm/glm.hpp>

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

    int parentIndex;
    glm::mat4 preRotation;
    glm::quat rotation;
    glm::mat4 postRotation;
    glm::mat4 transform;
};

/// A single binding to a joint in an FBX document.
class FBXCluster {
public:
    
    int jointIndex;
    glm::mat4 inverseBindMatrix;
};

/// A single mesh (with optional blendshapes) extracted from an FBX document.
class FBXMesh {
public:
    
    QVector<int> quadIndices;
    QVector<int> triangleIndices;
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> normals;
    QVector<glm::vec2> texCoords;
    QVector<glm::vec4> clusterIndices;
    QVector<glm::vec4> clusterWeights;
    
    QVector<FBXCluster> clusters;
    
    bool isEye;
    
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    float shininess;
    
    QByteArray diffuseFilename;
    QByteArray normalFilename;
    
    QVector<FBXBlendshape> blendshapes;
    
    float springiness;
    QVector<QPair<int, int> > springEdges;
    QVector<QVarLengthArray<QPair<int, int>, 4> > vertexConnections;
};

/// A set of meshes extracted from an FBX document.
class FBXGeometry {
public:

    QVector<FBXJoint> joints;
    
    QVector<FBXMesh> meshes;
    
    glm::mat4 offset;
    
    int leftEyeJointIndex;
    int rightEyeJointIndex;
    int neckJointIndex;
    
    glm::vec3 neckPivot;
};

/// Reads FBX geometry from the supplied model and mapping data.
/// \exception QString if an error occurs in parsing
FBXGeometry readFBX(const QByteArray& model, const QByteArray& mapping);

#endif /* defined(__interface__FBXReader__) */
