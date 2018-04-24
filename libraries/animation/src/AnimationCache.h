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

/**jsdoc
 * API to manage Animation Cache resources
 * @namespace AnimationCache
 */

class Animation;

typedef QSharedPointer<Animation> AnimationPointer;

/// Scriptable interface for FBX animation loading.
class AnimationCache : public ResourceCache, public Dependency  {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    // Copied over from ResourceCache (see ResourceCache.h for reason)

    /**jsdoc
     * @namespace AnimationCache
     * @property numTotal {number} total number of total resources
     * @property numCached {number} total number of cached resource
     * @property sizeTotal {number} size in bytes of all resources
     * @property sizeCached {number} size in bytes of all cached resources
     */

    /**jsdoc
     * Returns the total number of resources
     * @function AnimationCache.getNumTotalResources
     * @returns {number}
     */

    /**jsdoc
     * Returns the total size in bytes of all resources
     * @function AnimationCache.getSizeTotalResources
     * @returns {number}
     */

    /**jsdoc
     * Returns the total number of cached resources
     * @function AnimationCache.getNumCachedResources
     * @returns {number}
     */

    /**jsdoc
     * Returns the total size in bytes of cached resources
     * @function AnimationCache.getSizeCachedResources
     * @returns {number}
     */

    /**jsdoc
     * Returns list of all resource urls
     * @function AnimationCache.getResourceList
     * @returns {string[]}
     */

    /**jsdoc
     * Asynchronously loads a resource from the spedified URL and returns it.
     * @param url {string} url of resource to load
     * @param fallback {string} fallback URL if load of the desired url fails
     * @function AnimationCache.getResource
     * @returns {Resource}
     */
    
    /**jsdoc
     * Prefetches a resource.
     * @param url {string} url of resource to load
     * @function AnimationCache.prefetch
     * @returns {Resource}
     */

    /**jsdoc
     * @param {number} deltaSize
     * @function AnimationCache.updateTotalSize
     * @returns {Resource}
     */

    /**jsdoc
     * @function AnimationCache.dirty
     * @returns {Signal} 
     */

    /**jsdoc
     * Returns animation resource for particular animation
     * @function AnimationCache.getAnimation
     * @param url {string} url to load
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
