import QtQuick 2.5
import QtWebEngine 1.1
import QtWebChannel 1.0
import "../controls-uit" as HiFiControls

Item {
    property alias url: root.url
    property alias eventBridge: eventBridgeWrapper.eventBridge
    property bool keyboardEnabled: true  // FIXME - Keyboard HMD only: Default to false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    // FIXME - Keyboard HMD only: Make Interface either set keyboardRaised property directly in OffscreenQmlSurface
    // or provide HMDinfo object to QML in RenderableWebEntityItem and do the following.
    /*
    onKeyboardRaisedChanged: {
        keyboardEnabled = HMDinfo.active;
    }
    */

    QtObject {
        id: eventBridgeWrapper
        WebChannel.id: "eventBridgeWrapper"
        property var eventBridge;
    }

    WebEngineView {
        id: root
        x: 0
        y: 0
        width: parent.width
        height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height : parent.height

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

        userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard ]

        property string newUrl: ""

        webChannel.registeredObjects: [eventBridgeWrapper]

        Component.onCompleted: {
            // Ensure the JS from the web-engine makes it to our logging
            root.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
                console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
            });

            root.profile.httpUserAgent = "Mozilla/5.0 Chrome (HighFidelityInterface)";
        }

        // FIXME hack to get the URL with the auth token included.  Remove when we move to Qt 5.6
        Timer {
            id: urlReplacementTimer
            running: false
            repeat: false
            interval: 50
            onTriggered: url = root.newUrl;
        }

        onUrlChanged: {
            var originalUrl = url.toString();
            root.newUrl = urlHandler.fixupUrl(originalUrl).toString();
            if (root.newUrl !== originalUrl) {
                root.stop();
                if (urlReplacementTimer.running) {
                    console.warn("Replacement timer already running");
                    return;
                }
                urlReplacementTimer.start();
            }
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
                var url = loadRequest.url.toString();
                if (urlHandler.canHandleUrl(url)) {
                    if (urlHandler.handleUrl(url)) {
                        root.stop();
                    }
                }
            }
        }

        onNewViewRequested:{
            // desktop is not defined for web-entities
            if (desktop) {
                var component = Qt.createComponent("../Browser.qml");
                var newWindow = component.createObject(desktop);
                request.openIn(newWindow.webView);
            }
        }
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

}
