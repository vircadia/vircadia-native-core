//
//  FBXBaker.h
//  tools/baking/src
//
//  Created by Stephen Birarda on 3/30/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FBXBaker_h
#define hifi_FBXBaker_h

#include <QtCore/QFutureSynchronizer>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

#include "Baker.h"
#include "TextureBaker.h"

#include "ModelBakingLoggingCategory.h"

#include <gpu/Texture.h> 

#include <FBX.h>

static const QString BAKED_FBX_EXTENSION = ".baked.fbx";

using TextureBakerThreadGetter = std::function<QThread*()>;

class FBXBaker : public Baker {
    Q_OBJECT
public:
    FBXBaker(const QUrl& fbxURL, TextureBakerThreadGetter textureThreadGetter,
             const QString& bakedOutputDir, const QString& originalOutputDir = "");
    ~FBXBaker() override;

    QUrl getFBXUrl() const { return _fbxURL; }
    QString getBakedFBXFilePath() const { return _bakedFBXFilePath; }

    virtual void setWasAborted(bool wasAborted) override;

public slots:
    virtual void bake() override;
    virtual void abort() override;

signals:
    void sourceCopyReadyToLoad();

private slots:
    void bakeSourceCopy();
    void handleFBXNetworkReply();
    void handleBakedTexture();
    void handleAbortedTexture();

private:
    void setupOutputFolder();

    void loadSourceFBX();

    void importScene();
    void rewriteAndBakeSceneModels();
    void rewriteAndBakeSceneTextures();
    void exportScene();
    void removeEmbeddedMediaFolder();

    void checkIfTexturesFinished();

    QString createBakedTextureFileName(const QFileInfo& textureFileInfo);
    QUrl getTextureURL(const QFileInfo& textureFileInfo, QString relativeFileName, bool isEmbedded = false);

    void bakeTexture(const QUrl& textureURL, image::TextureUsage::Type textureType, const QDir& outputDir,
                     const QString& bakedFilename, const QByteArray& textureContent = QByteArray());

    QUrl _fbxURL;

    FBXNode _rootNode;
    FBXGeometry* _geometry;
    QHash<QByteArray, QByteArray> _textureContent;
    
    QString _bakedFBXFilePath;

    QString _bakedOutputDir;

    // If set, the original FBX and textures will also be copied here
    QString _originalOutputDir;

    QDir _tempDir;
    QString _originalFBXFilePath;

    QMultiHash<QUrl, QSharedPointer<TextureBaker>> _bakingTextures;
    QHash<QString, int> _textureNameMatchCount;
    QHash<QUrl, QString> _remappedTexturePaths;

    TextureBakerThreadGetter _textureThreadGetter;

    bool _pendingErrorEmission { false };
};

#endif // hifi_FBXBaker_h
