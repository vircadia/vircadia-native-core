//
//  WebBrowserSuggestionsEngine.cpp
//  interface/src/webbrowser
//
//  Created by Vlad Stelmahovsky on 30/10/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WebBrowserSuggestionsEngine.h"
#include "qregexp.h"

#include <qbuffer.h>
#include <qcoreapplication.h>
#include <qlocale.h>
#include <qnetworkrequest.h>
#include <qnetworkreply.h>
#include <qregexp.h>
#include <qstringlist.h>

#include <QUrlQuery>
#include <QJsonDocument>

#include <NetworkAccessManager.h>

static const QString GoogleSuggestionsUrl = "https://suggestqueries.google.com/complete/search?output=firefox&q=%1";
static const int SUGGESTIONS_LIST_INDEX = 1;

WebBrowserSuggestionsEngine::WebBrowserSuggestionsEngine(QObject* parent)
    : QObject(parent)
    , _suggestionsReply(0) {
    _currentNAM = &NetworkAccessManager::getInstance();
    connect(_currentNAM, &QNetworkAccessManager::finished, this, &WebBrowserSuggestionsEngine::suggestionsFinished);
}


WebBrowserSuggestionsEngine::~WebBrowserSuggestionsEngine() {
    disconnect(_currentNAM, &QNetworkAccessManager::finished, this, &WebBrowserSuggestionsEngine::suggestionsFinished);
}

void WebBrowserSuggestionsEngine::querySuggestions(const QString &searchString) {
    if (_suggestionsReply) {
        _suggestionsReply->disconnect(this);
        _suggestionsReply->abort();
        _suggestionsReply->deleteLater();
        _suggestionsReply = 0;
    }
    QString url = QString(GoogleSuggestionsUrl).arg(searchString);
    _suggestionsReply = _currentNAM->get(QNetworkRequest(url));
}

void WebBrowserSuggestionsEngine::suggestionsFinished(QNetworkReply *reply) {

    if (reply != _suggestionsReply) {
        return; //invalid reply. ignore
    }

    const QByteArray response = _suggestionsReply->readAll();

    _suggestionsReply->close();
    _suggestionsReply->deleteLater();
    _suggestionsReply = 0;

    QJsonParseError err;
    QJsonDocument json = QJsonDocument::fromJson(response, &err);
    const QVariant res = json.toVariant();

    if (err.error != QJsonParseError::NoError || res.type() != QVariant::List) {
        return;
    }

    const QVariantList list = res.toList();

    if (list.size() <= SUGGESTIONS_LIST_INDEX) {
        return;
    }

    QStringList out;
    const QVariantList& suggList = list.at(SUGGESTIONS_LIST_INDEX).toList();

    foreach (const QVariant &v, suggList) {
        out.append(v.toString());
    }

    emit suggestions(out);
}
