import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1
import QtWebChannel 1.0
import "../controls-uit" as HiFiControls
import "../styles" as HifiStyles
import "../styles-uit"
import HFWebEngineProfile 1.0
import HFTabletWebEngineProfile 1.0
import "../"
Item {
    id: web
    width: parent.width
    height: parent.height
    property var parentStackItem: null
    property int headerHeight: 38
    property alias url: root.url
    property string address: url
    property alias scriptURL: root.userScriptUrl
    property alias eventBridge: eventBridgeWrapper.eventBridge
    property bool keyboardEnabled: HMD.active
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool isDesktop: false
    property WebEngineView view: root

    
    Row {
        id: buttons
        HifiConstants { id: hifi }
        HifiStyles.HifiConstants { id: hifistyles }
        height: headerHeight
        spacing: 4
        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.leftMargin: 8
        HiFiGlyphs {
            id: back;
            enabled: true;
            text: hifi.glyphs.backward
            color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
            size: 48
            MouseArea { anchors.fill: parent;  onClicked: stackRoot.goBack() }
        }
        
        HiFiGlyphs {
            id: forward;
            enabled: stackRoot.currentItem.canGoForward;
            text: hifi.glyphs.forward
            color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
            size: 48
            MouseArea { anchors.fill: parent;  onClicked: stackRoot.currentItem.goForward() }
        }
        
        HiFiGlyphs {
            id: reload;
            enabled: true;
            text: webview.loading ? hifi.glyphs.close : hifi.glyphs.reload
            color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
            size: 48
            MouseArea { anchors.fill: parent;  onClicked: stackRoot.currentItem.reloadPage(); }
        }
        
    }

    TextField {
        id: addressBar
        height: 30
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.left: buttons.right
        anchors.leftMargin: 0
        anchors.verticalCenter: buttons.verticalCenter
        focus: true
        text: address
        Component.onCompleted: ScriptDiscoveryService.scriptsModelFilter.filterRegExp = new RegExp("^.*$", "i")

        Keys.onPressed: {
            switch (event.key) {
                case Qt.Key_Enter:
                case Qt.Key_Return: 
                    event.accepted = true;
                    if (text.indexOf("http") != 0) {
                        text = "http://" + text;
                     }
                    //root.hidePermissionsBar();
                    web.keyboardRaised = false;
                    stackRoot.currentItem.gotoPage(text);
                    break;

                
            }
        }
    }

        

    StackView {
        id: stackRoot
        width: parent.width
        height: parent.height - web.headerHeight
        anchors.top: buttons.bottom
        //property var goBack: currentItem.goBack();
        property WebEngineView view: root
        
        initialItem: root;
        
        function goBack() {
            if (depth > 1 ) {
                if (currentItem.canGoBack) {
                    currentItem.goBack();
                } else {
                    stackRoot.pop();
                    currentItem.webView.focus = true;
                    currentItem.webView.forceActiveFocus();
                    web.address = currentItem.webView.url;
                }
            } else {
                if (currentItem.canGoBack) {
                    currentItem.goBack();
                } else if (parentStackItem) {
                    web.parentStackItem.pop();
                }
            }
        }

        QtObject {
            id: eventBridgeWrapper
            WebChannel.id: "eventBridgeWrapper"
            property var eventBridge;
        }

        WebEngineView {
            id: root
            objectName: "webEngineView"
            x: 0
            y: 0
            width: parent.width
            height: keyboardEnabled && keyboardRaised ? (parent.height - keyboard.height) : parent.height
            profile: HFTabletWebEngineProfile {
                id: webviewTabletProfile
                storageName: "qmlTabletWebEngine"
            }
            
            property WebEngineView webView: root
            function reloadPage() {
                root.reload();
            }

            function gotoPage(url) {
                root.url = url;
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
                sourceUrl: root.userScriptUrl
                injectionPoint: WebEngineScript.DocumentReady  // DOM ready but page load may not be finished.
                worldId: WebEngineScript.MainWorld
            }

            userScripts: [ createGlobalEventBridge, raiseAndLowerKeyboard, userScript ]

            property string newUrl: ""

            webChannel.registeredObjects: [eventBridgeWrapper]

            Component.onCompleted: {
                // Ensure the JS from the web-engine makes it to our logging
                root.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
                    console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
                });

                root.profile.httpUserAgent = "Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Mobile Safari/537.36"   

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
                var component = Qt.createComponent("../TabletBrowser.qml");
                if (component.status != Component.Ready) {
                    if (component.status == Component.Error) {
                        console.log("Error: " + component.errorString());
                    }
                    return;
                }
                var newWindow = component.createObject();
                newWindow.setProfile(root.profile);
                request.openIn(newWindow.webView);
                newWindow.eventBridge = web.eventBridge;
                stackRoot.push(newWindow);
            }
        }
        
        HiFiControls.Keyboard {
            id: keyboard
            raised: web.keyboardEnabled && web.keyboardRaised
            numeric: web.punctuationMode
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
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
                    addressBar.selectAll()
                    addressBar.forceActiveFocus()
                }
                break;
        }
    }
}
