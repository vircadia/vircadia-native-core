//
//  HFM.h
//  libraries/hfm/src
//
//  Created by Sabrina Shanman on 2018/11/02.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFM_h_
#define hifi_HFM_h_

#include <QMetaType>
#include <QSet>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Extents.h>
#include <Transform.h>

#include <graphics/Geometry.h>
#include <graphics/Material.h>

const int MAX_NUM_PIXELS_FOR_FBX_TEXTURE = 2048 * 2048;

// High Fidelity Model namespace
namespace hfm {

/// A single blendshape.
class Blendshape {
public:
    QVector<int> indices;
    QVector<glm::vec3> vertices;
    QVector<glm::vec3> normals;
    QVector<glm::vec3> tangents;
};

struct JointShapeInfo {
    // same units and frame as Joint.translation
    glm::vec3 avgPoint;
    std::vector<float> dots;
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> debugLines;
};

/// A single joint (transformation node).
class Joint {
public:
    JointShapeInfo shapeInfo;
    QVector<int> freeLineage;
    bool isFree;
    int parentIndex;
    float distanceToParent;

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

    // geometric offset is applied in local space but does NOT affect children.
    bool hasGeometricOffset;
    glm::vec3 geometricTranslation;
    glm::quat geometricRotation;
    glm::vec3 geometricScaling;
};


/// A single binding to a joint.
class Cluster {
public:

    int jointIndex;
    glm::mat4 inverseBindMatrix;
    Transform inverseBindTransform;
};

/// A texture map.
class Texture {
public:
    QString id;
    QString name;
    QByteArray filename;
    QByteArray content;

    Transform transform;
    int maxNumPixels { MAX_NUM_PIXELS_FOR_FBX_TEXTURE };
    int texcoordSet;
    QString texcoordSetName;

    bool isBumpmap{ false };

    bool isNull() const { return name.isEmpty() && filename.isEmpty() && content.isEmpty(); }
};

/// A single part of a mesh (with the same material).
class MeshPart {
public:

    QVector<int> quadIndices; // original indices from the FBX mesh
    QVector<int> quadTrianglesIndices; // original indices from the FBX mesh of the quad converted as triangles
    QVector<int> triangleIndices; // original indices from the FBX mesh

    QString materialID;
};

class Material {
public:
    Material() {};
    Material(const glm::vec3& diffuseColor, const glm::vec3& specularColor, const glm::vec3& emissiveColor,
         float shininess, float opacity) :
        diffuseColor(diffuseColor),
        specularColor(specularColor),
        emissiveColor(emissiveColor),
        shininess(shininess),
        opacity(opacity)  {}

    void getTextureNames(QSet<QString>& textureList) const;
    void setMaxNumPixelsPerTexture(int maxNumPixels);

    glm::vec3 diffuseColor{ 1.0f };
    float diffuseFactor{ 1.0f };
    glm::vec3 specularColor{ 0.02f };
    float specularFactor{ 1.0f };

    glm::vec3 emissiveColor{ 0.0f };
    float emissiveFactor{ 0.0f };

    float shininess{ 23.0f };
    float opacity{ 1.0f };

    float metallic{ 0.0f };
    float roughness{ 1.0f };
    float emissiveIntensity{ 1.0f };
    float ambientFactor{ 1.0f };

    float bumpMultiplier { 1.0f }; // TODO: to be implemented

    QString materialID;
    QString name;
    QString shadingModel;
    graphics::MaterialPointer _material;

    Texture normalTexture;
    Texture albedoTexture;
    Texture opacityTexture;
    Texture glossTexture;
    Texture roughnessTexture;
    Texture specularTexture;
    Texture metallicTexture;
    Texture emissiveTexture;
    Texture occlusionTexture;
    Texture scatteringTexture;
    Texture lightmapTexture;
    glm::vec2 lightmapParams{ 0.0f, 1.0f };


