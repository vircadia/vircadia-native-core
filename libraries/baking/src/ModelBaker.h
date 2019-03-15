//
//  ModelBaker.h
//  libraries/baking/src
//
//  Created by Utkarsh Gautam on 9/29/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelBaker_h
#define hifi_ModelBaker_h

#include <QtCore/QFutureSynchronizer>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

#include "Baker.h"
#include "TextureBaker.h"
#include "baking/TextureFileNamer.h"

#include "ModelBakingLoggingCategory.h"

#include <gpu/Texture.h> 

#include <FBX.h>
#include <hfm/HFM.h>

using TextureBakerThreadGetter = std::function<QThread*()>;
using GetMaterialIDCallback = std::function <int(int)>;

static const QString FST_EXTENSION { ".fst" };
static const QString BAKED_FST_EXTENSION { ".baked.fst" };
static const QString FBX_EXTENSION { ".fbx" };
static const QString BAKED_FBX_EXTENSION { ".baked.fbx" };
static const QString OBJ_EXTENSION { ".obj" };
static const QString GLTF_EXTENSION { ".gltf" };

class ModelBaker : public Baker {
    Q_OBJECT

public:
    using TextureKey = QPair<QUrl, image::TextureUsage::Type>;

    ModelBaker(const QUrl& inputModelURL, TextureBakerThreadGetter inputTextureThreadGetter,
               const QString& bakedOutputDirectory, const QString& originalOutputDirectory = "", bool hasBeenBaked = false);
    virtual ~ModelBaker();

    void setOutputURLSuffix(const QUrl& urlSuffix);
    void setMappingURL(const QUrl& mappingURL);
    void setMapping(const hifi::VariantHash& mapping);

    void initializeOutputDirs();

    bool buildDracoMeshNode(FBXNode& dracoMeshNode, const QByteArray& dracoMeshBytes, const std::vector<hifi::ByteArray>& dracoMaterialList);
    QString compressTexture(QString textureFileName, image::TextureUsage::Type = image::TextureUsage::Type::DEFAULT_TEXTURE);
    virtual void setWasAborted(bool wasAborted) override;

    QUrl getModelURL() const { return _modelURL; }
    virtual QUrl getFullOutputMappingURL() const;
    QUrl getBakedModelURL() const { return _bakedModelURL; }

signals:
    void modelLoaded();

public slots:
    virtual void bake() override;
    virtual void abort() override;

protected:
    void saveSourceModel();
    virtual void bakeProcessedSource(const hfm::Model::Pointer& hfmModel, const std::vector<hifi::ByteArray>& dracoMeshes, const std::vector<std::vector<hifi::ByteArray>>& dracoMaterialLists) = 0;
    void checkIfTexturesFinished();
    void texturesFinished();
    void embedTextureMetaData();
    void exportScene();

    FBXNode _rootNode;
    QHash<QByteArray, QByteArray> _textureContentMap;
    QUrl _modelURL;
    QUrl _outputURLSuffix;
    QUrl _mappingURL;
    hifi::VariantHash _mapping;
    QString _bakedOutputDir;
    QString _originalOutputDir;
    TextureBakerThreadGetter _textureThreadGetter;
    QString _outputMappingURL;
    QUrl _bakedModelURL;
    QDir _modelTempDir;
    QString _originalModelFilePath;

protected slots:
    void handleModelNetworkReply();
    virtual void bakeSourceCopy();

private slots:
    void handleBakedTexture();
    void handleAbortedTexture();

private:
    QUrl getTextureURL(const QFileInfo& textureFileInfo, bool isEmbedded = false);
    void bakeTexture(const TextureKey& textureKey, const QDir& outputDir, const QString& bakedFilename, const QByteArray& textureContent);
    QString texturePathRelativeToModel(QUrl modelURL, QUrl textureURL);

    QMultiHash<TextureKey, QSharedPointer<TextureBaker>> _bakingTextures;
    QHash<QString, int> _textureNameMatchCount;
    bool _pendingErrorEmission { false };

    bool _hasBeenBaked { false };

    TextureFileNamer _textureFileNamer;
};

#endif // hifi_ModelBaker_h
