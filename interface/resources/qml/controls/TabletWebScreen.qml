import QtQuick 2.7
import QtWebEngine 1.5
import QtWebChannel 1.0
import "../controls-uit" as HiFiControls

Item {
    id: root
    property alias url: webroot.url
    property alias scriptURL: webroot.userScriptUrl
    property alias canGoBack: webroot.canGoBack;
    property var goBack: webroot.goBack;
    property alias urlTag: webroot.urlTag
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

    property alias viewProfile: webroot.profile

    Flickable {
        id: flick
        width: parent.width
        height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height : parent.height

        WebEngineView {
            id: webroot
            objectName: "webEngineView"
            anchors.fill: parent

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
                sourceUrl: webroot.userScriptUrl
                injectionPoint: WebEngineScript.DocumentReady  // DOM ready but page load may not be finished.
                worldId: WebEngineScript.MainWorld
            }

            property string urlTag: "noDownload=false";

            userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard, userScript ]

            property string newUrl: ""

            //disable popup
            onContextMenuRequested: {
                request.accepted = true;
            }

            onContentsSizeChanged: {
                flick.contentHeight = Math.max(contentsSize.height, flick.height);
                flick.contentWidth = flick.width
            }

            Component.onCompleted: {
                webChannel.registerObject("eventBridge", eventBridge);
                webChannel.registerObject("eventBridgeWrapper", eventBridgeWrapper);
                // Ensure the JS from the web-engine makes it to our logging
                webroot.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
                    console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
                });

                webroot.profile.httpUserAgent = "Mozilla/5.0 Chrome (HighFidelityInterface)";
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
                    flick.contentWidth = 0
                    flick.contentHeight = 0
                    var url = loadRequest.url.toString();
                    url = (url.indexOf("?") >= 0) ? url + urlTag : url + "?" + urlTag;
                    if (urlHandler.canHandleUrl(url)) {
                        if (urlHandler.handleUrl(url)) {
                            webroot.stop();
                        }
                    }
                }
                if (WebEngineView.LoadSucceededStatus == loadRequest.status) {
                    webroot.runJavaScript("document.body.scrollHeight;", function (i_actualPageHeight) {
                        flick.contentHeight = Math.max(i_actualPageHeight, flick.height);
                    })
                    flick.contentWidth = flick.width
                }
            }

            onNewViewRequested:{
                // desktop is not defined for web-entities or tablet
                if (typeof desktop !== "undefined") {
                    desktop.openBrowserWindow(request, profile);
                } else {
                    tabletRoot.openBrowserWindow(request, profile);
                }
            }
        }
    }

    HiFiControls.WebSpinner {
        anchors.centerIn: parent
        webroot: webroot
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
