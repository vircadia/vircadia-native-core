//
//  SnapshotShareDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/16/14.
//
//

#include "SnapshotShareDialog.h"
#include "AccountManager.h"

#include <QString>
#include <QUrlQuery>
#include <QtNetwork/QNetworkRequest>
#include <QHttpMultiPart>

const int NARROW_SNAPSHOT_DIALOG_SIZE = 500;
const int WIDE_SNAPSHOT_DIALOG_WIDTH = 650;
const QString FORUM_POST_URL = "http://localhost:4000";

SnapshotShareDialog::SnapshotShareDialog(QString fileName, QWidget* parent) : QDialog(parent), _fileName(fileName) {

    setAttribute(Qt::WA_DeleteOnClose);

    ui.setupUi(this);

    QPixmap snaphsotPixmap(fileName);
    float snapshotRatio = static_cast<float>(snaphsotPixmap.size().width()) / snaphsotPixmap.size().height();

    // narrow snapshot
    if (snapshotRatio > 1) {
        setFixedWidth(WIDE_SNAPSHOT_DIALOG_WIDTH);
        ui.snapshotWidget->setFixedWidth(WIDE_SNAPSHOT_DIALOG_WIDTH);
    }

    float labelRatio = static_cast<float>(ui.snapshotWidget->size().width()) / ui.snapshotWidget->size().height();

    // set the same aspect ratio of label as of snapshot
    if (snapshotRatio > labelRatio) {
        int oldHeight = ui.snapshotWidget->size().height();
        ui.snapshotWidget->setFixedHeight(ui.snapshotWidget->size().width() / snapshotRatio);

        // if height is less then original, resize the window as well
        if (ui.snapshotWidget->size().height() < NARROW_SNAPSHOT_DIALOG_SIZE) {
            setFixedHeight(size().height() - (oldHeight - ui.snapshotWidget->size().height()));
        }
    } else {
        ui.snapshotWidget->setFixedWidth(ui.snapshotWidget->size().height() * snapshotRatio);
    }

    ui.snapshotWidget->setPixmap(snaphsotPixmap);
    ui.snapshotWidget->adjustSize();
}

void SnapshotShareDialog::accept() {

    close();
    
    // post to Discourse forum
//    AccountManager& accountManager = AccountManager::getInstance();
    QNetworkAccessManager* _networkAccessManager = NULL;

    if (!_networkAccessManager) {
        _networkAccessManager = new QNetworkAccessManager(this);
    }

    QNetworkRequest request;

    QUrl grantURL(FORUM_POST_URL);
    grantURL.setPath("/posts");


    QByteArray postData;
    //    postData.append("api_key=" + accountManager.getAccountInfo().getDiscourseApiKey() + "&");
    postData.append("api_key=9168f53930b2fc69ec278414d6ff04fed723ef717867a25954143150d3e2dfe8&");
    postData.append("topic_id=64&");
    postData.append("raw=" + QUrl::toPercentEncoding(ui.textEdit->toPlainText()));

    request.setUrl(grantURL);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* requestReply = _networkAccessManager->post(request, postData);
    connect(_networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(serviceRequestFinished(QNetworkReply*)));

    connect(requestReply, &QNetworkReply::finished, this, &SnapshotShareDialog::requestFinished);
    connect(requestReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
}

void SnapshotShareDialog::serviceRequestFinished(QNetworkReply* reply) {
    qDebug() << reply->errorString();
}

void SnapshotShareDialog::requestFinished() {

    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());

    qDebug() << requestReply->errorString();
    delete requestReply;
}

void SnapshotShareDialog::requestError(QNetworkReply::NetworkError error) {
    // TODO: error handling
    qDebug() << "AccountManager requestError - " << error;
}