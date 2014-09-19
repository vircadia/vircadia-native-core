//
//  DataWebDialog.cpp
//  interface/src/ui
//
//  Created by Stephen Birarda on 2014-09-17.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qwebframe.h>
#include <qwebview.h>

#include <AccountManager.h>
#include <LimitedNodeList.h>
#include <OAuthNetworkAccessManager.h>

#include "Application.h"

#include "DataWebDialog.h"

DataWebDialog::DataWebDialog() {
    // make sure the dialog deletes itself when it closes
    setAttribute(Qt::WA_DeleteOnClose);
    
    // use an OAuthNetworkAccessManager instead of regular QNetworkAccessManager so our requests are authed
    page()->setNetworkAccessManager(OAuthNetworkAccessManager::getInstance());
    
    // have the page delegate external links so they can be captured by the Application in case they are a hifi link
    page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    
    // have the Application handle external links
    connect(this, &QWebView::linkClicked, Application::getInstance(), &Application::openUrl);
}

DataWebDialog* DataWebDialog::dialogForPath(const QString& path,
                                            const JavascriptObjectMap& javascriptObjects) {
    DataWebDialog* dialogWebView = new DataWebDialog();
    
    dialogWebView->_javascriptObjects = javascriptObjects;
    connect(dialogWebView->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared,
            dialogWebView, &DataWebDialog::addJavascriptObjectsToWindow);
    
    QUrl dataWebUrl(DEFAULT_NODE_AUTH_URL);
    dataWebUrl.setPath(path);
    
    qDebug() << "Opening a data web dialog for" << dataWebUrl.toString();
    
    dialogWebView->load(dataWebUrl);
    
    return dialogWebView;
}

void DataWebDialog::addJavascriptObjectsToWindow() {
    foreach(const QString& name, _javascriptObjects.keys()) {
        page()->mainFrame()->addToJavaScriptWindowObject(name, _javascriptObjects[name]);
    }
}