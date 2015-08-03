//
//  AssetRequest.h
//
//  Created by Ryan Huffman on 2015/07/24
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssetRequest_h
#define hifi_AssetRequest_h

#include <QByteArray>
#include <QObject>
#include <QString>

#include "AssetClient.h"

#include "AssetUtils.h"

// You should be able to get an asset from any thread, and handle the responses in a safe way
// on your own thread. Everything should happen on AssetClient's thread, the caller should
// receive events by connecting to signals on an object that lives on AssetClient's threads.

// Receives parts of an asset and puts them together
// Emits signals:
//   Progress
//   Completion, success or error
// On finished, the AssetClient is effectively immutable and can be read from
// any thread safely
//
// Will often make multiple requests to the AssetClient to get data
class AssetRequest : public QObject {
    Q_OBJECT
public:
    enum State {
        NOT_STARTED = 0,
        WAITING_FOR_INFO,
        WAITING_FOR_DATA,
        FINISHED
    };

    enum Result {
        Success = 0,
        Timeout,
        NotFound,
        Error,
    };

    AssetRequest(QObject* parent, QString hash);

    Q_INVOKABLE void start();
    //AssetRequest* requestAsset(QString hash);
    // Create AssetRequest
    // Start request for hash
    // Store messageID -> AssetRequest
    // When complete:
    //   Update AssetRequest
    //   AssetRequest emits signal

    void receiveData(DataOffset start, DataOffset end, QByteArray data);
    const QByteArray& getData();

signals:
    void finished(AssetRequest*);
    void progress(uint64_t totalReceived, uint64_t total);

private:
    State _state = NOT_STARTED;
    Result _result;
    AssetInfo _info;
    uint64_t _totalReceived { 0 };
    QString _hash;
    QByteArray _data;
    int _numPendingRequests { 0 };
    // Timeout
};

#endif
