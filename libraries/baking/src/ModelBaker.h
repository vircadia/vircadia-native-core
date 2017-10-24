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
using getTextureContentTypeCallback = std::function<QPair<QByteArray, image::TextureUsage::Type>()>;

class ModelBaker : public Baker{
    Q_OBJECT

public:
    ModelBaker();
    bool compressMesh(FBXMesh& mesh, bool hasDeformers, FBXNode& dracoMeshNode, getMaterialIDCallback materialIDCallback = NULL);
    QByteArray* compressTexture(QString textureFileName, QUrl modelUrl, QString bakedOutputDir, TextureBakerThreadGetter textureThreadGetter, 
                                getTextureContentTypeCallback textureContentTypeCallback = NULL, const QString& originalOutputDir = "");
    virtual void setWasAborted(bool wasAborted) override;

public slots:
    virtual void bake() override;

private slots:
    void handleBakedTexture();
    void handleAbortedTexture();

private:
    QString createBakedTextureFileName(const QFileInfo & textureFileInfo);
    QUrl getTextureURL(const QFileInfo& textureFileInfo, QString relativeFileName, bool isEmbedded = false);
    void bakeTexture(const QUrl & textureURL, image::TextureUsage::Type textureType, const QDir & outputDir, 
                     const QString & bakedFilename, const QByteArray & textureContent);
    QString texturePathRelativeToModel(QUrl fbxURL, QUrl textureURL);
    void checkIfTexturesFinished();
    
    QHash<QString, int> _textureNameMatchCount;
    QHash<QUrl, QString> _remappedTexturePaths;
    QUrl _modelURL;
    QMultiHash<QUrl, QSharedPointer<TextureBaker>> _bakingTextures;
    TextureBakerThreadGetter _textureThreadGetter;
    QString _originalOutputDir;
    bool _pendingErrorEmission{ false };
};

#endif // hifi_ModelBaker_h
