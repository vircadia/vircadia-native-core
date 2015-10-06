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
#include <NetworkingConstants.h>

#include "Application.h"
#include "DataWebPage.h"

#include "DataWebDialog.h"

DataWebDialog::DataWebDialog() {
    // make sure the dialog deletes itself when it closes
    setAttribute(Qt::WA_DeleteOnClose);
    
    // set our page to a DataWebPage
    setPage(new DataWebPage(this));
    
    // have the Application handle external links
    connect(this, &QWebView::linkClicked, qApp, &Application::openUrl);
}

DataWebDialog* DataWebDialog::dialogForPath(const QString& path,
                                            const JavascriptObjectMap& javascriptObjects) {
    DataWebDialog* dialogWebView = new DataWebDialog();
    
    dialogWebView->_javascriptObjects = javascriptObjects;
    connect(dialogWebView->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared,
            dialogWebView, &DataWebDialog::addJavascriptObjectsToWindow);
    
    QUrl dataWebUrl(NetworkingConstants::METAVERSE_SERVER_URL);
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