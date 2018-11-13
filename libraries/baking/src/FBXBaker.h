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
#include "ModelBaker.h"
#include "ModelBakingLoggingCategory.h"

#include <gpu/Texture.h> 

#include <FBX.h>

using TextureBakerThreadGetter = std::function<QThread*()>;

class FBXBaker : public ModelBaker {
    Q_OBJECT
public:
    using ModelBaker::ModelBaker;

public slots:
    virtual void bake() override;

signals:
    void sourceCopyReadyToLoad();

private slots:
    void bakeSourceCopy();
    void handleFBXNetworkReply();

private:
    void setupOutputFolder();

    void loadSourceFBX();

    void importScene();
    void embedTextureMetaData();
    void rewriteAndBakeSceneModels();
    void rewriteAndBakeSceneTextures();

    HFMModel* _hfmModel;
    QHash<QString, int> _textureNameMatchCount;
    QHash<QUrl, QString> _remappedTexturePaths;

    bool _pendingErrorEmission { false };
};

#endif // hifi_FBXBaker_h
