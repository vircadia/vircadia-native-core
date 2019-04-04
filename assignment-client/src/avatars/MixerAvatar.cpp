//
// MixerAvatar.cpp
// assignment-client/src/avatars
//
// Created by Simon Walton April 2019
// 
// Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <ResourceManager.h>

#include "MixerAvatar.h"
#include "AvatarLogging.h"

void MixerAvatar::fetchAvatarFST() {
    _avatarFSTValid = false;
    auto resourceManager = DependencyManager::get<ResourceManager>();
    QUrl avatarURL = getSkeletonModelURL();
    if (avatarURL.isEmpty()) {
        return;
    }
    _avatarURLString = avatarURL.toDisplayString();

    ResourceRequest * fstRequest = resourceManager->createResourceRequest(this, avatarURL);
    if (fstRequest) {
        QMutexLocker certifyLocker(&_avatarCertifyLock);

        if (_avatarRequest) {
            _avatarRequest->deleteLater();
        }
        _avatarRequest = fstRequest;
        connect(fstRequest, &ResourceRequest::finished, this, &MixerAvatar::fstRequestComplete);
        fstRequest->send();
    } else {
        qCDebug(avatars) << "Couldn't create FST request for" << avatarURL;
    }

}

void MixerAvatar::fstRequestComplete() {
    ResourceRequest* fstRequest = static_cast<ResourceRequest*>(QObject::sender());
    QMutexLocker certifyLocker(&_avatarCertifyLock);
    if (fstRequest == _avatarRequest) {
        auto result = fstRequest->getResult();
        if (result != ResourceRequest::Success) {
            qCDebug(avatars) << "FST request for" << fstRequest->getUrl() << "failed:" << result;
        } else {
            _avatarFSTContents = fstRequest->getData();
            _avatarFSTValid = true;
        }
        _avatarRequest->deleteLater();
        _avatarRequest = nullptr;
     } else {
        qCDebug(avatars) << "Incorrect request for" << getDisplayName();
    }
}
