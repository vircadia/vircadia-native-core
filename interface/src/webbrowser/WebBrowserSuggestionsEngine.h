//
//  WebBrowserSuggestionsEngine.h
//  interface/src/webbrowser
//
//  Created by Vlad Stelmahovsky on 30/10/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef WEBBROWSERSUGGESTIONSENGINE_H
#define WEBBROWSERSUGGESTIONSENGINE_H

#include <qpair.h>
#include <qimage.h>
#include <qmap.h>

#include <qnetworkaccessmanager.h>
#include <qstring.h>
#include <qurl.h>

class QNetworkReply;

class WebBrowserSuggestionsEngine : public QObject {
    Q_OBJECT

public:
    WebBrowserSuggestionsEngine(QObject* parent = 0);
    virtual ~WebBrowserSuggestionsEngine();

public slots:
    void querySuggestions(const QString& searchString);

signals:
    void suggestions(const QStringList& suggestions);

private slots:
    void suggestionsFinished(QNetworkReply *reply);
private:
    QNetworkReply* _suggestionsReply;
    QNetworkAccessManager* _currentNAM;
};

#endif // WEBBROWSERSUGGESTIONSENGINE_H

