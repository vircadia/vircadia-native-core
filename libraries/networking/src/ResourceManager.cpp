//
//  ResourceManager.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ResourceManager.h"

#include "AssetResourceRequest.h"
#include "FileResourceRequest.h"
#include "HTTPResourceRequest.h"

#include <SharedUtil.h>

ResourceManager::PrefixMap ResourceManager::_prefixMap;
QMutex ResourceManager::_prefixMapLock;


void ResourceManager::setUrlPrefixOverride(const QString& prefix, const QString& replacement) {
    QMutexLocker locker(&_prefixMapLock);
    _prefixMap[prefix] = replacement;
}

QString ResourceManager::normalizeURL(const QString& urlString) {
    QString result = urlString;
    QMutexLocker locker(&_prefixMapLock);
    foreach(const auto& entry, _prefixMap) {
        const auto& prefix = entry.first;
        const auto& replacement = entry.second;
        if (result.startsWith(prefix)) {
            result.replace(0, prefix.size(), replacement);
        }
    }
    return result;
}

QUrl ResourceManager::normalizeURL(const QUrl& originalUrl) {
    QUrl url = QUrl(normalizeURL(originalUrl.toString()));
    auto scheme = url.scheme();
    if (!(scheme == URL_SCHEME_FILE ||
          scheme == URL_SCHEME_HTTP || scheme == URL_SCHEME_HTTPS || scheme == URL_SCHEME_FTP ||
          scheme == URL_SCHEME_ATP)) {

        // check the degenerative file case: on windows we can often have urls of the form c:/filename
        // this checks for and works around that case.
        QUrl urlWithFileScheme{ URL_SCHEME_FILE + ":///" + url.toString() };
        if (!urlWithFileScheme.toLocalFile().isEmpty()) {
            return urlWithFileScheme;
        }
    }
    return url;
}

ResourceRequest* ResourceManager::createResourceRequest(QObject* parent, const QUrl& url) {
    auto normalizedURL = normalizeURL(url);
    auto scheme = normalizedURL.scheme();
    if (scheme == URL_SCHEME_FILE) {
        return new FileResourceRequest(parent, normalizedURL);
    } else if (scheme == URL_SCHEME_HTTP || scheme == URL_SCHEME_HTTPS || scheme == URL_SCHEME_FTP) {
        return new HTTPResourceRequest(parent, normalizedURL);
    } else if (scheme == URL_SCHEME_ATP) {
        return new AssetResourceRequest(parent, normalizedURL);
    }

    qDebug() << "Unknown scheme (" << scheme << ") for URL: " << url.url();

    return nullptr;
}
