import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebEngine 1.1

import "controls"
import "styles"

VrDialog {
    id: root
    HifiConstants { id: hifi }
    title: "WebWindow"
    resizable: true
    enabled: false
    visible: false
    // Don't destroy on close... otherwise the JS/C++ will have a dangling pointer
    destroyOnCloseButton: false
    contentImplicitWidth: clientArea.implicitWidth
    contentImplicitHeight: clientArea.implicitHeight
    backgroundColor: "#7f000000"
    property url source: "about:blank"

    signal navigating(string url)
    function stop() {
        webview.stop();
    }

    Component.onCompleted: {
        // Ensure the JS from the web-engine makes it to our logging
        webview.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
            console.log("Web Window JS message: " + sourceID + " " + lineNumber + " " +  message);
        });

    }

    Item {
        id: clientArea
        implicitHeight: 600
        implicitWidth: 800
        x: root.clientX
        y: root.clientY
        width: root.clientWidth
        height: root.clientHeight

        WebEngineView {
            id: webview
            url: root.source
            anchors.fill: parent
            focus: true

            property var originalUrl
            property var lastFixupTime: 0

            onUrlChanged: {
                var currentUrl = url.toString();
                var newUrl = urlHandler.fixupUrl(currentUrl).toString();
                if (newUrl != currentUrl) {
                    var now = new Date().valueOf();
                    if (url === originalUrl && (now - lastFixupTime < 100))  {
                        console.warn("URL fixup loop detected")
                        return;
                    }
                    originalUrl = url
                    lastFixupTime = now
                    url = newUrl;
                }
            }
    
            onLoadingChanged: {
                // Required to support clicking on "hifi://" links
                if (WebEngineView.LoadStartedStatus == loadRequest.status) {
                    var url = loadRequest.url.toString();
                    if (urlHandler.canHandleUrl(url)) {
                        if (urlHandler.handleUrl(url)) {
                            webview.stop();
                        }
                    }
                }
            }

            profile: WebEngineProfile {
                id: webviewProfile
                httpUserAgent: "Mozilla/5.0 (HighFidelityInterface)"
                storageName: "qmlWebEngine"
            } 
        }
    } // item
} // dialog
