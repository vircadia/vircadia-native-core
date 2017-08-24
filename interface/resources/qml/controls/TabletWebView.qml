import QtQuick 2.7
//import QtQuick.Controls 1.4
import QtWebEngine 1.5
//import QtWebChannel 1.0
import "../controls-uit" as HiFiControls
import "../styles" as HifiStyles
import "../styles-uit"
//import "../"
//import "."

Item {
    id: root
    HifiConstants { id: hifi }
    width: parent !== null ? parent.width : undefined
    height: parent !== null ? parent.height : undefined
    property var parentStackItem: null
    property int headerHeight: 70
    property string url
    property string scriptURL
    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool isDesktop: false
    property alias webView: web.webViewCore
    property alias profile: web.webViewCoreProfile
    property bool remove: false
    property bool closeButtonVisible: true

    // Manage own browse history because WebEngineView history is wiped when a new URL is loaded via
    // onNewViewRequested, e.g., as happens when a social media share button is clicked.
    property var history: []
    property int historyIndex: -1

    Rectangle {
        id: buttons
        width: parent.width
        height: parent.headerHeight
        color: hifi.colors.white

        Row {
            id: nav
            anchors {
                top: parent.top
                topMargin: 10
                horizontalCenter: parent.horizontalCenter
            }
            spacing: 120
            
            TabletWebButton {
                id: back
                enabledColor: hifi.colors.darkGray
                disabledColor: hifi.colors.lightGrayText
                enabled: historyIndex > 0
                text: "BACK"

                MouseArea {
                    anchors.fill: parent
                    onClicked: goBack()
                }
            }

            TabletWebButton {
                id: close
                enabledColor: hifi.colors.darkGray
                disabledColor: hifi.colors.lightGrayText
                enabled: true
                text: "CLOSE"
                visible: closeButtonVisible

                MouseArea {
                    anchors.fill: parent
                    onClicked: closeWebEngine()
                }
            }
        }

        RalewaySemiBold {
            id: displayUrl
            color: hifi.colors.baseGray
            font.pixelSize: 12
            verticalAlignment: Text.AlignLeft
            text: root.url
            anchors {
                top: nav.bottom
                horizontalCenter: parent.horizontalCenter;
                left: parent.left
                leftMargin: 20
            }
        }

        MouseArea {
            anchors.fill: parent
            preventStealing: true
            propagateComposedEvents: true
        }
    }

    function goBack() {
        if (historyIndex > 0) {
            historyIndex--;
            loadUrl(history[historyIndex]);
        }
    }

    function closeWebEngine() {
        if (remove) {
            root.destroy();
            return;
        }
        if (parentStackItem) {
            parentStackItem.pop();
        } else {
            root.visible = false;
        }
    }

    function goForward() {
        if (historyIndex < history.length - 1) {
            historyIndex++;
            loadUrl(history[historyIndex]);
        }
    }

    function reloadPage() {
        view.reloadAndBypassCache()
        view.setActiveFocusOnPress(true);
        view.setEnabled(true);
    }

    function loadUrl(url) {
        web.webViewCore.url = url
        root.url = web.webViewCore.url;
    }

    onUrlChanged: {
        loadUrl(url);
    }

    FlickableWebViewCore {
        id: web
        width: parent.width
        height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height - root.headerHeight : parent.height - root.headerHeight
        anchors.top: buttons.bottom

        onUrlChanged: {
            // Record history, skipping null and duplicate items.
            var urlString = url + "";
            urlString = urlString.replace(/\//g, "%2F");  // Consistent representation of "/"s to avoid false differences.
            if (urlString.length > 0 && (historyIndex === -1 || urlString !== history[historyIndex])) {
                historyIndex++;
                history = history.slice(0, historyIndex);
                history.push(urlString);
            }
        }

        onLoadingChangedCallback: {
            keyboardRaised = false;
            punctuationMode = false;
            keyboard.resetShiftMode(false);
            webViewCore.forceActiveFocus();
        }

        onNewViewRequestedCallback: {
            request.openIn(webViewCore);
        }
    }

//    Flickable {
//        id: flick
//        width: parent.width
//        height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height - web.headerHeight : parent.height - web.headerHeight
//        anchors.top: buttons.bottom

//        WebEngineView {
//            id: webview
//            objectName: "webEngineView"
//            anchors.fill: parent

//            profile: HFWebEngineProfile;

//            property string userScriptUrl: ""

//            // creates a global EventBridge object.
//            WebEngineScript {
//                id: createGlobalEventBridge
//                sourceCode: eventBridgeJavaScriptToInject
//                injectionPoint: WebEngineScript.DocumentCreation
//                worldId: WebEngineScript.MainWorld
//            }

//            // detects when to raise and lower virtual keyboard
//            WebEngineScript {
//                id: raiseAndLowerKeyboard
//                injectionPoint: WebEngineScript.Deferred
//                sourceUrl: resourceDirectoryUrl + "/html/raiseAndLowerKeyboard.js"
//                worldId: WebEngineScript.MainWorld
//            }

//            // User script.
//            WebEngineScript {
//                id: userScript
//                sourceUrl: webview.userScriptUrl
//                injectionPoint: WebEngineScript.DocumentReady  // DOM ready but page load may not be finished.
//                worldId: WebEngineScript.MainWorld
//            }

//            property string urlTag: "noDownload=false";
//            userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard, userScript ]

//            property string newUrl: ""

//            Component.onCompleted: {
//                webChannel.registerObject("eventBridge", eventBridge);
//                webChannel.registerObject("eventBridgeWrapper", eventBridgeWrapper);
//                // Ensure the JS from the web-engine makes it to our logging
//                webview.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
//                    console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
//                });
//            }

//            onFeaturePermissionRequested: {
//                grantFeaturePermission(securityOrigin, feature, true);
//            }

//            onUrlChanged: {
//                // Record history, skipping null and duplicate items.
//                var urlString = url + "";
//                urlString = urlString.replace(/\//g, "%2F");  // Consistent representation of "/"s to avoid false differences.
//                if (urlString.length > 0 && (historyIndex === -1 || urlString !== history[historyIndex])) {
//                    historyIndex++;
//                    history = history.slice(0, historyIndex);
//                    history.push(urlString);
//                }
//            }

//            onContentsSizeChanged: {
//                flick.contentHeight = Math.max(contentsSize.height, flick.height);
//                flick.contentWidth = flick.width
//            }
//            //disable popup
//            onContextMenuRequested: {
//                request.accepted = true;
//            }

//            onLoadingChanged: {
//                keyboardRaised = false;
//                punctuationMode = false;
//                keyboard.resetShiftMode(false);
//                // Required to support clicking on "hifi://" links
//                if (WebEngineView.LoadStartedStatus == loadRequest.status) {
//                    flick.contentWidth = 0
//                    flick.contentHeight = 0
//                    var url = loadRequest.url.toString();
//                    if (urlHandler.canHandleUrl(url)) {
//                        if (urlHandler.handleUrl(url)) {
//                            root.stop();
//                        }
//                    }
//                }

//                if (WebEngineView.LoadFailedStatus == loadRequest.status) {
//                    console.log(" Tablet WebEngineView failed to load url: " + loadRequest.url.toString());
//                }

//                if (WebEngineView.LoadSucceededStatus == loadRequest.status) {
//                    webview.runJavaScript("document.body.scrollHeight;", function (i_actualPageHeight) {
//                        flick.contentHeight = Math.max(i_actualPageHeight, flick.height);
//                    })
//                    flick.contentWidth = flick.width

//                    webview.forceActiveFocus();
//                }
//            }

//            onNewViewRequested: {
//                request.openIn(webview);
//            }
//        }
//    }

//    HiFiControls.WebSpinner {
//        webview: webview
//        anchors.centerIn: parent
//    }

    HiFiControls.Keyboard {
        id: keyboard
        raised: parent.keyboardEnabled && parent.keyboardRaised
        numeric: parent.punctuationMode

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
    
    Component.onCompleted: {
        root.isDesktop = (typeof desktop !== "undefined");
        keyboardEnabled = HMD.active;
    }

    Keys.onPressed: {
        switch(event.key) {
        case Qt.Key_L:
            if (event.modifiers == Qt.ControlModifier) {
                event.accepted = true
            }
            break;
        }
    }
}
