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

#include <string>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(model_baking)

class FBXBaker {

public:
    FBXBaker(std::string fbxPath);
    bool bakeFBX();

private:
    bool importScene();
    bool rewriteAndCollectSceneTextures();
    bool rewriteAndCollectChannelTextures(FbxProperty& property);
    bool rewriteAndCollectTexture(FbxTexture* texture);
    
    std::string _fbxPath;
    FbxManager* _sdkManager;
    FbxScene* _scene { nullptr };
};

#endif // hifi_FBXBaker_h
