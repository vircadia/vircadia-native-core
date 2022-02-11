//
//  Created by Bradley Austin Davis on 2015/05/26
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ShaderCache.h"

NetworkShader::NetworkShader(const QUrl& url) :
    Resource(url) {}

void NetworkShader::downloadFinished(const QByteArray& data) {
    _source = QString::fromUtf8(data);
    finishedLoading(true);
}

ShaderCache& ShaderCache::instance() {
    static ShaderCache _instance;
    return _instance;
}

NetworkShaderPointer ShaderCache::getShader(const QUrl& url) {
    return ResourceCache::getResource(url).staticCast<NetworkShader>();
}

QSharedPointer<Resource> ShaderCache::createResource(const QUrl& url) {
    return QSharedPointer<NetworkShader>(new NetworkShader(url), &Resource::deleter);
}

QSharedPointer<Resource> ShaderCache::createResourceCopy(const QSharedPointer<Resource>& resource) {
    return QSharedPointer<NetworkShader>(new NetworkShader(*resource.staticCast<NetworkShader>()), &Resource::deleter);
}
