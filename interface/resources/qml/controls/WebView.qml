import QtQuick 2.5
import QtWebEngine 1.1

WebEngineView {
    id: root
    property var newUrl;

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
        onTriggered: url = newUrl;
    }

    onUrlChanged: {
        console.log("Url changed to " + url);
        var originalUrl = url.toString();
        newUrl = urlHandler.fixupUrl(originalUrl).toString();
        if (newUrl !== originalUrl) {
            root.stop();
            if (urlReplacementTimer.running) {
                console.warn("Replacement timer already running");
                return;
            }
            urlReplacementTimer.start();
        }
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

    profile: WebEngineProfile {
        id: webviewProfile
        httpUserAgent: "Mozilla/5.0 (HighFidelityInterface)"
        storageName: "qmlWebEngine"
    }
}
