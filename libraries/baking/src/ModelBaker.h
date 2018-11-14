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

#include "ModelBakingLoggingCategory.h"

#include <gpu/Texture.h> 

#include <FBX.h>
#include <hfm/HFM.h>

using TextureBakerThreadGetter = std::function<QThread*()>;
using GetMaterialIDCallback = std::function <int(int)>;

static const QString BAKED_FBX_EXTENSION = ".baked.fbx";

class ModelBaker : public Baker {
    Q_OBJECT

public:
    ModelBaker(const QUrl& inputModelURL, TextureBakerThreadGetter inputTextureThreadGetter,
               const QString& bakedOutputDirectory, const QString& originalOutputDirectory = "");
    virtual ~ModelBaker();

    bool compressMesh(HFMMesh& mesh, bool hasDeformers, FBXNode& dracoMeshNode, GetMaterialIDCallback materialIDCallback = nullptr);
    QString compressTexture(QString textureFileName, image::TextureUsage::Type = image::TextureUsage::Type::DEFAULT_TEXTURE);
    virtual void setWasAborted(bool wasAborted) override;

    QUrl getModelURL() const { return _modelURL; }
    QString getBakedModelFilePath() const { return _bakedModelFilePath; }

public slots:
    virtual void abort() override;

protected:
    void checkIfTexturesFinished();
    void texturesFinished();
    void embedTextureMetaData();
    void exportScene();
    
    FBXNode _rootNode;
    QHash<QByteArray, QByteArray> _textureContentMap;
    QUrl _modelURL;
    QString _bakedOutputDir;
    QString _originalOutputDir;
    QString _bakedModelFilePath;
    QDir _modelTempDir;
    QString _originalModelFilePath;

private slots:
    void handleBakedTexture();
    void handleAbortedTexture();

private:
    QString createBaseTextureFileName(const QFileInfo & textureFileInfo);
    QUrl getTextureURL(const QFileInfo& textureFileInfo, QString relativeFileName, bool isEmbedded = false);
    void bakeTexture(const QUrl & textureURL, image::TextureUsage::Type textureType, const QDir & outputDir, 
                     const QString & bakedFilename, const QByteArray & textureContent);
    QString texturePathRelativeToModel(QUrl modelURL, QUrl textureURL);
    
    TextureBakerThreadGetter _textureThreadGetter;
    QMultiHash<QUrl, QSharedPointer<TextureBaker>> _bakingTextures;
    QHash<QString, int> _textureNameMatchCount;
    QHash<QUrl, QString> _remappedTexturePaths;
    bool _pendingErrorEmission{ false };
};

#endif // hifi_ModelBaker_h
