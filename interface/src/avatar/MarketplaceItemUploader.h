//
//  MarketplaceItemUploader.h
//
//
//  Created by Ryan Huffman on 12/10/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_MarketplaceItemUploader_h
#define hifi_MarketplaceItemUploader_h

#include "ProjectFile.h"

#include <QObject>
#include <QUuid>

class QNetworkReply;

class MarketplaceItemUploader : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool finished READ getFinished NOTIFY finishedChanged)

    Q_PROPERTY(bool complete READ getComplete NOTIFY stateChanged)
    Q_PROPERTY(State state READ getState NOTIFY stateChanged)
    Q_PROPERTY(Error error READ getError NOTIFY errorChanged)
    Q_PROPERTY(QString responseData READ getResponseData)
public:
    enum class Error {
        None,
        Unknown,
    };
    Q_ENUM(Error);

    enum class State {
        Idle,
        GettingCategories,
        UploadingAvatar,
        WaitingForUploadResponse,
        WaitingForInventory,
        Complete,
    };
    Q_ENUM(State);

    MarketplaceItemUploader(QString title,
                            QString description,
                            QString rootFilename,
                            QUuid marketplaceID,
                            QList<ProjectFilePath> filePaths);

    Q_INVOKABLE void send();

    void setError(Error error);

    QString getResponseData() const { return _responseData; }
    void setState(State newState);
    State getState() const { return _state; }
    bool getComplete() const { return _state == State::Complete; }

    QUuid getMarketplaceID() const { return _marketplaceID; }

    Error getError() const { return _error; }
    bool getFinished() const { return _state == State::Complete || _error != Error::None; }

signals:
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void completed();

    void stateChanged(State newState);
    void errorChanged(Error error);

    // Triggered when the upload has finished, either succesfully completing, or stopping with an error
    void finishedChanged();

private:
    void doGetCategories();
    void doUploadAvatar();
    void doWaitForInventory();

    QNetworkReply* _reply;

    State _state { State::Idle };
    Error _error { Error::None };

    QString _title;
    QString _description;
    QString _rootFilename;
    QUuid _marketplaceID;
    int _categoryID;
    int _itemVersion;

    QString _responseData;

    int _numRequestsForInventory { 0 };

    QString _rootFilePath;
    QList<ProjectFilePath> _filePaths;
    QByteArray _fileData;
};

#endif  // hifi_MarketplaceItemUploader_h
