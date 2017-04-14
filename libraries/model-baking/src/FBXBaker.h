//
//  FBXBaker.h
//  libraries/model-baking/src
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

namespace fbxsdk {
    class FbxManager;
    class FbxProperty;
    class FbxScene;
    class FbxFileTexture;
}

enum TextureType {
    DEFAULT_TEXTURE,
    STRICT_TEXTURE,
    ALBEDO_TEXTURE,
    NORMAL_TEXTURE,
    BUMP_TEXTURE,
    SPECULAR_TEXTURE,
    METALLIC_TEXTURE = SPECULAR_TEXTURE, // for now spec and metallic texture are the same, converted to grey
    ROUGHNESS_TEXTURE,
    GLOSS_TEXTURE,
    EMISSIVE_TEXTURE,
    CUBE_TEXTURE,
    OCCLUSION_TEXTURE,
    SCATTERING_TEXTURE = OCCLUSION_TEXTURE,
    LIGHTMAP_TEXTURE,
    CUSTOM_TEXTURE,
    UNUSED_TEXTURE = -1
};

class TextureBaker;

static const QString BAKED_FBX_EXTENSION = ".baked.fbx";

class FBXBaker : public QObject {
    Q_OBJECT
public:
    FBXBaker(const QUrl& fbxURL, const QString& baseOutputPath, bool copyOriginals = true);
    ~FBXBaker();

    Q_INVOKABLE void bake();

    bool hasErrors() const { return !_errorList.isEmpty(); }

    QUrl getFBXUrl() const { return _fbxURL; }
    QString getBakedFBXRelativePath() const { return _bakedFBXRelativePath; }

signals:
    void finished();
    void allTexturesBaked();

    void sourceCopyReadyToLoad();

private slots:
    void bakeSourceCopy();
    void handleFBXNetworkReply();
    void handleBakedTexture();

private:
    void setupOutputFolder();

    void loadSourceFBX();

    void bakeCopiedFBX();

    void importScene();
    void rewriteAndBakeSceneTextures();
    void exportScene();
    void removeEmbeddedMediaFolder();
    void possiblyCleanupOriginals();

    QString createBakedTextureFileName(const QFileInfo& textureFileInfo);
    QUrl getTextureURL(const QFileInfo& textureFileInfo, fbxsdk::FbxFileTexture* fileTexture);

    void bakeTexture(const QUrl& textureURL);

    QString pathToCopyOfOriginal() const;

    void handleError(const QString& error);

    QUrl _fbxURL;
    QString _fbxName;
    
    QString _baseOutputPath;
    QString _uniqueOutputPath;
    QString _bakedFBXRelativePath;

    fbxsdk::FbxManager* _sdkManager;
    fbxsdk::FbxScene* _scene { nullptr };

    QStringList _errorList;

    QHash<QUrl, QString> _unbakedTextures;
    QHash<QString, int> _textureNameMatchCount;
    QHash<uint64_t, TextureType> _textureTypes;

    std::list<std::unique_ptr<TextureBaker>> _bakingTextures;
    QFutureSynchronizer<void> _textureBakeSynchronizer;

    bool _copyOriginals { true };

    bool _finishedNonTextureOperations { false };
};

#endif // hifi_FBXBaker_h
