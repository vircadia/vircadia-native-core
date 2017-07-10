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

#include <QObject>
#include <QtCore/QMutex>
#include <QThread>

#include <DependencyManager.h>

#include "ResourceRequest.h"

const QString URL_SCHEME_FILE = "file";
const QString URL_SCHEME_HTTP = "http";
const QString URL_SCHEME_HTTPS = "https";
const QString URL_SCHEME_FTP = "ftp";
const QString URL_SCHEME_ATP = "atp";

class ResourceManager: public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    ResourceManager();

    void setUrlPrefixOverride(const QString& prefix, const QString& replacement);
    QString normalizeURL(const QString& urlString);
    QUrl normalizeURL(const QUrl& url);

    ResourceRequest* createResourceRequest(QObject* parent, const QUrl& url);

    void init();
    void cleanup();

    // Blocking call to check if a resource exists. This function uses a QEventLoop internally
    // to return to the calling thread so that events can still be processed.
    bool resourceExists(const QUrl& url);

    // adjust where we persist the cache
    void setCacheDir(const QString& cacheDir);

private:
    QThread _thread;

    using PrefixMap = std::map<QString, QString>;

    PrefixMap _prefixMap;
    QMutex _prefixMapLock;

};

#endif
