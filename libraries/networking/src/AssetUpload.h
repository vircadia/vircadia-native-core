//
//  AssetUpload.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2015-08-26.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_AssetUpload_h
#define hifi_AssetUpload_h

#include <QtCore/QObject>

#include <cstdint>

// You should be able to upload an asset from any thread, and handle the responses in a safe way
// on your own thread. Everything should happen on AssetClient's thread, the caller should
// receive events by connecting to signals on an object that lives on AssetClient's threads.

class AssetUpload : public QObject {
    Q_OBJECT
public:
    
    enum Error {
        NoError = 0,
        NetworkError,
        Timeout,
        TooLarge,
        PermissionDenied,
        FileOpenError,
        ServerFileError
    };
    
    static const QString PERMISSION_DENIED_ERROR;
    
    AssetUpload(const QString& filename);
    AssetUpload(const QByteArray& data);
    
    Q_INVOKABLE void start();
    
    const QString& getFilename() const { return _filename; }
    const Error& getError() const { return _error; }
    QString getErrorString() const;
    
signals:
    void finished(AssetUpload* upload, const QString& hash);
    void progress(uint64_t totalReceived, uint64_t total);
    
private:
    QString _filename;
    QByteArray _data;
    Error _error;
};

#endif // hifi_AssetUpload_h
