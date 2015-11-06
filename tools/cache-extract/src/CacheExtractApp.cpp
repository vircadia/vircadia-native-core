//
//  CacheExtractApp.cpp
//  tools/cache-extract/src
//
//  Created by Anthony Thibault on 11/6/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CacheExtractApp.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QtNetwork/QAbstractNetworkCache>

// extracted from qnetworkdiskcache.cpp
#define CACHE_VERSION 8
enum {
   CacheMagic = 0xe8,
   CurrentCacheVersion = CACHE_VERSION
};
#define DATA_DIR QLatin1String("data")

CacheExtractApp::CacheExtractApp(int& argc, char** argv) :
    QCoreApplication(argc, argv)
{
    QString myDataLoc = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    int lastSlash = myDataLoc.lastIndexOf(QDir::separator());
    QString cachePath = myDataLoc.leftRef(lastSlash).toString() + QDir::separator() +
        "High Fidelity" + QDir::separator() + "Interface" + QDir::separator() +
        DATA_DIR + QString::number(CACHE_VERSION) + QLatin1Char('/');

    QString outputPath = myDataLoc.leftRef(lastSlash).toString() + QDir::separator() +
        "High Fidelity" + QDir::separator() + "Interface" + QDir::separator() + "extracted";

    qDebug() << "Searching cachePath = " << cachePath << "...";

    // build list of files
    QList<QString> fileList;
    QDir dir(cachePath);
    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.isDir()) {
            QDir subDir(fileInfo.filePath());
            subDir.setFilter(QDir::Files);
            QFileInfoList subList = subDir.entryInfoList();
            for (int j = 0; j < subList.size(); ++j) {
                fileList << subList.at(j).filePath();
            }
        }
    }

    // dump each cache file into the outputPath
    for (int i = 0; i < fileList.size(); ++i) {
        QByteArray contents;
        MyMetaData metaData;
        if (extractFile(fileList.at(i), metaData, contents)) {
            QString outFileName = outputPath + metaData.url.path();
            int lastSlash = outFileName.lastIndexOf(QDir::separator());
            QString outDirName = outFileName.leftRef(lastSlash).toString();
            QDir dir(outputPath);
            dir.mkpath(outDirName);
            QFile out(outFileName);
            if (out.open(QIODevice::WriteOnly)) {
                out.write(contents);
                out.close();
                qDebug().noquote() << metaData.url.toDisplayString();
            }
        } else {
            qCritical() << "Error extracting = " << fileList.at(i);
        }
    }

    QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
}

bool CacheExtractApp::extractFile(const QString& filePath, MyMetaData& metaData, QByteArray& data) const {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "error opening " << filePath;
        return false;
    }
    QDataStream in(&f);
    // from qnetworkdiskcache.cpp QCacheItem::read()
    qint32 marker, version, streamVersion;
    in >> marker;
    if (marker != CacheMagic) {
        return false;
    }
    in >> version;
    if (version != CurrentCacheVersion) {
        return false;
    }
    in >> streamVersion;
    if (streamVersion > in.version())
        return false;
    in.setVersion(streamVersion);

    bool compressed;
    in >> metaData;
    in >> compressed;
    if (compressed) {
        QByteArray compressedData;
        in >> compressedData;
        QBuffer buffer;
        buffer.setData(qUncompress(compressedData));
        buffer.open(QBuffer::ReadOnly);
        data = buffer.readAll();
    } else {
        data = f.readAll();
    }
    return true;
}

QDataStream &operator>>(QDataStream& in, MyMetaData& metaData) {
    in >> metaData.url;
    in >> metaData.expirationDate;
    in >> metaData.lastModified;
    in >> metaData.saveToDisk;
    in >> metaData.attributes;
    in >> metaData.rawHeaders;
}
