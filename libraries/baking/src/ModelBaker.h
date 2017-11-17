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

using TextureBakerThreadGetter = std::function<QThread*()>;
using getMaterialIDCallback = std::function <int(int)>;
using getTextureTypeCallback = std::function<image::TextureUsage::Type()>;

class ModelBaker : public Baker {
    Q_OBJECT

public:
    ModelBaker(const QUrl& inputModelURL, TextureBakerThreadGetter inputTextureThreadGetter,
               const QString& bakedOutputDirectory, const QString& originalOutputDirectory);
    virtual ~ModelBaker();
    bool compressMesh(FBXMesh& mesh, bool hasDeformers, FBXNode& dracoMeshNode, getMaterialIDCallback materialIDCallback = NULL);
    QByteArray* compressTexture(QString textureFileName, getTextureTypeCallback textureTypeCallback = NULL);
    virtual void setWasAborted(bool wasAborted) override;

protected:
    void checkIfTexturesFinished();
    
    QHash<QByteArray, QByteArray> _textureContentMap;
    QUrl _modelURL;
    QString _bakedOutputDir;
    QString _originalOutputDir;
    QString _bakedModelFilePath;
    QDir _modelTempDir;
    QString _originalModelFilePath;

public slots:
    virtual void abort() override;

private slots:
    void handleBakedTexture();
    void handleAbortedTexture();

private:
    QString createBakedTextureFileName(const QFileInfo & textureFileInfo);
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
