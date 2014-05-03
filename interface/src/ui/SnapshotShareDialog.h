//
//  SnapshotShareDialog.h
//  hifi
//
//  Created by Stojce Slavkovski on 2/16/14.
//
//

#ifndef __hifi__snapshotShareDialog__
#define __hifi__snapshotShareDialog__

#include "ui_shareSnapshot.h"
#include <QNetworkReply>

class SnapshotShareDialog : public QDialog {
    Q_OBJECT

public:
    SnapshotShareDialog(QString fileName, QWidget* parent = 0);

private:
    QString _fileName;
    Ui_SnapshotShareDialog ui;

public slots:
    void requestFinished();
    void requestError(QNetworkReply::NetworkError error);
    void serviceRequestFinished(QNetworkReply* reply);

private slots:
    void accept();
};

#endif /* defined(__hifi__snapshotShareDialog__) */
