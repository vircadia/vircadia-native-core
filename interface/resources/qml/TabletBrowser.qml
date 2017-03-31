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
    width: parent.width
    height: 600
    property variant permissionsBar: {'securityOrigin':'none','feature':'none'}
    //property alias url: webview.url
    property WebEngineView webView: webview
    property alias eventBridge: eventBridgeWrapper.eventBridge
    property bool canGoBack: webview.canGoBack
    property bool canGoForward: webview.canGoForward
    property var goForward: webview.goForward();
    property var goBack: webview.goBack();


    signal loadingChanged(int status)

    x: 0
    y: 0

    Component.onCompleted: {
        webView.address = webview.url
    }

    function showPermissionsBar(){
        //permissionsContainer.visible=true;
    }

    function hidePermissionsBar(){
      //permissionsContainer.visible=false;
    }

    function allowPermissions(){
        webview.grantFeaturePermission(permissionsBar.securityOrigin, permissionsBar.feature, true);
        hidePermissionsBar();
    }

    function setAutoAdd(auto) {
       // desktop.setAutoAdd(auto);
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

              Item {
            id: border
            height: 48
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.left: buttons.right
            anchors.leftMargin: 8
    
            Item {
                id: barIcon
                width: parent.height
                height: parent.height
                Image {
                    //source: webview.icon;
                    x: (parent.height - height) / 2
                    y: (parent.width - width) / 2
                    sourceSize: Qt.size(width, height);
                    verticalAlignment: Image.AlignVCenter;
                    horizontalAlignment: Image.AlignHCenter
                    onSourceChanged: console.log("Icon url: " + source)
                }
            }

            TextField {
                id: addressBar
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.left: barIcon.right
                anchors.leftMargin: 0
                anchors.verticalCenter: parent.verticalCenter
                focus: true
                //colorScheme: hifi.colorSchemes.dark
                placeholderText: "Enter URL"
                Component.onCompleted: ScriptDiscoveryService.scriptsModelFilter.filterRegExp = new RegExp("^.*$", "i")
                Keys.onPressed: {
                    switch(event.key) {
                        case Qt.Key_Enter:
                        case Qt.Key_Return:
                            event.accepted = true
                            if (text.indexOf("http") != 0) {
                                text = "http://" + text;
                            }
                            root.hidePermissionsBar();
                            root.keyboardRaised = false;
                            webview.url = text;
                            break;
                    }
                }
            }
        }

        Rectangle {
            id:permissionsContainer
            visible:false
            color: "#000000"
            width:  parent.width
            anchors.top: buttons.bottom
            height:40
            z:100
            gradient: Gradient {
                GradientStop { position: 0.0; color: "black" }
                GradientStop { position: 1.0; color: "grey" }
            }

            RalewayLight {
                    id: permissionsInfo
                    anchors.right:permissionsRow.left
                    anchors.rightMargin: 32
                    anchors.topMargin:8
                    anchors.top:parent.top
                    text: "This site wants to use your microphone/camera"
                    size: 18
                    color: hifi.colors.white
            }
         
            Row {
                id: permissionsRow
                spacing: 4
                anchors.top:parent.top
                anchors.topMargin: 8
                anchors.right: parent.right
                visible: true
                z:101
                
                Button {
                    id:allow
                    text: "Allow"
                    //color: hifi.buttons.blue
                    //colorScheme: root.colorScheme
                    width: 120
                    enabled: true
                    onClicked: root.allowPermissions(); 
                    z:101
                }

                Button {
                    id:block
                    text: "Block"
                    //color: hifi.buttons.red
                    //colorScheme: root.colorScheme
                    width: 120
                    enabled: true
                    onClicked: root.hidePermissionsBar();
                    z:101
                }
            }
        }
        
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
                    console.log("Web Entity JS message: " + sourceID + " " + lineNumber + " " +  message);
                });
                
                webview.profile.httpUserAgent = "Mozilla/5.0 Chrome (HighFidelityInterface";
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
                    stackRoot.push(newWindow);
                    newWindow.forceActiveFocus();
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
