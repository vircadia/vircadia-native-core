//
//  ResourceManager.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ResourceManager_h
#define hifi_ResourceManager_h

#include <functional>

#include <QtCore/QMutex>

#include "ResourceRequest.h"

const QString URL_SCHEME_FILE = "file";
const QString URL_SCHEME_HTTP = "http";
const QString URL_SCHEME_HTTPS = "https";
const QString URL_SCHEME_FTP = "ftp";
const QString URL_SCHEME_ATP = "atp";

class ResourceManager {
public:
    static void setUrlPrefixOverride(const QString& prefix, const QString& replacement);
    static QString normalizeURL(const QString& urlString);
    static QUrl normalizeURL(const QUrl& url);

    static ResourceRequest* createResourceRequest(QObject* parent, const QUrl& url);

    static void init();
    static void cleanup();

private:
    static QThread _thread;

    using PrefixMap = std::map<QString, QString>;

    static PrefixMap _prefixMap;
    static QMutex _prefixMapLock;
};

#endif
