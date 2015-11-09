//
//  CacheExtractApp.h
//  tools/cache-extract/src
//
//  Created by Anthony Thibault on 11/6/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CacheExtractApp_h
#define hifi_CacheExtractApp_h

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtNetwork/QNetworkDiskCache>
#include <QBuffer>
#include <QDateTime>
#include <QtNetwork/QNetworkRequest>

// copy of QNetworkCacheMetaData
class MyMetaData {
public:
    using RawHeader = QPair<QByteArray, QByteArray>;
    using RawHeaderList = QList<RawHeader>;
    using AttributesMap = QHash<qint32, QVariant>;

    QUrl url;
    QDateTime expirationDate;
    QDateTime lastModified;
    bool saveToDisk;
    AttributesMap attributes;
    RawHeaderList rawHeaders;
};

QDataStream &operator>>(QDataStream& in, MyMetaData& metaData);

class CacheExtractApp : public QCoreApplication {
    Q_OBJECT
public:
    CacheExtractApp(int& argc, char** argv);

    bool extractFile(const QString& filePath, MyMetaData& metaData, QByteArray& data) const;
};

#endif // hifi_CacheExtractApp_h
