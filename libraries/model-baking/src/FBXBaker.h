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
#include <QtCore/QLoggingCategory>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>

Q_DECLARE_LOGGING_CATEGORY(model_baking)

namespace fbxsdk {
    class FbxManager;
    class FbxProperty;
    class FbxScene;
    class FbxTexture;
}

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
    
private:
    void bake();

    bool setupOutputFolder();
    bool importScene();
    bool rewriteAndCollectSceneTextures();
    bool exportScene();
    bool bakeTextures();
    bool bakeTexture();
    bool removeEmbeddedMediaFolder();

    QString pathToCopyOfOriginal() const;

    QUrl _fbxURL;
    QString _fbxName;
    
    QString _baseOutputPath;
    QString _uniqueOutputPath;

    fbxsdk::FbxManager* _sdkManager;
    fbxsdk::FbxScene* _scene { nullptr };

    QStringList _errorList;

    QSet<QUrl> _unbakedTextures;
};

#endif // hifi_FBXBaker_h
