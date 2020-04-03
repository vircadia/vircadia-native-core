import QtQuick 2.5
import QtWebEngine 1.1
import QtWebChannel 1.0

import controlsUit 1.0 as Controls
import stylesUit 1.0
Controls.WebView {
    id: webview
    url: "about:blank"
    anchors.fill: parent
    focus: true
    profile: HFWebEngineProfile;

    property string userScriptUrl: ""

    // Create a global EventBridge object for raiseAndLowerKeyboard.
    WebEngineScript {
        id: createGlobalEventBridge
        sourceCode: eventBridgeJavaScriptToInject
        injectionPoint: WebEngineScript.DocumentCreation
        worldId: WebEngineScript.MainWorld
    }

    // Detect when may want to raise and lower keyboard.
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

    function onWebEventReceived(event) {
        if (typeof event === "string" && event.slice(0, 17) === "CLARA.IO DOWNLOAD") {
            ApplicationInterface.addAssetToWorldFromURL(event.slice(18));
        }
    }

    Component.onCompleted: {
        webChannel.registerObject("eventBridge", eventBridge);
        webChannel.registerObject("eventBridgeWrapper", eventBridgeWrapper);
        eventBridge.webEventReceived.connect(onWebEventReceived);
    }
}
