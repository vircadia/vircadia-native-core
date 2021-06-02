//
//  ExternalResource.h
//
//  Created by Dale Glass on 6 Sep 2020
//  Copyright 2020 Vircadia contributors.
//
//  Flexible management for external resources (e.g., on S3).
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#ifndef vircadia_ExternalResource_h
#define vircadia_ExternalResource_h

#include <QObject>
#include <QUrl>
#include <QMap>

#include <mutex>

#include "NetworkingConstants.h"

/**
 * Flexible management for external resources
 *
 * With the death of the original High Fidelity and the transition of the project to a community-managed
 * one, it became necessary to deal with that the various assets that used to be located under the
 * highfidelity.com domain, and there won't be a fixed place for them afterwards. Data
 * hosted by community members may not remain forever, reorganization may be necessary, people may want
 * to run mirrors, and some might want to run without external Internet access at all.
 *
 * This class makes it possible to deal with this in a more flexible manner: rather than hard-coding URLs
 * all over the codebase, now it's possible to easily change where all those things are downloaded from.
 *
 * The term 'bucket' refers to the buckets used on Amazon S3, but there's no requirement for S3 or anything
 * similar to be used. The term should just be taken as referring to the name of a data set.
 */
class ExternalResource : public QObject {
    Q_OBJECT

public:
    static ExternalResource* getInstance();
    ~ExternalResource(){};

    /*@jsdoc
     * <p>An external resource bucket.</p>
     * <p>The original High Fidelity used "Public", "Content", and "MPAssets" Amazon S3 buckets. The intention is that the
     * community-run versions of these will keep the original data and structure, and any new additions will be made to 
     * Vircadia's "Assets" bucket. This should ease the transition from High Fidelity and ensure a clean separation.</p>
     * @typedef {object} Script.ResourceBuckets
     * @property {Script.ResourceBucket} Assets - Vircadia assets.
     * @property {Script.ResourceBucket} HF_Public - Assets that used to be in High Fidelity's <code>hifi-public</code> Amazon 
     *     S3 bucket.
     * @property {Script.ResourceBucket} HF_Content - Assets that used to be in High Fidelity's <code>hifi-content</code> Amazon
     *     S3 bucket.
     * @property {Script.ResourceBucket} HF_Marketplace - Assets that used to be in the High Fidelity's <code>mpassets</code>
     *     Amazon S3 bucket. (High Fidelity marketplace.)
     */
    /*@jsdoc
     * <p>An external resource bucket.</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Name</th><th>Description</th>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>HF_Public</td><td>Assets that used to be in High Fidelity's <code>hifi-public</code>
     *       Amazon S3 bucket.</td></tr>
     *     <tr><td><code>1</code></td><td>HF_Content</td><td>Assets that used to be in High Fidelity's <code>hifi-content</code>
     *       Amazon S3 bucket.</td></tr>
     *     <tr><td><code>2</code></td><td>HF_Marketplace</td><td>Assets that used to be in the High Fidelity's 
     *       <code>mpassets</code> Amazon S3 bucket. (High Fidelity marketplace.)</td></tr>
     *     <tr><td><code>3</code></td><td>Assets</td><td>Vircadia assets.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} Script.ResourceBucket
     */
    enum class Bucket {
        HF_Public,
        HF_Content,
        HF_Marketplace,
        Assets
    };
    Q_ENUM(Bucket)

    /**
     * Returns the location of a resource as a QUrl
     *
     * Returns the location of the resource \p path in bucket \p bucket
     *
     * @note The resulting path will be sanitized by condensing multiple instances of '/' to one.
     * This is done for easier usage with Amazon S3 and compatible systems.
     *
     * @par It will also convert all paths into relative ones respect to the bucket.
     *
     * @warning This function should only be given paths with a domain name. If given a complete path,
     * it will emit a warning into the log and return the unmodified path it was given.
     *
     * @param bucket The bucket in which the resource is found
     * @param relative_path The path of the resource within the bucket
     * @returns The resulting URL as a QUrl
     */
    QUrl getQUrl(Bucket bucket, QString path);

    QString getUrl(Bucket bucket, QString path) {
       return ExternalResource::getQUrl(bucket, path).toString();
    };

    /**
     * Returns the base path for a bucket
     * 
     * @param bucket Bucket whose path to return
     */
    QString getBase(Bucket bucket);

    /**
     * Sets the base path for a bucket
     *
     * The \p url parameter will be validated, and the action will not be performed
     * if the URL isn't a valid one, or if the bucket wasn't valid.
     *
     * @param bucket Bucket whose path to set
     * @param url Base URL for the bucket.
     * @returns Whether the base was set.
     */
    bool setBase(Bucket bucket, const QString& url);

private:
    ExternalResource(QObject* parent = nullptr);

    std::mutex _bucketMutex;

    QMap<Bucket, QUrl> _bucketBases {
        { Bucket::HF_Public, QUrl(NetworkingConstants::HF_PUBLIC_CDN_URL) },
        { Bucket::HF_Content, QUrl(NetworkingConstants::HF_CONTENT_CDN_URL) },
        { Bucket::HF_Marketplace, QUrl(NetworkingConstants::HF_MPASSETS_CDN_URL) },
        { Bucket::Assets, QUrl(NetworkingConstants::VIRCADIA_CONTENT_CDN_URL) }
    };
};

#endif
