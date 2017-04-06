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

#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

namespace fbxsdk {
    class FbxManager;
    class FbxProperty;
    class FbxScene;
    class FbxTexture;
}

class TextureBaker;

class FBXBaker : public QObject {
    Q_OBJECT
public:
    FBXBaker(QUrl fbxURL, QString baseOutputPath);
    ~FBXBaker();

    void start();

signals:
    void finished();

private slots:
    void handleFBXNetworkReply();
    void handleBakedTexture();
    
private:
    void bake();

    bool setupOutputFolder();
    bool importScene();
    bool rewriteAndBakeSceneTextures();
    bool exportScene();
    bool removeEmbeddedMediaFolder();

    void bakeTexture(const QUrl& textureURL);

    QString pathToCopyOfOriginal() const;

    QUrl _fbxURL;
    QString _fbxName;
    
    QString _baseOutputPath;
    QString _uniqueOutputPath;

    fbxsdk::FbxManager* _sdkManager;
    fbxsdk::FbxScene* _scene { nullptr };

    QStringList _errorList;

    QHash<QUrl, QString> _unbakedTextures;
    QHash<QString, int> _textureNameMatchCount;

    QHash<QUrl, QByteArray> _downloadedTextures;

    std::list<std::unique_ptr<TextureBaker>> _bakingTextures;
};

#endif // hifi_FBXBaker_h
