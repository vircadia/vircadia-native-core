//
//  DataWebPage.h
//  interface/src/ui
//
//  Created by Stephen Birarda on 2014-09-22.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DataWebPage_h
#define hifi_DataWebPage_h

#include <qwebpage.h>

class DataWebPage : public QWebPage {
public:
    DataWebPage(QObject* parent = 0);
protected:
    void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID) override;
    bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, QWebPage::NavigationType type) override;
    virtual QString userAgentForUrl(const QUrl& url) const override;
};

#endif // hifi_DataWebPage_h
