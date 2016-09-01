import QtQuick 2.5
import QtWebEngine 1.1

Item {
    property alias url: root.url

    WebEngineView {
        id: root

        x: 0
        y: 0
        width: parent.width
        height: parent.height - keyboard1.height

        property string newUrl: ""

        profile.httpUserAgent: "Mozilla/5.0 Chrome (HighFidelityInterface)"

        Component.onCompleted: {
            console.log("Connecting JS messaging to Hifi Logging")
            // Ensure the JS from the web-engine makes it to our logging
            root.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
                console.log("Web Window JS message: " + sourceID + " " + lineNumber + " " +  message);
            });

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
            if (desktop) {
                var component = Qt.createComponent("../Browser.qml");
                var newWindow = component.createObject(desktop);
                request.openIn(newWindow.webView);
            }
        }

        // This breaks the webchannel used for passing messages.  Fixed in Qt 5.6
        // See https://bugreports.qt.io/browse/QTBUG-49521
        //profile: desktop.browserProfile
    }

    Keyboard {
        id: keyboard1
        x: 197
        y: 182
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
    }
}
