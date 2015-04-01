//
//  FriendsWindow.cpp
//  interface/src/ui
//
//  Created by David Rowe on 1 Apr 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QVBoxLayout>
#include <QWebView>

#include "DataWebPage.h"
#include "FriendsWindow.h"

const QString EDIT_FRIENDS_DIALOG_URL = "https://metaverse.highfidelity.com/user/friends";

FriendsWindow::FriendsWindow(QWidget* parent) {
    this->setWindowTitle("Add/Remove Friends");
    this->setAttribute(Qt::WA_DeleteOnClose);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    QWebView* webView = new QWebView(this);
    layout->addWidget(webView);
    webView->setPage(new DataWebPage());
    webView->setUrl(EDIT_FRIENDS_DIALOG_URL);
}
