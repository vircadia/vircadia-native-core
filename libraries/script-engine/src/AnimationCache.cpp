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
#include <QScriptEngine>
#include <QThreadPool>

#include "AnimationCache.h"

static int animationPointerMetaTypeId = qRegisterMetaType<AnimationPointer>();

AnimationCache::AnimationCache(QObject* parent) :
    ResourceCache(parent) {
}

AnimationPointer AnimationCache::getAnimation(const QUrl& url) {
    if (QThread::currentThread() != thread()) {
        AnimationPointer result;
        QMetaObject::invokeMethod(this, "getAnimation", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(AnimationPointer, result), Q_ARG(const QUrl&, url));
        return result;
    }
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

QStringList Animation::getJointNames() const {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        QMetaObject::invokeMethod(const_cast<Animation*>(this), "getJointNames", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QStringList, result));
        return result;
    }
    QStringList names;
    foreach (const FBXJoint& joint, _geometry.joints) {
        names.append(joint.name);
    }
    return names;
}

QVector<FBXAnimationFrame> Animation::getFrames() const {
    if (QThread::currentThread() != thread()) {
        QVector<FBXAnimationFrame> result;
        QMetaObject::invokeMethod(const_cast<Animation*>(this), "getFrames", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVector<FBXAnimationFrame>, result));
        return result;
    }
    return _geometry.animationFrames;
}

void Animation::setGeometry(const FBXGeometry& geometry) {
    _geometry = geometry;
    finishedLoading(true);
}

void Animation::downloadFinished(QNetworkReply* reply) {
    // send the reader off to the thread pool
    QThreadPool::globalInstance()->start(new AnimationReader(_self, reply));
}

QStringList AnimationObject::getJointNames() const {
    return qscriptvalue_cast<AnimationPointer>(thisObject())->getJointNames();
}

QVector<FBXAnimationFrame> AnimationObject::getFrames() const {
    return qscriptvalue_cast<AnimationPointer>(thisObject())->getFrames();
}

QVector<glm::quat> AnimationFrameObject::getRotations() const {
    return qscriptvalue_cast<FBXAnimationFrame>(thisObject()).rotations;
}

void registerAnimationTypes(QScriptEngine* engine) {
    qScriptRegisterSequenceMetaType<QVector<FBXAnimationFrame> >(engine);
    engine->setDefaultPrototype(qMetaTypeId<FBXAnimationFrame>(), engine->newQObject(
        new AnimationFrameObject(), QScriptEngine::ScriptOwnership));
    engine->setDefaultPrototype(qMetaTypeId<AnimationPointer>(), engine->newQObject(
        new AnimationObject(), QScriptEngine::ScriptOwnership));
}
