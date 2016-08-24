//
//  SnapshotUploader.h
//  interface/src/ui
//
//  Created by Howard Stearns on 8/22/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SnapshotUploader_h
#define hifi_SnapshotUploader_h

#include <QObject>
#include <QtNetwork/QNetworkReply>

class SnapshotUploader : public QObject {
    Q_OBJECT
        public slots:
    void uploadSuccess(QNetworkReply& reply);
    void uploadFailure(QNetworkReply& reply);
    void createStorySuccess(QNetworkReply& reply);
    void createStoryFailure(QNetworkReply& reply);
};
#endif // hifi_SnapshotUploader_h