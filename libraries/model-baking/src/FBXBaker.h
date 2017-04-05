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

#include <QLoggingCategory>
#include <QUrl>

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
    FBXBaker(QUrl fbxPath);
    ~FBXBaker();

    void start();

signals:
    void finished(QStringList errorList);

private:
    void bake();
    bool importScene();
    bool rewriteAndCollectSceneTextures();
    bool exportScene();
    bool bakeTextures();
    bool bakeTexture();

    QUrl _fbxPath;
    fbxsdk::FbxManager* _sdkManager;
    fbxsdk::FbxScene* _scene { nullptr };

    QStringList _errorList;

    QList<QUrl> _unbakedTextures;
};

#endif // hifi_FBXBaker_h
