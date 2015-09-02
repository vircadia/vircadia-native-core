//
//  FBXReader.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FBXReader_h
#define hifi_FBXReader_h

#define USE_MODEL_MESH 1

#include <QMetaType>
#include <QUrl>
#include <QVarLengthArray>
#include <QVariant>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Extents.h>
#include <Transform.h>

#include <model/Geometry.h>
#include <model/Material.h>

class QIODevice;
class FBXNode;

typedef QList<FBXNode> FBXNodeList;

/// The names of the joints in the Maya HumanIK rig, terminated with an empty string.
extern const char* HUMANIK_JOINTS[];

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
    float distanceToParent;
    float boneRadius;

    // http://download.autodesk.com/us/fbx/20112/FBX_SDK_HELP/SDKRef/a00209.html

    glm::vec3 translation;   // T
    glm::mat4 preTransform;  // Roff * Rp
    glm::quat preRotation;   // Rpre
    glm::quat rotation;      // R
    glm::quat postRotation;  // Rpost
    glm::mat4 postTransform; // Rp-1 * Soff * Sp * S * Sp-1

    // World = ParentWorld * T * (Roff * Rp) * Rpre * R * Rpost * (Rp-1 * Soff * Sp * S * Sp-1)

    glm::mat4 transform;
    glm::vec3 rotationMin;  // radians
    glm::vec3 rotationMax;  // radians
    glm::quat inverseDefaultRotation;
    glm::quat inverseBindRotation;
    glm::mat4 bindTransform;
    QString name;
    bool isSkeletonJoint;
    bool bindTransformFoundInCluster;
};


/// A single binding to a joint in an FBX document.
class FBXCluster {
public:
    
    int jointIndex;
    glm::mat4 inverseBindMatrix;
};

/// A texture map in an FBX document.
class FBXTexture {
public:
    QString name;
    QByteArray filename;
    QByteArray content;
    
    Transform transform;
    int texcoordSet;
    QString texcoordSetName;
};

/// A single part of a mesh (with the same material).
class FBXMeshPart {
public:
    
    QVector<int> quadIndices; // original indices from the FBX mesh
    QVector<int> triangleIndices; // original indices from the FBX mesh
    mutable gpu::BufferPointer quadsAsTrianglesIndicesBuffer;

    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    glm::vec3 emissiveColor;
    glm::vec2 emissiveParams;
    float shininess;
    float opacity;
    
    FBXTexture diffuseTexture;
    FBXTexture normalTexture;
    FBXTexture specularTexture;
    FBXTexture emissiveTexture;

    QString materialID;
    model::MaterialPointer _material;
    mutable bool trianglesForQuadsAvailable = false;
    mutable int trianglesForQuadsIndicesCount = 0;

    gpu::BufferPointer getTrianglesForQuads() const;
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
    QVector<glm::vec2> texCoords1;
    QVector<glm::vec4> clusterIndices;
    QVector<glm::vec4> clusterWeights;
    
    QVector<FBXCluster> clusters;

    Extents meshExtents;
    glm::mat4 modelTransform;

    bool isEye;
    
    QVector<FBXBlendshape> blendshapes;
    
    bool hasSpecularTexture() const;
    bool hasEmissiveTexture() const;

    unsigned int meshIndex; // the order the meshes appeared in the object file
#   if USE_MODEL_MESH
    model::Mesh _mesh;
#   endif
};

/// A single animation frame extracted from an FBX document.
class FBXAnimationFrame {
public:
    
    QVector<glm::quat> rotations;
};

/// A light in an FBX document.
class FBXLight {
public:
    QString name;
    Transform transform;
    float intensity;
    float fogValue;
    glm::vec3 color;

    FBXLight() :
        name(),
        transform(),
        intensity(1.0f),
        fogValue(0.0f),
        color(1.0f)
    {}
};

Q_DECLARE_METATYPE(FBXAnimationFrame)
Q_DECLARE_METATYPE(QVector<FBXAnimationFrame>)

/// A point where an avatar can sit
class SittingPoint {
public:
    QString name;
    glm::vec3 position; // relative postion
    glm::quat rotation; // relative orientation
};

inline bool operator==(const SittingPoint& lhs, const SittingPoint& rhs)
{
    return (lhs.name == rhs.name) && (lhs.position == rhs.position) && (lhs.rotation == rhs.rotation);
}

inline bool operator!=(const SittingPoint& lhs, const SittingPoint& rhs)
{
    return (lhs.name != rhs.name) || (lhs.position != rhs.position) || (lhs.rotation != rhs.rotation);
}

/// A set of meshes extracted from an FBX document.
class FBXGeometry {
public:

    QString author;
    QString applicationName; ///< the name of the application that generated the model

    QVector<FBXJoint> joints;
    QHash<QString, int> jointIndices; ///< 1-based, so as to more easily detect missing indices
    bool hasSkeletonJoints;
    
    QVector<FBXMesh> meshes;
    
    glm::mat4 offset;
    
    int leftEyeJointIndex = -1;
    int rightEyeJointIndex = -1;
    int neckJointIndex = -1;
    int rootJointIndex = -1;
    int leanJointIndex = -1;
    int headJointIndex = -1;
    int leftHandJointIndex = -1;
    int rightHandJointIndex = -1;
    int leftToeJointIndex = -1;
    int rightToeJointIndex = -1;

    float leftEyeSize = 0.0f;  // Maximum mesh extents dimension
    float rightEyeSize = 0.0f;

    QVector<int> humanIKJointIndices;
    
    glm::vec3 palmDirection;
    
    QVector<SittingPoint> sittingPoints;
    
    glm::vec3 neckPivot;
    
    Extents bindExtents;
    Extents meshExtents;
    
    QVector<FBXAnimationFrame> animationFrames;
        
    int getJointIndex(const QString& name) const { return jointIndices.value(name) - 1; }
    QStringList getJointNames() const;
    
    bool hasBlendedMeshes() const;

    /// Returns the unscaled extents of the model's mesh
    Extents getUnscaledMeshExtents() const;

    bool convexHullContains(const glm::vec3& point) const;

    QHash<int, QString> meshIndicesToModelNames;
    
    /// given a meshIndex this will return the name of the model that mesh belongs to if known
    QString getModelNameOfMesh(int meshIndex) const;
    
    QList<QString> blendshapeChannelNames;
};

Q_DECLARE_METATYPE(FBXGeometry)

/// Reads FBX geometry from the supplied model and mapping data.
/// \exception QString if an error occurs in parsing
FBXGeometry* readFBX(const QByteArray& model, const QVariantHash& mapping, const QString& url = "", bool loadLightmaps = true, float lightmapLevel = 1.0f);

/// Reads FBX geometry from the supplied model and mapping data.
/// \exception QString if an error occurs in parsing
FBXGeometry* readFBX(QIODevice* device, const QVariantHash& mapping, const QString& url = "", bool loadLightmaps = true, float lightmapLevel = 1.0f);

#endif // hifi_FBXReader_h
