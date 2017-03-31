import QtQuick 2.5
import QtQuick.Controls 1.2
import QtWebChannel 1.0
import QtWebEngine 1.2

import "controls"
import "styles" as HifiStyles
import "styles-uit"
import "windows"
import HFWebEngineProfile 1.0

Item {
    id: root
    HifiConstants { id: hifi }
    HifiStyles.HifiConstants { id: hifistyles }
    //width: parent.width
    height: 600
    property variant permissionsBar: {'securityOrigin':'none','feature':'none'}
    property alias url: webview.url
    property WebEngineView webView: webview
    property alias eventBridge: eventBridgeWrapper.eventBridge
    property bool canGoBack: webview.canGoBack
    property bool canGoForward: webview.canGoForward


    signal loadingChanged(int status)

    x: 0
    y: 0
    
    function goBack() {
        webview.goBack();
    }
    
    function goForward() {
        webview.goForward();
    }
    
    function gotoPage(url) {
        webview.url = url;
    }

    function setProfile(profile) {
        webview.profile = profile;
    }

    function reloadPage() {
        webview.reloadAndBypassCache();
        webview.setActiveFocusOnPress(true);
        webview.setEnabled(true);
    }

    Item {
        id:item
        width: parent.width
        implicitHeight: parent.height

        
        QtObject {
            id: eventBridgeWrapper
            WebChannel.id: "eventBridgeWrapper"
            property var eventBridge;
        }

        WebEngineView {
            id: webview
            objectName: "webEngineView"
            x: 0
            y: 0
            width: parent.width
            height: keyboardEnabled && keyboardRaised ? parent.height - keyboard.height : parent.height
            
            profile: HFWebEngineProfile {
                id: webviewProfile
                storageName: "qmlWebEngine"
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
                    console.log("TabletBrowser");
                    console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
                });
                
                webview.profile.httpUserAgent = "Mozilla/5.0 Chrome (HighFidelityInterface";
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
                    var url = loadRequest.url.toString();
                    if (urlHandler.canHandleUrl(url)) {
                        if (urlHandler.handleUrl(url)) {
                            root.stop();
                        }
                    }
                }
            }

            onActiveFocusOnPressChanged: {
                console.log("on active focus changed");
                setActiveFocusOnPress(true);
            }

            onNewViewRequested:{
                // desktop is not defined for web-entities
                if (stackRoot.isDesktop) {
                    var component = Qt.createComponent("./Browser.qml");
                    var newWindow = component.createObject(desktop); 
                    request.openIn(newWindow.webView);
                } else {
                    var component = Qt.createComponent("./TabletBrowser.qml");
                    
                    if (component.status != Component.Ready) {
                        if (component.status == Component.Error) {
                            console.log("Error: " + component.errorString());
                        }
                        return;
                    }
                    var newWindow = component.createObject();
                    newWindow.setProfile(webview.profile);
                    request.openIn(newWindow.webView);
                    newWindow.eventBridge = web.eventBridge;
                    stackRoot.push(newWindow);
                    newWindow.webView.forceActiveFocus();
                }
            }
        }
        
    } // item
    
    
    Keys.onPressed: {
        switch(event.key) {
        case Qt.Key_L:
            if (event.modifiers == Qt.ControlModifier) {
                event.accepted = true
                addressBar.selectAll()
                addressBar.forceActiveFocus()
            }
            break;
        }
    }
    
   } // dialog
