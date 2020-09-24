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

QUrl ExternalResource::getQUrl(Bucket bucket, QString path) {
    qCDebug(external_resource) << "Requested URL for bucket " << bucket << ", path " << path;

    // Whitespace could interfere with the next step
    path = path.trimmed();

    // Remove all initial slashes. This is because if QUrl is fed //foo/bar,
    // it will treat //foo as a domain name. This also ensures the URL is always treated as
    // relative to any path specified in the bucket.
    while (path.startsWith('/')) {
        path = path.remove(0, 1);
    }

    // De-duplicate URL separators, since S3 doesn't actually have subdirectories, and treats a '/' as a part
    // of the filename. This means that "/dir/file.txt" and "/dir//file.txt" are not equivalent on S3.
    //
    // We feed it through a QUrl to ensure we're only working on the path component.
    QUrl pathQUrl(path);

    QString tempPath = pathQUrl.path();
    while (tempPath.contains("//")) {
        tempPath = tempPath.replace("//", "/");
    }
    pathQUrl.setPath(tempPath);

    if (!pathQUrl.isValid()) {
        qCCritical(external_resource) << "External resource " << pathQUrl << " was requested from bucket " << bucket
                                      << " with an invalid path. Error: " << pathQUrl.errorString();
        return QUrl();
    }

    if (!pathQUrl.isRelative()) {
        qCWarning(external_resource) << "External resource " << pathQUrl << " was requested from bucket " << bucket
                                     << " without using a relative path, returning as-is.";
        return pathQUrl;
    }

    QUrl base;
    {
        std::lock_guard<std::mutex> guard(_bucketMutex);

        if (!_bucketBases.contains(bucket)) {
            qCCritical(external_resource) << "External resource " << path << " was requested from unrecognized bucket "
                                          << bucket;
            return QUrl();
        }

        base = _bucketBases[bucket];
    }

    QUrl merged = base.resolved(pathQUrl).adjusted(QUrl::NormalizePathSegments);

    if (merged.isValid()) {
        qCDebug(external_resource) << "External resource resolved to " << merged;
    } else {
        qCCritical(external_resource) << "External resource resolved to invalid URL " << merged << "; Error "
                                      << merged.errorString() << "; base = " << base << "; path = " << path
                                      << "; filtered path = " << pathQUrl;
    }

    return merged;
}

QString ExternalResource::getBase(Bucket bucket) {
    std::lock_guard<std::mutex> guard(_bucketMutex);
    return _bucketBases.value(bucket).toString();
};

bool ExternalResource::setBase(Bucket bucket, const QString& url) {
    QUrl newURL(url);

    if (!newURL.isValid() || newURL.isRelative()) {
        qCCritical(external_resource) << "Attempted to set bucket " << bucket << " to invalid URL " << url;
        return false;
    }

    if (!_bucketBases.contains(bucket)) {
        qCritical(external_resource) << "Invalid bucket " << bucket;
        return false;
    }

    qCDebug(external_resource) << "Setting base URL for " << bucket << " to " << newURL;

    std::lock_guard<std::mutex> guard(_bucketMutex);
    _bucketBases[bucket] = newURL;
    return true;
}
