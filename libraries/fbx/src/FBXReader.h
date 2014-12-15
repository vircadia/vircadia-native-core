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

#include <QMetaType>
#include <QUrl>
#include <QVarLengthArray>
#include <QVariant>
#include <QVector>

#include <Extents.h>
#include <Transform.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class QIODevice;

class FBXNode;

typedef QList<FBXNode> FBXNodeList;

/// The names of the blendshapes expected by Faceshift, terminated with an empty string.
extern const char* FACESHIFT_BLENDSHAPES[];
/// The size of FACESHIFT_BLENDSHAPES
extern const int NUM_FACESHIFT_BLENDSHAPES;

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

enum ShapeType {
    SHAPE_TYPE_SPHERE = 0,
    SHAPE_TYPE_CAPSULE = 1,
    SHAPE_TYPE_UNKNOWN = 2
};

/// A single joint (transformation node) extracted from an FBX document.
class FBXJoint {
public:

    bool isFree;
    QVector<int> freeLineage;
    int parentIndex;
    float distanceToParent;
    float boneRadius;
    glm::vec3 translation;
    glm::mat4 preTransform;
    glm::quat preRotation;
    glm::quat rotation;
    glm::quat postRotation;
    glm::mat4 postTransform;
    glm::mat4 transform;
    glm::vec3 rotationMin;  // radians
    glm::vec3 rotationMax;  // radians
    glm::quat inverseDefaultRotation;
    glm::quat inverseBindRotation;
    glm::mat4 bindTransform;
    QString name;
    glm::vec3 shapePosition;  // in joint frame
    glm::quat shapeRotation;  // in joint frame
    ShapeType shapeType;
    bool isSkeletonJoint;
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
    std::string texcoordSetName;
};

/// A single part of a mesh (with the same material).
class FBXMeshPart {
public:
    
    QVector<int> quadIndices;
    QVector<int> triangleIndices;
    
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
    glm::vec3 color;

    FBXLight() :
        name(),
        transform(),
        intensity(1.0f),
        color(1.0f)
    {}
};

Q_DECLARE_METATYPE(FBXAnimationFrame)
Q_DECLARE_METATYPE(QVector<FBXAnimationFrame>)

/// An attachment to an FBX document.
class FBXAttachment {
public:
    
    int jointIndex;
    QUrl url;
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

/// A point where an avatar can sit
class SittingPoint {
public:
    QString name;
    glm::vec3 position; // relative postion
    glm::quat rotation; // relative orientation
};

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
    
    int leftEyeJointIndex;
    int rightEyeJointIndex;
    int neckJointIndex;
    int rootJointIndex;
    int leanJointIndex;
    int headJointIndex;
    int leftHandJointIndex;
    int rightHandJointIndex;
    int leftToeJointIndex;
    int rightToeJointIndex;
    
    QVector<int> humanIKJointIndices;
    
    glm::vec3 palmDirection;
    
    QVector<SittingPoint> sittingPoints;
    
    glm::vec3 neckPivot;
    
    Extents bindExtents;
    Extents meshExtents;
    
    QVector<FBXAnimationFrame> animationFrames;
    
    QVector<FBXAttachment> attachments;
    
    int getJointIndex(const QString& name) const { return jointIndices.value(name) - 1; }
    QStringList getJointNames() const;
    
    bool hasBlendedMeshes() const;

    /// Returns the unscaled extents of the model's mesh
    Extents getUnscaledMeshExtents() const;


    QHash<int, QString> meshIndicesToModelNames;
    
    /// given a meshIndex this will return the name of the model that mesh belongs to if known
    QString getModelNameOfMesh(int meshIndex) const;
};

Q_DECLARE_METATYPE(FBXGeometry)

/// Reads an FST mapping from the supplied data.
QVariantHash readMapping(const QByteArray& data);

/// Writes an FST mapping to a byte array.
QByteArray writeMapping(const QVariantHash& mapping);

/// Reads FBX geometry from the supplied model and mapping data.
/// \exception QString if an error occurs in parsing
FBXGeometry readFBX(const QByteArray& model, const QVariantHash& mapping, bool loadLightmaps = true, float lightmapLevel = 1.0f);

/// Reads FBX geometry from the supplied model and mapping data.
/// \exception QString if an error occurs in parsing
FBXGeometry readFBX(QIODevice* device, const QVariantHash& mapping, bool loadLightmaps = true, float lightmapLevel = 1.0f);

/// Reads SVO geometry from the supplied model data.
FBXGeometry readSVO(const QByteArray& model);

#endif // hifi_FBXReader_h
