//
//  AnimationCache.cpp
//  libraries/script-engine/src/
//
//  Created by Andrzej Kapolka on 4/14/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QRunnable>
#include <QThreadPool>

#include "AnimationCache.h"

QSharedPointer<Animation> AnimationCache::getAnimation(const QUrl& url) {
    return getResource(url).staticCast<Animation>();
}

QSharedPointer<Resource> AnimationCache::createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
        bool delayLoad, const void* extra) {
    return QSharedPointer<Resource>(new Animation(url), &Resource::allReferencesCleared);
}

Animation::Animation(const QUrl& url) :
    Resource(url) {
}

class AnimationReader : public QRunnable {
public:

    AnimationReader(const QWeakPointer<Resource>& animation, QNetworkReply* reply);
    
    virtual void run();

private:
    
    QWeakPointer<Resource> _animation;
    QNetworkReply* _reply;
};

AnimationReader::AnimationReader(const QWeakPointer<Resource>& animation, QNetworkReply* reply) :
    _animation(animation),
    _reply(reply) {
}

void AnimationReader::run() {
    QSharedPointer<Resource> animation = _animation.toStrongRef();
    if (!animation.isNull()) {
        QMetaObject::invokeMethod(animation.data(), "setGeometry",
            Q_ARG(const FBXGeometry&, readFBX(_reply->readAll(), QVariantHash())));
    }
    _reply->deleteLater();
}

void Animation::setGeometry(const FBXGeometry& geometry) {
    _geometry = geometry;
    finishedLoading(true);
}

void Animation::downloadFinished(QNetworkReply* reply) {
    // send the reader off to the thread pool
    QThreadPool::globalInstance()->start(new AnimationReader(_self, reply));
}
