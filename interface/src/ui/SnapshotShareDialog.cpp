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
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QUrlQuery>


const int NARROW_SNAPSHOT_DIALOG_SIZE = 500;
const int WIDE_SNAPSHOT_DIALOG_WIDTH = 650;
const int SUCCESS_LABEL_HEIGHT = 140;

const QString FORUM_URL = "https://alphas.highfidelity.io";
const QString FORUM_UPLOADS_URL = FORUM_URL + "/uploads";
const QString FORUM_POST_URL = FORUM_URL + "/posts";
const QString FORUM_REPLY_TO_TOPIC = "244";
const QString FORUM_POST_TEMPLATE = "<img src='%1'/><p>%2</p>";
const QString SHARE_DEFAULT_ERROR = "The server isn't responding. Please try again in a few minutes.";
const QString SUCCESS_LABEL_TEMPLATE = "Success!!! Go check out your image ...<br/><a style='color:#333;text-decoration:none' href='%1'>%1</a>";
const QString SHARE_BUTTON_STYLE = "border-width:0;border-radius:9px;border-radius:9px;font-family:Arial;font-size:18px;"
    "font-weight:100;color:#FFFFFF;width: 120px;height: 50px;";
const QString SHARE_BUTTON_ENABLED_STYLE = "background-color: #333;";
const QString SHARE_BUTTON_DISABLED_STYLE = "background-color: #999;";

Q_DECLARE_METATYPE(QNetworkAccessManager::Operation)

SnapshotShareDialog::SnapshotShareDialog(QString fileName, QWidget* parent) :
    QDialog(parent),
    _fileName(fileName)
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
    // prevent multiple clicks on share button
    _ui.shareButton->setEnabled(false);
    // gray out share button
    _ui.shareButton->setStyleSheet(SHARE_BUTTON_STYLE + SHARE_BUTTON_DISABLED_STYLE);
    uploadSnapshot();
}

void SnapshotShareDialog::uploadSnapshot() {

    if (AccountManager::getInstance().getAccountInfo().getDiscourseApiKey().isEmpty()) {
        QMessageBox::warning(this, "",
                             "Your Discourse API key is missing, you cannot share snapshots. Please try to relog.");
        return;
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart apiKeyPart;
    apiKeyPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"api_key\""));
    apiKeyPart.setBody(AccountManager::getInstance().getAccountInfo().getDiscourseApiKey().toLatin1());

    QFile* file = new QFile(_fileName);
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

    QNetworkReply* reply = NetworkAccessManager::getInstance().post(request, multiPart);
    connect(reply, &QNetworkReply::finished, this, &SnapshotShareDialog::uploadRequestFinished);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void SnapshotShareDialog::sendForumPost(QString snapshotPath) {
    // post to Discourse forum
    QNetworkRequest request;
    QUrl forumUrl(FORUM_POST_URL);

    QUrlQuery query;
    query.addQueryItem("api_key", AccountManager::getInstance().getAccountInfo().getDiscourseApiKey());
    query.addQueryItem("topic_id", FORUM_REPLY_TO_TOPIC);
    query.addQueryItem("raw", FORUM_POST_TEMPLATE.arg(snapshotPath, _ui.textEdit->toPlainText()));
    forumUrl.setQuery(query);

    QByteArray postData = forumUrl.toEncoded(QUrl::RemoveFragment);
    request.setUrl(forumUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* requestReply = NetworkAccessManager::getInstance().post(request, postData);
    connect(requestReply, &QNetworkReply::finished, this, &SnapshotShareDialog::postRequestFinished);

    QEventLoop loop;
    connect(requestReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void SnapshotShareDialog::postRequestFinished() {

    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());
    QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
    const QJsonObject& responseObject = jsonResponse.object();

    if (responseObject.contains("id")) {
        _ui.textEdit->setHtml("");
        const QString urlTemplate = "%1/t/%2/%3/%4";
        QString link = urlTemplate.arg(FORUM_URL,
                                        responseObject["topic_slug"].toString(),
                                        QString::number(responseObject["topic_id"].toDouble()),
                                        QString::number(responseObject["post_number"].toDouble()));

        _ui.successLabel->setText(SUCCESS_LABEL_TEMPLATE.arg(link));
        _ui.successLabel->setFixedHeight(SUCCESS_LABEL_HEIGHT);

        // hide input widgets
        _ui.shareButton->hide();
        _ui.textEdit->hide();
        _ui.labelNotes->hide();

    } else {
        QString errorMessage(SHARE_DEFAULT_ERROR);
        if (responseObject.contains("errors")) {
            QJsonArray errorArray = responseObject["errors"].toArray();
            if (!errorArray.first().toString().isEmpty()) {
                errorMessage = errorArray.first().toString();
            }
        }
        QMessageBox::warning(this, "", errorMessage);
        _ui.shareButton->setEnabled(true);
        _ui.shareButton->setStyleSheet(SHARE_BUTTON_STYLE + SHARE_BUTTON_ENABLED_STYLE);
    }
}

void SnapshotShareDialog::uploadRequestFinished() {

    QNetworkReply* requestReply = reinterpret_cast<QNetworkReply*>(sender());
    QJsonDocument jsonResponse = QJsonDocument::fromJson(requestReply->readAll());
    const QJsonObject& responseObject = jsonResponse.object();

    if (responseObject.contains("url")) {
        sendForumPost(responseObject["url"].toString());
    } else {
        QMessageBox::warning(this, "", SHARE_DEFAULT_ERROR);
        _ui.shareButton->setEnabled(true);
        _ui.shareButton->setStyleSheet(SHARE_BUTTON_STYLE + SHARE_BUTTON_ENABLED_STYLE);
    }

    delete requestReply;
}
