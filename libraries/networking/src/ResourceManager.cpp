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

#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QThread>

#include <SharedUtil.h>


#include "AssetResourceRequest.h"
#include "FileResourceRequest.h"
#include "HTTPResourceRequest.h"
#include "NetworkAccessManager.h"
#include "NetworkLogging.h"

QThread ResourceManager::_thread;
ResourceManager::PrefixMap ResourceManager::_prefixMap;
QMutex ResourceManager::_prefixMapLock;


void ResourceManager::setUrlPrefixOverride(const QString& prefix, const QString& replacement) {
    QMutexLocker locker(&_prefixMapLock);
    if (replacement.isEmpty()) {
        _prefixMap.erase(prefix);
    } else {
        _prefixMap[prefix] = replacement;
    }
}

QString ResourceManager::normalizeURL(const QString& urlString) {
    QString result = urlString;
    PrefixMap copy;

    {
        QMutexLocker locker(&_prefixMapLock);
        copy = _prefixMap;
    }

    foreach(const auto& entry, copy) {
        const auto& prefix = entry.first;
        const auto& replacement = entry.second;
        if (result.startsWith(prefix)) {
            qCDebug(networking) << "Replacing " << prefix << " with " << replacement;
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

void ResourceManager::init() {
    _thread.setObjectName("Resource Manager Thread");

    auto assetClient = DependencyManager::set<AssetClient>();
    assetClient->moveToThread(&_thread);
    QObject::connect(&_thread, &QThread::started, assetClient.data(), &AssetClient::init);

    _thread.start();
}

void ResourceManager::cleanup() {
    // cleanup the AssetClient thread
    DependencyManager::destroy<AssetClient>();
    _thread.quit();
    _thread.wait();
}

ResourceRequest* ResourceManager::createResourceRequest(QObject* parent, const QUrl& url) {
    auto normalizedURL = normalizeURL(url);
    auto scheme = normalizedURL.scheme();

    ResourceRequest* request = nullptr;

    if (scheme == URL_SCHEME_FILE) {
        request = new FileResourceRequest(normalizedURL);
    } else if (scheme == URL_SCHEME_HTTP || scheme == URL_SCHEME_HTTPS || scheme == URL_SCHEME_FTP) {
        request = new HTTPResourceRequest(normalizedURL);
    } else if (scheme == URL_SCHEME_ATP) {
        request = new AssetResourceRequest(normalizedURL);
    } else {
        qCDebug(networking) << "Unknown scheme (" << scheme << ") for URL: " << url.url();
        return nullptr;
    }
    Q_ASSERT(request);

    if (parent) {
        QObject::connect(parent, &QObject::destroyed, request, &QObject::deleteLater);
    }
    request->moveToThread(&_thread);
    return request;
}
