import QtQuick 2.5
import QtWebChannel 1.0
import QtWebEngine 1.5

import controlsUit 1.0

WebView {
    id: webview
    url: "https://vircadia.com/"
    profile: FileTypeProfile;
    property var parentRoot: null

    // Create a global EventBridge object for raiseAndLowerKeyboard.
    WebEngineScript {
        id: createGlobalEventBridge
        sourceCode: eventBridgeJavaScriptToInject
        injectionPoint: WebEngineScript.Deferred
        worldId: WebEngineScript.MainWorld
    }

    // Detect when may want to raise and lower keyboard.
    WebEngineScript {
        id: raiseAndLowerKeyboard
        injectionPoint: WebEngineScript.Deferred
        sourceUrl: resourceDirectoryUrl + "/html/raiseAndLowerKeyboard.js"
        worldId: WebEngineScript.MainWorld
    }

    userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard ]

    onLoadingChanged: {
        if (loadRequest.status === WebEngineView.LoadSucceededStatus) {
            addressBar.text = loadRequest.url
        }
        parentRoot.loadingChanged(loadRequest.status);
    }

    onWindowCloseRequested: {
        parentRoot.destroy();
    }

    Component.onCompleted: {
        webChannel.registerObject("eventBridge", eventBridge);
        webChannel.registerObject("eventBridgeWrapper", eventBridgeWrapper);
        desktop.initWebviewProfileHandlers(webview.profile);
    }
}
