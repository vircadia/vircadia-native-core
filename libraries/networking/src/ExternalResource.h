//
//  ExternalResource.h
//
//  Created by Dale Glass on 6 Sep 2020
//  Copyright 2020 Vircadia contributors.
//
//  Flexible management for external resources (eg, on S3)
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#ifndef vircadia_ExternalResource_h
#define vircadia_ExternalResource_h

#include <QObject>
#include <QUrl>
#include <QMap>

#include <mutex>

/**
 * Flexible management for external resources
 *
 * With the death of the original High Fidelity and the transition of the project to a community-managed
 * one, it became necessary to deal with that the various assets that used to be located under the
 * highfidelity.com domain will disappear, and there won't be a fixed place for them afterwards. Data
 * hosted by community members may not remain forever, reorganization may be necessary, people may want
 * to run mirrors, and some might want to run without external internet access at all.
 *
 * This class makes it possible to deal with this in a more flexible manner: rather than hardcoding URLs
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

    /**
     * Bucket from which to retrieve the resource
     *
     * The original High Fidelity used the Public, Content and MPAssets buckets. The intention is that the
     * community-run versions of these will keep the original data and structure, and any new additions
     * will be done to the Assets bucket instead. This should ease the transition and ensure a clean
     * separation.
     */
    enum class Bucket
    {
        /** Assets that used to be in the hifi-public S3 bucket */
        HF_Public,

        /** Assets that used to be in the hifi-content S3 bucket */
        HF_Content,

        /** Assets that used to be in the mpassets S3 bucket (hifi marketplace) */
        HF_Marketplace,

        /** Vircadia assets */
        Assets
    };

    Q_ENUM(Bucket)

    /**
     * Returns the location of a resource as a QUrl
     *
     * Returns the location of the resource \p relative_path in bucket \p bucket
     *
     * @note The resulting path will be sanitized by condensing multiple instances of '/' to one.
     * This is done for easier usage with Amazon S3 and compatible systems.
     *
     * @param bucket The bucket in which the resource is found
     * @param relative_path The path of the resource within the bucket
     * @returns The resulting URL as a QUrl
     */
    QUrl getQUrl(Bucket bucket, const QUrl& relative_path);

    /**
     * Returns the location of a resource as a QUrl
     *
     * Returns the location of the resource \p relative_path in bucket \p bucket
     *
     * @note The resulting path will be sanitized by condensing multiple instances of '/' to one.
     * This is done for easier usage with Amazon S3 and compatible systems.
     *
     * @param bucket The bucket in which the resource is found
     * @param relative_path The path of the resource within the bucket
     * @returns The resulting URL as a QUrl
     */
    QUrl getQUrl(Bucket bucket, QString path) { return getQUrl(bucket, QUrl(path)); }

    /**
     * Returns the location of a resource as a QString
     *
     * Returns the location of the resource \p relative_path in bucket \p bucket
     *
     * @note The resulting path will be sanitized by condensing multiple instances of '/' to one.
     * This is done for easier usage with Amazon S3 and compatible systems.
     *
     * @param bucket The bucket in which the resource is found
     * @param relative_path The path of the resource within the bucket
     * @returns The resulting URL as a QString
     */
    QString getUrl(Bucket bucket, const QUrl& relative_path) {
        return ExternalResource::getQUrl(bucket, relative_path).toString();
    };

    /**
     * Returns the location of a resource as a QString
     *
     * Returns the location of the resource \p relative_path in bucket \p bucket
     *
     * @note The resulting path will be sanitized by condensing multiple instances of '/' to one.
     * This is done for easier usage with Amazon S3 and compatible systems.
     *
     * @param bucket The bucket in which the resource is found
     * @param relative_path The path of the resource within the bucket
     * @returns The resulting URL as a QString
     */
    Q_INVOKABLE QString getUrl(Bucket bucket, const QString& relative_path) {
        return ExternalResource::getQUrl(bucket, QUrl(relative_path)).toString();
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

    QMap<Bucket, QUrl> _bucketBases{
        { Bucket::HF_Public, QUrl("https://public.vircadia.com") },
        { Bucket::HF_Content, QUrl("https://content.vircadia.com") },
        { Bucket::Assets, QUrl("https://assets.vircadia.com") },
        { Bucket::HF_Marketplace, QUrl("https://mpassets.vircadia.com") },
    };
};

#endif