//
//  FBXSerializer.h
//  libraries/model-serializers/src
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FBXSerializer_h
#define hifi_FBXSerializer_h

#include <QtGlobal>
#include <QMetaType>
#include <QSet>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Extents.h>
#include <Transform.h>
#include <shared/HifiTypes.h>

#include "FBX.h"
#include <hfm/HFMSerializer.h>

#include <graphics/Geometry.h>
#include <graphics/Material.h>

class QIODevice;
class FBXNode;

class TextureParam {
public:
    glm::vec4 cropping;
    QString UVSet;

    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scaling;
    uint8_t alphaSource;
    uint8_t currentTextureBlendMode;
    bool useMaterial;

    template <typename T>
    bool assign(T& ref, const T& v) {
        if (ref == v) {
            return false;
        } else {
            ref = v;
            isDefault = false;
            return true;
        }
    }

    bool isDefault;

    TextureParam() :
        cropping(0.0f),
        UVSet("map1"),
        translation(0.0f),
        rotation(0.0f),
        scaling(1.0f),
        alphaSource(0),
        currentTextureBlendMode(0),
        useMaterial(true),
        isDefault(true)
    {}

    TextureParam(const TextureParam& src) :
        cropping(src.cropping),
        UVSet(src.UVSet),
        translation(src.translation),
        rotation(src.rotation),
        scaling(src.scaling),
        alphaSource(src.alphaSource),
        currentTextureBlendMode(src.currentTextureBlendMode),
        useMaterial(src.useMaterial),
        isDefault(src.isDefault)
    {}

};

class MaterialParam {
public:
    glm::vec3 translation;
    glm::vec3 scaling;

    MaterialParam() :
        translation(0.0),
        scaling(1.0)
    {}

    MaterialParam(const MaterialParam& src) :
        translation(src.translation),
        scaling(src.scaling)
    {}
};

class ExtractedMesh;

class FBXSerializer : public HFMSerializer {
public:
    virtual ~FBXSerializer() {}

    MediaType getMediaType() const override;
    std::unique_ptr<hfm::Serializer::Factory> getFactory() const override;

    HFMModel* _hfmModel;
    /// Reads HFMModel from the supplied model and mapping data.
    /// \exception QString if an error occurs in parsing
    HFMModel::Pointer read(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url = hifi::URL()) override;

    FBXNode _rootNode;
    static FBXNode parseFBX(QIODevice* device);

    HFMModel* extractHFMModel(const hifi::VariantHash& mapping, const QString& url);

    static ExtractedMesh extractMesh(const FBXNode& object, unsigned int& meshIndex, bool deduplicate);
    QHash<QString, ExtractedMesh> meshes;

    HFMTexture getTexture(const QString& textureID, const QString& materialID);

    QHash<QString, QString> _textureNames;
    // Hashes the original RelativeFilename of textures
    QHash<QString, hifi::ByteArray> _textureFilepaths;
    // Hashes the place to look for textures, in case they are not inlined
    QHash<QString, hifi::ByteArray> _textureFilenames;
    // Hashes texture content by filepath, in case they are inlined
    QHash<hifi::ByteArray, hifi::ByteArray> _textureContent;
    QHash<QString, TextureParam> _textureParams;


    QHash<QString, QString> diffuseTextures;
    QHash<QString, QString> diffuseFactorTextures;
    QHash<QString, QString> transparentTextures;
    QHash<QString, QString> bumpTextures;
    QHash<QString, QString> normalTextures;
    QHash<QString, QString> specularTextures;
    QHash<QString, QString> metallicTextures;
    QHash<QString, QString> roughnessTextures;
    QHash<QString, QString> shininessTextures;
    QHash<QString, QString> emissiveTextures;
    QHash<QString, QString> ambientTextures;
    QHash<QString, QString> ambientFactorTextures;
    QHash<QString, QString> occlusionTextures;

    QHash<QString, HFMMaterial> _hfmMaterials;
    QHash<QString, MaterialParam> _materialParams;

    void consolidateHFMMaterials();

    bool _loadLightmaps { true };
    float _lightmapOffset { 0.0f };
    float _lightmapLevel { 1.0f };

    QMultiMap<QString, QString> _connectionParentMap;
    QMultiMap<QString, QString> _connectionChildMap;

    static glm::vec3 getVec3(const QVariantList& properties, int index);
    static QVector<glm::vec4> createVec4Vector(const QVector<double>& doubleVector);
    static QVector<glm::vec4> createVec4VectorRGBA(const QVector<double>& doubleVector, glm::vec4& average);
    static QVector<glm::vec3> createVec3Vector(const QVector<double>& doubleVector);
    static QVector<glm::vec2> createVec2Vector(const QVector<double>& doubleVector);
    static glm::mat4 createMat4(const QVector<double>& doubleVector);

    static QVector<int> getIntVector(const FBXNode& node);
    static QVector<float> getFloatVector(const FBXNode& node);
    static QVector<double> getDoubleVector(const FBXNode& node);
};

#endif // hifi_FBXSerializer_h
