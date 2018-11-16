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

#include <QtGlobal>
#include <QMetaType>
#include <QSet>
#include <QUrl>
#include <QVarLengthArray>
#include <QVariant>
#include <QVector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Extents.h>
#include <Transform.h>

#include "FBX.h"
#include <hfm/HFM.h>

#include <graphics/Geometry.h>
#include <graphics/Material.h>

class QIODevice;
class FBXNode;

/// Reads HFMModel from the supplied model and mapping data.
/// \exception QString if an error occurs in parsing
HFMModel* readFBX(const QByteArray& data, const QVariantHash& mapping, const QString& url = "", bool loadLightmaps = true, float lightmapLevel = 1.0f);

/// Reads HFMModel from the supplied model and mapping data.
/// \exception QString if an error occurs in parsing
HFMModel* readFBX(QIODevice* device, const QVariantHash& mapping, const QString& url = "", bool loadLightmaps = true, float lightmapLevel = 1.0f);

class TextureParam {
public:
    glm::vec2 UVTranslation;
    glm::vec2 UVScaling;
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
        UVTranslation(0.0f),
        UVScaling(1.0f),
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
        UVTranslation(src.UVTranslation),
        UVScaling(src.UVScaling),
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

class ExtractedMesh;

class FBXReader {
public:
    HFMModel* _hfmModel;

    FBXNode _rootNode;
    static FBXNode parseFBX(QIODevice* device);

    HFMModel* extractHFMModel(const QVariantHash& mapping, const QString& url);

    static ExtractedMesh extractMesh(const FBXNode& object, unsigned int& meshIndex, bool deduplicate = true);
    QHash<QString, ExtractedMesh> meshes;
    static void buildModelMesh(HFMMesh& extractedMesh, const QString& url);

    static glm::vec3 normalizeDirForPacking(const glm::vec3& dir);

    HFMTexture getTexture(const QString& textureID);

    QHash<QString, QString> _textureNames;
    // Hashes the original RelativeFilename of textures
    QHash<QString, QByteArray> _textureFilepaths;
    // Hashes the place to look for textures, in case they are not inlined
    QHash<QString, QByteArray> _textureFilenames;
    // Hashes texture content by filepath, in case they are inlined
    QHash<QByteArray, QByteArray> _textureContent;
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

    void consolidateHFMMaterials(const QVariantHash& mapping);

    bool _loadLightmaps = true;
    float _lightmapOffset = 0.0f;
    float _lightmapLevel;

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

#endif // hifi_FBXReader_h
