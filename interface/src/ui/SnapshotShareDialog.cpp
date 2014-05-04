//
//  SnapshotShareDialog.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SnapshotShareDialog.h"
#include "AccountManager.h"

#include <QFile>
#include <QUrlQuery>
#include <QHttpMultiPart>

const int NARROW_SNAPSHOT_DIALOG_SIZE = 500;
const int WIDE_SNAPSHOT_DIALOG_WIDTH = 650;

const QString FORUM_URL = "http://localhost:4000";
const QString FORUM_UPLOADS_URL = FORUM_URL + "/uploads";
const QString FORUM_POST_URL = FORUM_URL + "/posts";
const QString FORUM_REPLY_TO_TOPIC = "64";
const QString FORUM_POST_TEMPLATE = "<img src='%1'/><p>%2</p>";

Q_DECLARE_METATYPE(QNetworkAccessManager::Operation)

SnapshotShareDialog::SnapshotShareDialog(QString fileName, QWidget* parent) :
    QDialog(parent),
    _fileName(fileName),
    _networkAccessManager(NULL)
{

    setAttribute(Qt::WA_DeleteOnClose);

    _ui.setupUi(this);

    QPixmap snaphsotPixmap(fileName);
    float snapshotRatio = static_cast<float>(snaphsotPixmap.size().width()) / snaphsotPixmap.size().height();

    // narrow snapshot
    if (snapshotRatio > 1) {
        setFixedWidth(WIDE_SNAPSHOT_DIALOG_WIDTH);
        _ui.snapshotWidget->setFixedWidth(WIDE_SNAPSHOT_DIALOG_WIDTH);
    }

    float labelRatio = static_cast<float>(_ui.snapshotWidget->size().width()) / _ui.snapshotWidget->size().height();

    // set the same aspect ratio of label as of snapshot
    if (snapshotRatio > labelRatio) {
        int oldHeight = _ui.snapshotWidget->size().height();
        _ui.snapshotWidget->setFixedHeight((int) (_ui.snapshotWidget->size().width() / snapshotRatio));

        // if height is less then original, resize the window as well
        if (_ui.snapshotWidget->size().height() < NARROW_SNAPSHOT_DIALOG_SIZE) {
            setFixedHeight(size().height() - (oldHeight - _ui.snapshotWidget->size().height()));
        }
    } else {
        _ui.snapshotWidget->setFixedWidth((int) (_ui.snapshotWidget->size().height() * snapshotRatio));
    }

    _ui.snapshotWidget->setPixmap(snaphsotPixmap);
    _ui.snapshotWidget->adjustSize();
}

void SnapshotShareDialog::accept() {
    uploadSnapshot();
    sendForumPost("/uploads/default/25/b607c8faea6de9c3.jpg");
    close();
}

void SnapshotShareDialog::uploadSnapshot() {

    if (!_networkAccessManager) {
        _networkAccessManager = new QNetworkAccessManager(this);
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart apiKeyPart;
    apiKeyPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"api_key\""));
    apiKeyPart.setBody("9168f53930b2fc69ec278414d6ff04fed723ef717867a25954143150d3e2dfe8");
//    apiKeyPart.setBody(AccountManager::getInstance().getAccountInfo().getDiscourseApiKey().toLatin1());

    QFile *file = new QFile(_fileName);
    file->open(QIODevice::ReadOnly);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"file\"; filename=\"" + file->fileName() +"\""));
    imagePart.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

    multiPart->append(apiKeyPart);
    multiPart->append(imagePart);

    QUrl url(FORUM_UPLOADS_URL);
    QNetworkRequest request(url);

    QNetworkReply *reply = _networkAccessManager->post(request, multiPart);
    bool check;
    Q_UNUSED(check);

    check = connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));
    Q_ASSERT(check);
}

void SnapshotShareDialog::sendForumPost(QString snapshotPath) {

    if (!_networkAccessManager) {
        _networkAccessManager = new QNetworkAccessManager(this);
    }

    // post to Discourse forum
    QNetworkRequest request;
    QUrl forumUrl(FORUM_POST_URL);

    QUrlQuery query;
//    query.addQueryItem("api_key", accountManager.getAccountInfo().getDiscourseApiKey();
    query.addQueryItem("api_key", "9168f53930b2fc69ec278414d6ff04fed723ef717867a25954143150d3e2dfe8");
    query.addQueryItem("topic_id", FORUM_REPLY_TO_TOPIC);
    query.addQueryItem("raw", FORUM_POST_TEMPLATE.arg(snapshotPath, _ui.textEdit->toPlainText()));
    forumUrl.setQuery(query);

    QByteArray postData = forumUrl.toEncoded(QUrl::RemoveFragment);
    request.setUrl(forumUrl);
    QNetworkReply* requestReply = _networkAccessManager->post(request, postData);

    bool check;
    Q_UNUSED(check);

    check = connect(requestReply, &QNetworkReply::finished, this, &SnapshotShareDialog::requestFinished);
    Q_ASSERT(check);

    check = connect(requestReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
    Q_ASSERT(check);
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
