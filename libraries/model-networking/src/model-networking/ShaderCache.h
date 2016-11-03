//
//  Created by Bradley Austin Davis on 2015/05/26
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_ShaderCache_h
#define hifi_ShaderCache_h

#include <ResourceCache.h>

class NetworkShader : public Resource {
public:
    NetworkShader(const QUrl& url);
    virtual void downloadFinished(const QByteArray& data) override;

    QString _source;
};

using NetworkShaderPointer = QSharedPointer<NetworkShader>;

class ShaderCache : public ResourceCache {
public:
    static ShaderCache& instance();

    NetworkShaderPointer getShader(const QUrl& url);

protected:
    virtual QSharedPointer<Resource> createResource(const QUrl& url, const QSharedPointer<Resource>& fallback,
        const void* extra) override;
};

#endif
