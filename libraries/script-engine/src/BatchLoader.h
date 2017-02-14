//
//  BatchLoader.h
//  libraries/script-engine/src
//
//  Created by Ryan Huffman on 01/22/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BatchLoader_h
#define hifi_BatchLoader_h

#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QUrl>

#include "ScriptCache.h"

#include <mutex>

class ScriptCacheSignalProxy : public QObject {
    Q_OBJECT
public:
    void receivedContent(const QString& url, const QString& contents, bool isURL, bool success, const QString& status);

signals:
    void contentAvailable(const QString& url, const QString& contents, bool isURL, bool success, const QString& status);
};

class BatchLoader : public QObject {
    Q_OBJECT
public:
    BatchLoader(const QList<QUrl>& urls);

    void start(int maxRetries = ScriptRequest::MAX_RETRIES);
    bool isFinished() const { return _finished; };

signals:
    void finished(const QMap<QUrl, QString>& data, const QMap<QUrl, QString>& status);

private:
    void checkFinished();

    bool _started;
    bool _finished;
    QSet<QUrl> _urls;
    QMap<QUrl, QString> _data;
    QMap<QUrl, QString> _status;
};

#endif // hifi_BatchLoader_h
