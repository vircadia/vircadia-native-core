//
//  AnimationCache.cpp
//  libraries/animation/src/
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
#include "AnimationLogging.h"

int animationPointerMetaTypeId = qRegisterMetaType<AnimationPointer>();

AnimationCache::AnimationCache(QObject* parent) :
    ResourceCache(parent)
{
    const qint64 ANIMATION_DEFAULT_UNUSED_MAX_SIZE = 50 * BYTES_PER_MEGABYTES;
    setUnusedResourceCacheSize(ANIMATION_DEFAULT_UNUSED_MAX_SIZE);
    setObjectName("AnimationCache");
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
    const void* extra) {
    return QSharedPointer<Resource>(new Animation(url), &Resource::deleter);
}

Animation::Animation(const QUrl& url) : Resource(url) {}

AnimationReader::AnimationReader(const QUrl& url, const QByteArray& data) :
    _url(url),
    _data(data) {
}

void AnimationReader::run() {
    auto originalPriority = QThread::currentThread()->priority();
    if (originalPriority == QThread::InheritPriority) {
        originalPriority = QThread::NormalPriority;
    }
    QThread::currentThread()->setPriority(QThread::LowPriority);
    try {
        if (_data.isEmpty()) {
            throw QString("Reply is NULL ?!");
        }
        QString urlname = _url.path().toLower();
        bool urlValid = true;
        urlValid &= !urlname.isEmpty();
        urlValid &= !_url.path().isEmpty();

        if (urlValid) {
            // Parse the FBX directly from the QNetworkReply
            FBXGeometry::Pointer fbxgeo;
            if (_url.path().toLower().endsWith(".fbx")) {
                fbxgeo.reset(readFBX(_data, QVariantHash(), _url.path()));
            } else {
                QString errorStr("usupported format");
                emit onError(299, errorStr);
            }
            emit onSuccess(fbxgeo);
        } else {
            throw QString("url is invalid");
        }

    } catch (const QString& error) {
        emit onError(299, error);
    }
    QThread::currentThread()->setPriority(originalPriority);
}

bool Animation::isLoaded() const {
    return _loaded && _geometry;
}

QStringList Animation::getJointNames() const {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        QMetaObject::invokeMethod(const_cast<Animation*>(this), "getJointNames", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QStringList, result));
        return result;
    }
    QStringList names;
    foreach (const FBXJoint& joint, _geometry->joints) {
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
    return _geometry->animationFrames;
}

const QVector<FBXAnimationFrame>& Animation::getFramesReference() const {
    return _geometry->animationFrames;
}

void Animation::downloadFinished(const QByteArray& data) {
    // parse the animation/fbx file on a background thread.
    AnimationReader* animationReader = new AnimationReader(_url, data);
    connect(animationReader, SIGNAL(onSuccess(FBXGeometry::Pointer)), SLOT(animationParseSuccess(FBXGeometry::Pointer)));
    connect(animationReader, SIGNAL(onError(int, QString)), SLOT(animationParseError(int, QString)));
    QThreadPool::globalInstance()->start(animationReader);
}

void Animation::animationParseSuccess(FBXGeometry::Pointer geometry) {

    qCDebug(animation) << "Animation parse success" << _url.toDisplayString();

    _geometry = geometry;
    finishedLoading(true);
}

void Animation::animationParseError(int error, QString str) {
    qCCritical(animation) << "Animation failure parsing " << _url.toDisplayString() << "code =" << error << str;
    emit failed(QNetworkReply::UnknownContentError);
    finishedLoading(false);
}

AnimationDetails::AnimationDetails() :
    role(), url(), fps(0.0f), priority(0.0f), loop(false), hold(false), 
    startAutomatically(false), firstFrame(0.0f), lastFrame(0.0f), running(false), currentFrame(0.0f) 
{
}

AnimationDetails::AnimationDetails(QString role, QUrl url, float fps, float priority, bool loop,
    bool hold, bool startAutomatically, float firstFrame, float lastFrame, bool running, float currentFrame) :
    role(role), url(url), fps(fps), priority(priority), loop(loop), hold(hold), 
    startAutomatically(startAutomatically), firstFrame(firstFrame), lastFrame(lastFrame), 
    running(running), currentFrame(currentFrame) 
{ 
}


QScriptValue animationDetailsToScriptValue(QScriptEngine* engine, const AnimationDetails& details) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("role", details.role);
    obj.setProperty("url", details.url.toString());
    obj.setProperty("fps", details.fps);
    obj.setProperty("priority", details.priority);
    obj.setProperty("loop", details.loop);
    obj.setProperty("hold", details.hold);
    obj.setProperty("startAutomatically", details.startAutomatically);
    obj.setProperty("firstFrame", details.firstFrame);
    obj.setProperty("lastFrame", details.lastFrame);
    obj.setProperty("running", details.running);
    obj.setProperty("currentFrame", details.currentFrame);
    return obj;
}

void animationDetailsFromScriptValue(const QScriptValue& object, AnimationDetails& details) {
    // nothing for now...
}

