import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.2
import QtWebChannel 1.0
import "../controls-uit" as HiFiControls
import "../styles" as HifiStyles
import "../styles-uit"
import "../"
import "."

Item {
    id: web
    HifiConstants { id: hifi }
    width: parent.width
    height: parent.height
    property var parentStackItem: null
    property int headerHeight: 70
    property string url
    property string scriptURL
    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool isDesktop: false
    property alias webView: webview
    property alias profile: webview.profile
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
            text: webview.url
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
            web.destroy();
            return;
        }
        if (parentStackItem) {
            parentStackItem.pop();
        } else {
            web.visible = false;
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
        webview.url = url
        web.url = webview.url;
    }

    onUrlChanged: {
        loadUrl(url);
    }

    WebEngineView {
        id: webview
        objectName: "webEngineView"
        x: 0
        y: 0
        width: parent.width
        height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height - web.headerHeight : parent.height - web.headerHeight
        anchors.top: buttons.bottom
        profile: HFWebEngineProfile;

        property string userScriptUrl: ""

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
            sourceUrl: webview.userScriptUrl
            injectionPoint: WebEngineScript.DocumentReady  // DOM ready but page load may not be finished.
            worldId: WebEngineScript.MainWorld
        }

        property string urlTag: "noDownload=false";
        userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard, userScript ]

        property string newUrl: ""

        Component.onCompleted: {
            webChannel.registerObject("eventBridge", eventBridge);
            webChannel.registerObject("eventBridgeWrapper", eventBridgeWrapper);
            // Ensure the JS from the web-engine makes it to our logging
            webview.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
                console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
            });
        }

        onFeaturePermissionRequested: {
            grantFeaturePermission(securityOrigin, feature, true);
        }

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

        onLoadingChanged: {
            keyboardRaised = false;
            punctuationMode = false;
            keyboard.resetShiftMode(false);
            // Required to support clicking on "hifi://" links
            if (WebEngineView.LoadStartedStatus == loadRequest.status) {
                var url = loadRequest.url.toString();
                if (urlHandler.canHandleUrl(url)) {
                    if (urlHandler.handleUrl(url)) {
                        root.stop();
                    }
                }
            }

            if (WebEngineView.LoadFailedStatus == loadRequest.status) {
                console.log(" Tablet WebEngineView failed to load url: " + loadRequest.url.toString());
            }

            if (WebEngineView.LoadSucceededStatus == loadRequest.status) {
                webview.forceActiveFocus();
            }
        }
        
        onNewViewRequested: {
            request.openIn(webview);
        }

        HiFiControls.WebSpinner { }
    }

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
        web.isDesktop = (typeof desktop !== "undefined");
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
