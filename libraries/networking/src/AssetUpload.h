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

// You should be able to upload an asset from any thread, and handle the responses in a safe way
// on your own thread. Everything should happen on AssetClient's thread, the caller should
// receive events by connecting to signals on an object that lives on AssetClient's threads.

class AssetUpload : public QObject {
    Q_OBJECT
public:
    
    enum Result {
        Success = 0,
        Timeout,
        TooLarge,
        PermissionDenied,
        ErrorLoadingFile
    };
    
    AssetUpload(QObject* parent, const QString& filename);
    
    Q_INVOKABLE void start();
    
    const QString& getFilename() const { return _filename; }
    const QString& getExtension() const  { return _extension; }
    const Result& getResult() const { return _result; }
    
signals:
    void finished(AssetUpload* upload, const QString& hash);
    void progress(uint64_t totalReceived, uint64_t total);
    
private:
    QString _filename;
    QString _extension;
    Result _result;
};

#endif // hifi_AssetUpload_h