    bool isPBSMaterial{ false };
    // THe use XXXMap are not really used to drive which map are going or not, debug only
    bool useNormalMap{ false };
    bool useAlbedoMap{ false };
    bool useOpacityMap{ false };
    bool useRoughnessMap{ false };
    bool useSpecularMap{ false };
    bool useMetallicMap{ false };
    bool useEmissiveMap{ false };
    bool useOcclusionMap{ false };

    bool needTangentSpace() const;
};

/// A single mesh (with optional blendshapes).
class Mesh {
public:

    QVector<MeshPart> parts;

    QVector<glm::vec3> vertices;
    QVector<glm::vec3> normals;
    QVector<glm::vec3> tangents;
    QVector<glm::vec3> colors;
    QVector<glm::vec2> texCoords;
    QVector<glm::vec2> texCoords1;
    QVector<uint16_t> clusterIndices;
    QVector<uint16_t> clusterWeights;
    QVector<int32_t> originalIndices;

    QVector<Cluster> clusters;

    Extents meshExtents;
    glm::mat4 modelTransform;

    QVector<Blendshape> blendshapes;

    unsigned int meshIndex; // the order the meshes appeared in the object file

    graphics::MeshPointer _mesh;
    bool wasCompressed { false };

    void createMeshTangents(bool generateFromTexCoords);
    void createBlendShapeTangents(bool generateTangents);
};

/**jsdoc
 * @typedef {object} FBXAnimationFrame
 * @property {Quat[]} rotations
 * @property {Vec3[]} translations
 */
/// A single animation frame.
class AnimationFrame {
public:
    QVector<glm::quat> rotations;
    QVector<glm::vec3> translations;
};

/// A light.
class Light {
public:
    QString name;
    Transform transform;
    float intensity;
    float fogValue;
    glm::vec3 color;

    Light() :
        name(),
        transform(),
        intensity(1.0f),
        fogValue(0.0f),
        color(1.0f)
    {}
};

/// The runtime model format.
class Model {
public:
    using Pointer = std::shared_ptr<Model>;

    QString originalURL;
    QString author;
    QString applicationName; ///< the name of the application that generated the model

    QVector<Joint> joints;
    QHash<QString, int> jointIndices; ///< 1-based, so as to more easily detect missing indices
    bool hasSkeletonJoints;

    QVector<Mesh> meshes;
    QVector<QString> scripts;

    QHash<QString, Material> materials;

    glm::mat4 offset; // This includes offset, rotation, and scale as specified by the FST file

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

    glm::vec3 neckPivot;

    Extents bindExtents;
    Extents meshExtents;

    QVector<AnimationFrame> animationFrames;

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

    QMap<int, glm::quat> jointRotationOffsets;
};

};

class ExtractedMesh {
public:
    hfm::Mesh mesh;
    QMultiHash<int, int> newIndices;
    QVector<QHash<int, int> > blendshapeIndexMaps;
    QVector<QPair<int, int> > partMaterialTextures;
    QHash<QString, size_t> texcoordSetMap;
};

typedef hfm::Blendshape HFMBlendshape;
typedef hfm::JointShapeInfo HFMJointShapeInfo;
typedef hfm::Joint HFMJoint;
typedef hfm::Cluster HFMCluster;
typedef hfm::Texture HFMTexture;
typedef hfm::MeshPart HFMMeshPart;
typedef hfm::Material HFMMaterial;
typedef hfm::Mesh HFMMesh;
typedef hfm::AnimationFrame HFMAnimationFrame;
typedef hfm::Light HFMLight;
typedef hfm::Model HFMModel;

Q_DECLARE_METATYPE(HFMAnimationFrame)
Q_DECLARE_METATYPE(QVector<HFMAnimationFrame>)
Q_DECLARE_METATYPE(HFMModel)
Q_DECLARE_METATYPE(HFMModel::Pointer)

#endif // hifi_HFM_h_
