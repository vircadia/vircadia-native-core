import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.2
import QtWebChannel 1.0
import HFTabletWebEngineProfile 1.0
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
    property alias address: displayUrl.text //for compatibility
    property string scriptURL
    property alias eventBridge: eventBridgeWrapper.eventBridge
    property bool keyboardEnabled: HMD.active
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool isDesktop: false
    property bool removingPage: false
    property bool loadingPage: false

    
    property int currentPage: -1 // used as a model for repeater
    property alias pagesModel: pagesModel

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
                enabledColor: hifi.colors.baseGray
                enabled: false
                text: "BACK"

                MouseArea {
                    anchors.fill: parent
                    onClicked: goBack()
                    hoverEnabled: true
                    
                }
            }

            TabletWebButton {
                id: close
                enabledColor: hifi.colors.darkGray
                text: "CLOSE"

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
            anchors {
                top: nav.bottom
                horizontalCenter: parent.horizontalCenter;
            }
        }
    }

    ListModel {
        id: pagesModel
        onCountChanged: {
            currentPage = count - 1;
            if (currentPage > 0) {
                back.enabledColor = hifi.colors.darkGray;
            } else {
                back.enabledColor = hifi.colors.baseGray;
            }
        }
    }
        
    function goBack() {
        if (webview.canGoBack && !isUrlLoaded(webview.url)) {
            if (currentPage > 0) {
                removingPage = true;
                pagesModel.remove(currentPage);
            }
            webview.goBack();
        } else if (currentPage > 0) {
            removingPage = true;
            pagesModel.remove(currentPage);
        }
    }


    function closeWebEngine() {
        if (parentStackItem) {
            parentStackItem.pop();
        } else {
            web.visible = false;
        }
    }

    function goForward() {
        if (currentPage < pagesModel.count - 1) {
            currentPage++;
        }
    }

    function gotoPage(url) {
        urlAppend(url)
    }

    function isUrlLoaded(url) {
        return (pagesModel.get(currentPage).webUrl === url);
    }
    
    function reloadPage() {
        view.reloadAndBypassCache()
        view.setActiveFocusOnPress(true);
        view.setEnabled(true);
    }

    function urlAppend(url) {
        if (removingPage) {
            removingPage = false;
            return;
        }
        var lurl = decodeURIComponent(url)
        if (lurl[lurl.length - 1] !== "/") {
            lurl = lurl + "/"
        }
        if (currentPage === -1 || (pagesModel.get(currentPage).webUrl !== lurl && !timer.running)) {
            timer.start();
            pagesModel.append({webUrl: lurl});
        }
    }

    onCurrentPageChanged: {
        if (currentPage >= 0 && currentPage < pagesModel.count) {
            timer.start();
            webview.url = pagesModel.get(currentPage).webUrl;
            web.url = webview.url;
            web.address = webview.url;
        }
    }

    onUrlChanged: {
        gotoPage(url)
    }

    QtObject {
        id: eventBridgeWrapper
        WebChannel.id: "eventBridgeWrapper"
        property var eventBridge;
    }

    Timer {
        id: timer
        interval: 200
        running: false
        repeat: false
        onTriggered: timer.stop();
    }

        


    WebEngineView {
        id: webview
        objectName: "webEngineView"
        x: 0
        y: 0
        width: parent.width
        height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height - web.headerHeight : parent.height - web.headerHeight
        anchors.top: buttons.bottom
        profile: HFTabletWebEngineProfile {
            id: webviewTabletProfile
            storageName: "qmlTabletWebEngine"
        }

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

        userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard, userScript ]

        property string newUrl: ""

        webChannel.registeredObjects: [eventBridgeWrapper]

        Component.onCompleted: {
            // Ensure the JS from the web-engine makes it to our logging
            webview.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
                console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
            });

            webview.profile.httpUserAgent = "Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Mobile Safari/537.36";
            web.address = url;
        }

        onFeaturePermissionRequested: {
            grantFeaturePermission(securityOrigin, feature, true);
        }

        onLoadingChanged: {
            keyboardRaised = false;
            punctuationMode = false;
            keyboard.resetShiftMode(false);
            // Required to support clicking on "hifi://" links
            if (WebEngineView.LoadStartedStatus == loadRequest.status) {
                urlAppend(loadRequest.url.toString());
                loadingPage = true;
                var url = loadRequest.url.toString();
                if (urlHandler.canHandleUrl(url)) {
                    if (urlHandler.handleUrl(url)) {
                        root.stop();
                    }
                }
            }

            if (WebEngineView.LoadFailedStatus == loadRequest.status) {
                console.log(" Tablet WebEngineView failed to laod url: " + loadRequest.url.toString());
            }
        }

        onNewViewRequested: {
            request.openIn(webview);
        }
    }

    HiFiControls.Keyboard {
        id: keyboard
        raised: parent.keyboardEnabled && parent.keyboardRaised

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
    
    Component.onCompleted: {
        web.isDesktop = (typeof desktop !== "undefined");
        address = url;
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
