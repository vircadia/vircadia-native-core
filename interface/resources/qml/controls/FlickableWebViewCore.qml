import QtQuick 2.7
import QtWebEngine 1.5
import QtWebChannel 1.0

import QtQuick.Controls 2.2

import "../styles-uit" as StylesUIt

Flickable {
    id: flick

    property alias url: _webview.url
    property alias canGoBack: _webview.canGoBack
    property alias webViewCore: _webview
    property alias webViewCoreProfile: _webview.profile

    property string userScriptUrl: ""
    property string urlTag: "noDownload=false";

    signal newViewRequestedCallback(var request)
    signal loadingChangedCallback(var loadRequest)

    boundsBehavior: Flickable.StopAtBounds

    StylesUIt.HifiConstants {
        id: hifi
    }

    onHeightChanged: {
        if (height > 0) {
            //reload page since window dimentions changed,
            //so web engine should recalculate page render dimentions
            reloadTimer.start()
        }
    }

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        visible: flick.contentHeight > flick.height

        contentItem: Rectangle {
            opacity: 0.75
            implicitWidth: hifi.dimensions.scrollbarHandleWidth
            radius: height / 2
            color: hifi.colors.tableScrollHandleDark
        }
    }

    function onLoadingChanged(loadRequest) {
        if (WebEngineView.LoadStartedStatus === loadRequest.status) {
            flick.contentWidth = flick.width
            flick.contentHeight = flick.height

            // Required to support clicking on "hifi://" links
            var url = loadRequest.url.toString();
            if (urlHandler.canHandleUrl(url)) {
                if (urlHandler.handleUrl(url)) {
                    _webview.stop();
                }
            }
        }

        if (WebEngineView.LoadFailedStatus === loadRequest.status) {
            console.log(" Tablet WebEngineView failed to load url: " + loadRequest.url.toString());
        }

        if (WebEngineView.LoadSucceededStatus === loadRequest.status) {
            //disable Chromium's scroll bars
            _webview.runJavaScript("document.body.style.overflow = 'hidden';");
            //calculate page height
            _webview.runJavaScript("document.body.scrollHeight;", function (i_actualPageHeight) {
                if (i_actualPageHeight !== undefined) {
                    flick.contentHeight = i_actualPageHeight
                } else {
                    flick.contentHeight = flick.height;
                }
            })
            flick.contentWidth = flick.width
        }
    }

    Timer {
        id: reloadTimer
        interval: 100
        repeat: false
        onTriggered: {
            _webview.reload()
        }
    }

    WebEngineView {
        id: _webview

        height: parent.height

        profile: HFWebEngineProfile;

        // creates a global EventBridge object.
        WebEngineScript {
            id: createGlobalEventBridge
            sourceCode: eventBridgeJavaScriptToInject
            injectionPoint: WebEngineScript.DocumentCreation
            worldId: WebEngineScript.MainWorld
        }

        // detects when to raise and lower virtual keyboard
        WebEngineScript {
            id: raiseAndLowerKeyboard
            injectionPoint: WebEngineScript.Deferred
            sourceUrl: resourceDirectoryUrl + "/html/raiseAndLowerKeyboard.js"
            worldId: WebEngineScript.MainWorld
        }

        // User script.
        WebEngineScript {
            id: userScript
            sourceUrl: flick.userScriptUrl
            injectionPoint: WebEngineScript.DocumentReady  // DOM ready but page load may not be finished.
            worldId: WebEngineScript.MainWorld
        }

        userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard, userScript ]

        property string newUrl: ""

        Component.onCompleted: {
            width = Qt.binding(function() { return flick.width; });
            webChannel.registerObject("eventBridge", eventBridge);
            webChannel.registerObject("eventBridgeWrapper", eventBridgeWrapper);
            // Ensure the JS from the web-engine makes it to our logging
            _webview.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
                console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
            });
            _webview.profile.httpUserAgent = "Mozilla/5.0 Chrome (HighFidelityInterface)";

        }

        onFeaturePermissionRequested: {
            grantFeaturePermission(securityOrigin, feature, true);
        }

        onContentsSizeChanged: {
            flick.contentHeight = Math.max(contentsSize.height, flick.height);
            flick.contentWidth = flick.width
        }
        //disable popup
        onContextMenuRequested: {
            request.accepted = true;
        }

        onNewViewRequested: {
            newViewRequestedCallback(request)
        }

        onLoadingChanged: {
            flick.onLoadingChanged(loadRequest)
            loadingChangedCallback(loadRequest)
        }
    }

    AnimatedImage {
        //anchoring doesnt works here when changing content size
        x: flick.width/2 - width/2
        y: flick.height/2 - height/2
        source: "../../icons/loader-snake-64-w.gif"
        visible: _webview.loading && /^(http.*|)$/i.test(_webview.url.toString())
        playing: visible
        z: 10000
    }
}
