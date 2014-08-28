//
//  SnapshotShareDialog.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_snapshotShareDialog
#define hifi_snapshotShareDialog

#include "ui_shareSnapshot.h"

#include <QNetworkReply>
#include <QUrl>

class SnapshotShareDialog : public QDialog {
    Q_OBJECT

public:
    SnapshotShareDialog(QString fileName, QWidget* parent = 0);

private:
    QString _fileName;
    Ui_SnapshotShareDialog _ui;

    void uploadSnapshot();
    void sendForumPost(QString snapshotPath);

private slots:
    void uploadRequestFinished();
    void postRequestFinished();
    void accept();
};

#endif // hifi_snapshotShareDialog
