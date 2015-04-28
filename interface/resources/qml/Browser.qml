import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebKit 3.0
import "controls"
import "styles"

Dialog {
    id: root
    HifiConstants { id: hifi }
    title: "Browser"
    resizable: true
    contentImplicitWidth: clientArea.implicitWidth
    contentImplicitHeight: clientArea.implicitHeight


    Component.onCompleted: {
        enabled = true
        addressBar.text = webview.url
        
    }

    onParentChanged: {
        if (visible && enabled) {
            forceActiveFocus();
        }
    }

    Item {
        id: clientArea
        implicitHeight: 400
        implicitWidth: 600
        x: root.clientX
        y: root.clientY
        width: root.clientWidth
        height: root.clientHeight
        
        Row {
            id: buttons
            spacing: 4
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: 8
            FontAwesome { 
                id: back; size: 48; enabled: webview.canGoBack; text: "\uf0a8"
                MouseArea { anchors.fill: parent;  onClicked: webview.goBack() }
            }
            FontAwesome { 
                id: forward; size: 48; enabled: webview.canGoForward; text: "\uf0a9" 
                MouseArea { anchors.fill: parent;  onClicked: webview.goBack() }
            }
            FontAwesome { 
                id: reload; size: 48; text: webview.loading ? "\uf057" : "\uf021" 
                MouseArea { anchors.fill: parent;  onClicked: webview.loading ? webview.stop() : webview.reload() }
            }
        }
        Border {
            id: border1
            height: 48
            radius: 8
            color: "gray"
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
                    verticalAlignment: Image.AlignVCenter;
                    horizontalAlignment: Image.AlignHCenter
                    onSourceChanged: console.log("Icon url: " + source)
                }
            }
    
            TextInput {
                id: addressBar
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.left: barIcon.right
                anchors.leftMargin: 0
                anchors.verticalCenter: parent.verticalCenter
                onAccepted: {
                    if (text.indexOf("http") != 0) {
                        text = "http://" + text;
                    }
                    webview.url = text
                }
            }
    
        }
    
        ScrollView {
            anchors.top: buttons.bottom
            anchors.topMargin: 8
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            Rectangle { anchors.fill: parent; color: "#7500ff00" }
            WebView {
                id: webview
                url: "http://highfidelity.com"
                anchors.fill: parent
                onLoadingChanged: {
                    if (loadRequest.status == WebView.LoadSucceededStarted) {
                        addressBar.text = loadRequest.url
                    }
                }
                onIconChanged: {
                    barIcon.source = icon
                }
            }
        }
    } // item
} // dialog
