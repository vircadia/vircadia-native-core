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

#include "ExternalResource.h"
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(external_resource, "vircadia.networking.external_resource")

ExternalResource::ExternalResource(QObject* parent) : QObject(parent) {
}

ExternalResource* ExternalResource::getInstance() {
    static ExternalResource instance;
    return &instance;
}

QUrl ExternalResource::getQUrl(Bucket bucket, const QUrl& relative_path) {
    qCDebug(external_resource) << "Requested URL for bucket " << bucket << ", path " << relative_path;

    if (!_bucketBases.contains(bucket)) {
        qCCritical(external_resource) << "External resource " << relative_path << " was requested from unrecognized bucket "
                                      << bucket;
        return QUrl();
    }

    if (!relative_path.isValid()) {
        qCCritical(external_resource) << "External resource " << relative_path << " was requested from bucket " << bucket
                                      << " with an invalid path. Error: " << relative_path.errorString();
        return QUrl();
    }

    if (!relative_path.isRelative()) {
        qCWarning(external_resource) << "External resource " << relative_path << " was requested from bucket " << bucket
                                     << " without using a relative path, returning as-is.";
        return relative_path;
    }

    std::lock_guard<std::mutex> guard(_bucketMutex);
    QUrl base = _bucketBases[bucket];
    QUrl merged = base.resolved(relative_path);

    qCDebug(external_resource) << "External resource resolved to " << merged;

    return merged;
}

QString ExternalResource::getBase(Bucket bucket) {
    std::lock_guard<std::mutex> guard(_bucketMutex);
    return _bucketBases.value(bucket).toString();
};

void ExternalResource::setBase(Bucket bucket, const QString& url) {
    QUrl new_url(url);

    if (!new_url.isValid() || new_url.isRelative()) {
        qCCritical(external_resource) << "Attempted to set bucket " << bucket << " to invalid URL " << url;
        return;
    }

    if (!_bucketBases.contains(bucket)) {
        qCritical(external_resource) << "Invalid bucket " << bucket;
        return;
    }

    qCDebug(external_resource) << "Setting base URL for " << bucket << " to " << new_url;

    std::lock_guard<std::mutex> guard(_bucketMutex);
    _bucketBases[bucket] = new_url;
}
