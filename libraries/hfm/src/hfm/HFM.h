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

#include <image/ColorChannel.h>

#if defined(Q_OS_ANDROID)
#define HFM_PACK_NORMALS 0
#else
#define HFM_PACK_NORMALS 1
#endif

#if HFM_PACK_NORMALS
using NormalType = glm::uint32;
#define HFM_NORMAL_ELEMENT gpu::Element::VEC4F_NORMALIZED_XYZ10W2
#else
using NormalType = glm::vec3;
#define HFM_NORMAL_ELEMENT gpu::Element::VEC3F_XYZ
#endif

#define HFM_PACK_COLORS 1

#if HFM_PACK_COLORS
using ColorType = glm::uint32;
#define HFM_COLOR_ELEMENT gpu::Element::COLOR_RGBA_32
#else
using ColorType = glm::vec3;
#define HFM_COLOR_ELEMENT gpu::Element::VEC3F_XYZ
#endif

const int MAX_NUM_PIXELS_FOR_FBX_TEXTURE = 8192 * 8192;


using ShapeVertices = std::vector<glm::vec3>;
// The version of the Draco mesh binary data itself. See also: FBX_DRACO_MESH_VERSION in FBX.h
static const int DRACO_MESH_VERSION = 3;

static const int DRACO_BEGIN_CUSTOM_HIFI_ATTRIBUTES = 1000;
static const int DRACO_ATTRIBUTE_MATERIAL_ID = DRACO_BEGIN_CUSTOM_HIFI_ATTRIBUTES;
static const int DRACO_ATTRIBUTE_TEX_COORD_1 = DRACO_BEGIN_CUSTOM_HIFI_ATTRIBUTES + 1;
static const int DRACO_ATTRIBUTE_ORIGINAL_INDEX = DRACO_BEGIN_CUSTOM_HIFI_ATTRIBUTES + 2;

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
    image::ColorChannel sourceChannel { image::ColorChannel::NONE };

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

    glm::vec3 diffuseColor { 1.0f };
    float diffuseFactor { 1.0f };
    glm::vec3 specularColor { 0.02f };
    float specularFactor { 1.0f };

    glm::vec3 emissiveColor { 0.0f };
    float emissiveFactor { 0.0f };

    float shininess { 23.0f };
    float opacity { 1.0f };

    float metallic { 0.0f };
    float roughness { 1.0f };
    float emissiveIntensity { 1.0f };
    float ambientFactor { 1.0f };

    float bumpMultiplier { 1.0f }; // TODO: to be implemented

    graphics::MaterialKey::OpacityMapMode alphaMode { graphics::MaterialKey::OPACITY_MAP_BLEND };
    float alphaCutoff { 0.5f };

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
    glm::vec2 lightmapParams { 0.0f, 1.0f };


    bool isPBSMaterial { false };
    // THe use XXXMap are not really used to drive which map are going or not, debug only
    bool useNormalMap { false };
    bool useAlbedoMap { false };
    bool useOpacityMap { false };
    bool useRoughnessMap { false };
    bool useSpecularMap { false };
    bool useMetallicMap { false };
    bool useEmissiveMap { false };
    bool useOcclusionMap { false };

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
};

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

class FlowData {
public:
    FlowData() {};
    QVariantMap _physicsConfig;
    QVariantMap _collisionsConfig;
    bool shouldInitFlow() const { return _physicsConfig.size() > 0; }
    bool shouldInitCollisions() const { return _collisionsConfig.size() > 0; }
};

/// The runtime model format.
class Model {
public:
    using Pointer = std::shared_ptr<Model>;
    using ConstPointer = std::shared_ptr<const Model>;

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

    glm::vec3 neckPivot;

    Extents bindExtents;
    Extents meshExtents;

    QVector<AnimationFrame> animationFrames;

    int getJointIndex(const QString& name) const { return jointIndices.value(name) - 1; }
    QStringList getJointNames() const;

    bool hasBlendedMeshes() const;

    /// Returns the unscaled extents of the model's mesh
    Extents getUnscaledMeshExtents() const;
    const Extents& getMeshExtents() const { return meshExtents; }

    bool convexHullContains(const glm::vec3& point) const;

    QHash<int, QString> meshIndicesToModelNames;

    /// given a meshIndex this will return the name of the model that mesh belongs to if known
    QString getModelNameOfMesh(int meshIndex) const;
    void computeKdops();

    QList<QString> blendshapeChannelNames;

    QMap<int, glm::quat> jointRotationOffsets;
    std::vector<ShapeVertices> shapeVertices;
    FlowData flowData;

    void debugDump();
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
typedef hfm::FlowData FlowData;

Q_DECLARE_METATYPE(HFMAnimationFrame)
Q_DECLARE_METATYPE(QVector<HFMAnimationFrame>)
Q_DECLARE_METATYPE(HFMModel)
Q_DECLARE_METATYPE(HFMModel::Pointer)

#endif // hifi_HFM_h_
