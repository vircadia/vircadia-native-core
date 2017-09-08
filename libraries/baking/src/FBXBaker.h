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

    QUrl getFBXUrl() const { return _fbxURL; }
    QString getBakedFBXFilePath() const { return _bakedFBXFilePath; }

public slots:
    // all calls to FBXBaker::bake for FBXBaker instances must be from the same thread
    // because the Autodesk SDK will cause a crash if it is called from multiple threads
    virtual void bake() override;

signals:
    void sourceCopyReadyToLoad();

private slots:
    void bakeSourceCopy();
    void handleFBXNetworkReply();
    void handleBakedTexture();

private:
    void setupOutputFolder();

    void loadSourceFBX();

    void importScene();
    void rewriteAndBakeSceneTextures();
    void exportScene();
    void removeEmbeddedMediaFolder();

    void checkIfTexturesFinished();

    QString createBakedTextureFileName(const QFileInfo& textureFileInfo);
    QUrl getTextureURL(const QFileInfo& textureFileInfo, QString relativeFileName);

    void bakeTexture(const QUrl& textureURL, image::TextureUsage::Type textureType, const QDir& outputDir,
                     const QByteArray& textureContent = QByteArray());

    QUrl _fbxURL;

    FBXNode _rootNode;
    FBXGeometry _geometry;
    QHash<QByteArray, QByteArray> _textureContent;
    
    QString _bakedFBXFilePath;

    QString _bakedOutputDir;

    // If set, the original FBX and textures will also be copied here
    QString _originalOutputDir;

    QDir _tempDir;
    QString _originalFBXFilePath;

    QMultiHash<QUrl, QSharedPointer<TextureBaker>> _bakingTextures;
    QHash<QString, int> _textureNameMatchCount;

    TextureBakerThreadGetter _textureThreadGetter;

    bool _pendingErrorEmission { false };
};

#endif // hifi_FBXBaker_h
