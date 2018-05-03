//
//  AnimationCache.h
//  libraries/animation/src/
//
//  Created by Andrzej Kapolka on 4/14/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimationCache_h
#define hifi_AnimationCache_h

#include <QtCore/QRunnable>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

#include <DependencyManager.h>
#include <FBXReader.h>
#include <ResourceCache.h>

class Animation;

typedef QSharedPointer<Animation> AnimationPointer;

/// Scriptable interface for FBX animation loading.
class AnimationCache : public ResourceCache, public Dependency  {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:

    // Properties are copied over from ResourceCache (see ResourceCache.h for reason).

    /**jsdoc
     * API to manage animation cache resources.
     * @namespace AnimationCache
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-assignment-client
     *
     * @property {number} numTotal - Total number of total resources. <em>Read-only.</em>
     * @property {number} numCached - Total number of cached resource. <em>Read-only.</em>
     * @property {number} sizeTotal - Size in bytes of all resources. <em>Read-only.</em>
     * @property {number} sizeCached - Size in bytes of all cached resources. <em>Read-only.</em>
     */

    // Functions are copied over from ResourceCache (see ResourceCache.h for reason).

    /**jsdoc
     * Get the list of all resource URLs.
     * @function AnimationCache.getResourceList
     * @returns {string[]}
     */

    /**jsdoc
     * @function AnimationCache.dirty
     * @returns {Signal}
     */

    /**jsdoc
     * @function AnimationCache.updateTotalSize
     * @param {number} deltaSize
     */

    /**jsdoc
     * Prefetches a resource.
     * @function AnimationCache.prefetch
     * @param {string} url - URL of the resource to prefetch.
     * @param {object} [extra=null]
     * @returns {Resource}
     */

    /**jsdoc
     * Asynchronously loads a resource from the specified URL and returns it.
     * @function AnimationCache.getResource
     * @param {string} url - URL of the resource to load.
     * @param {string} [fallback=""] - Fallback URL if load of the desired URL fails.
     * @param {} [extra=null]
     * @returns {Resource}
     */


    /**jsdoc
     * Returns animation resource for particular animation.
     * @function AnimationCache.getAnimation
     * @param {string} url - URL to load.
     * @returns {Resource} animation
     */
    Q_INVOKABLE AnimationPointer getAnimation(const QString& url) { return getAnimation(QUrl(url)); }
    Q_INVOKABLE AnimationPointer getAnimation(const QUrl& url);

protected:

    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
        const void* extra) override;
private:
    explicit AnimationCache(QObject* parent = NULL);
    virtual ~AnimationCache() { }

};

Q_DECLARE_METATYPE(AnimationPointer)

/// An animation loaded from the network.
class Animation : public Resource {
    Q_OBJECT

public:

    explicit Animation(const QUrl& url);

    QString getType() const override { return "Animation"; }

    const FBXGeometry& getGeometry() const { return *_geometry; }

    virtual bool isLoaded() const override;

    
    Q_INVOKABLE QStringList getJointNames() const;
    
    Q_INVOKABLE QVector<FBXAnimationFrame> getFrames() const;

    const QVector<FBXAnimationFrame>& getFramesReference() const;
    
protected:
    virtual void downloadFinished(const QByteArray& data) override;

protected slots:
    void animationParseSuccess(FBXGeometry::Pointer geometry);
    void animationParseError(int error, QString str);

private:
    
    FBXGeometry::Pointer _geometry;
};

/// Reads geometry in a worker thread.
class AnimationReader : public QObject, public QRunnable {
    Q_OBJECT

public:
    AnimationReader(const QUrl& url, const QByteArray& data);
    virtual void run() override;

signals:
    void onSuccess(FBXGeometry::Pointer geometry);
    void onError(int error, QString str);

private:
    QUrl _url;
    QByteArray _data;
};


#endif // hifi_AnimationCache_h
