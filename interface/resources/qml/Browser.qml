import QtQuick 2.5
import controlsUit 1.0
import stylesUit 1.0
import "windows"
import "."

ScrollingWindow {
    id: root
    HifiConstants { id: hifi }

    title: "Browser"
    resizable: true
    destroyOnHidden: true
    width: 800
    height: 600
    property variant permissionsBar: {'securityOrigin':'none','feature':'none'}
    property alias url: webview.url
    property alias webView: webview

    signal loadingChanged(int status)

    x: 100
    y: 100

    Component.onCompleted: {
        focus = true
        shown = true
        addressBar.text = webview.url
    }

    function setProfile(profile) {
        webview.profile = profile;
    }

    function showPermissionsBar(){
        permissionsContainer.visible=true;
    }

    function hidePermissionsBar(){
      permissionsContainer.visible=false;
    }

    function allowPermissions(){
        webview.grantFeaturePermission(permissionsBar.securityOrigin, permissionsBar.feature, true);
        hidePermissionsBar();
    }

    function setAutoAdd(auto) {
        desktop.setAutoAdd(auto);
    }

    Item {
        id:item
        width: pane.contentWidth
        implicitHeight: pane.scrollHeight

        Row {
            id: buttons
            spacing: 4
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: 8
            HiFiGlyphs {
                id: back;
                enabled: webview.canGoBack;
                text: hifi.glyphs.backward
                color: enabled ? hifi.colors.text : hifi.colors.disabledText
                size: 48
                MouseArea { anchors.fill: parent;  onClicked: webview.goBack() }
            }

            HiFiGlyphs {
                id: forward;
                enabled: webview.canGoForward;
                text: hifi.glyphs.forward
                color: enabled ? hifi.colors.text : hifi.colors.disabledText
                size: 48
                MouseArea { anchors.fill: parent;  onClicked: webview.goForward() }
            }

            HiFiGlyphs {
                id: reload;
                enabled: webview.canGoForward;
                text: webview.loading ? hifi.glyphs.close : hifi.glyphs.reload
                color: enabled ? hifi.colors.text : hifi.colors.disabledText
                size: 48
                MouseArea { anchors.fill: parent;  onClicked: webview.goForward() }
            }

        }

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
                    source: webview.icon;
                    x: (parent.height - height) / 2
                    y: (parent.width - width) / 2
                    sourceSize: Qt.size(width, height);
                    verticalAlignment: Image.AlignVCenter;
                    horizontalAlignment: Image.AlignHCenter
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
                colorScheme: hifi.colorSchemes.dark
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
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme
                    width: 120
                    enabled: true
                    onClicked: root.allowPermissions(); 
                    z:101
                }

                Button {
                    id:block
                    text: "Block"
                    color: hifi.buttons.red
                    colorScheme: root.colorScheme
                    width: 120
                    enabled: true
                    onClicked: root.hidePermissionsBar();
                    z:101
                }
            }
        }

        BrowserWebView {
            id: webview
            parentRoot: root

            anchors.top: buttons.bottom
            anchors.topMargin: 8
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
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
