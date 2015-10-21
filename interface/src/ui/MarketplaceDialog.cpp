//
//  MarketplaceDialog.cpp
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "MarketplaceDialog.h"
#include "DependencyManager.h"

HIFI_QML_DEF(MarketplaceDialog)


MarketplaceDialog::MarketplaceDialog(QQuickItem* parent) : OffscreenQmlDialog(parent) {
}

bool MarketplaceDialog::navigationRequested(const QString& url) {
    qDebug() << url;
    if (qApp->canAcceptURL(url)) {
        if (qApp->acceptURL(url)) {
            return false; // we handled it, so QWebPage doesn't need to handle it
        }
    }
    return true;
}
