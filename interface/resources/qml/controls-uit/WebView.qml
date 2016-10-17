//
//  WebView.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import HFWebEngineProfile 1.0
import "."

BaseWebView {
    onNewViewRequested: {
        // Load dialog via OffscreenUi so that JavaScript EventBridge is available.
        var browser = OffscreenUi.load("Browser.qml");
        request.openIn(browser.webView);
        browser.webView.forceActiveFocus();
    }
}
