import QtQuick 2.5
import QtWebEngine 1.1

WebEngineView {
    id: root
    property var originalUrl
    property int lastFixupTime: 0

    Component.onCompleted: {
        console.log("Connecting JS messaging to Hifi Logging")
        // Ensure the JS from the web-engine makes it to our logging
        root.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
            console.log("Web Window JS message: " + sourceID + " " + lineNumber + " " +  message);
        });
    }


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
