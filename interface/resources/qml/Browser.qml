import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebEngine 1.1

import "controls"
import "styles"
import "windows"

Window {
    id: root
    HifiConstants { id: hifi }
    title: "Browser"
    resizable: true
    destroyOnInvisible: true
    width: 800
    height: 600

    Component.onCompleted: {
        visible = true
        addressBar.text = webview.url
    }

    onParentChanged: {
        if (visible) {
            addressBar.forceActiveFocus();
            addressBar.selectAll()
        }
    }

    Item {
        anchors.fill: parent
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: webview.top
            color: "white"
        }
    
        Row {
            id: buttons
            spacing: 4
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: 8
            FontAwesome { 
                id: back; text: "\uf0a8"; size: 48; enabled: webview.canGoBack; 
                color: enabled ? hifi.colors.text : hifi.colors.disabledText
                MouseArea { anchors.fill: parent;  onClicked: webview.goBack() }
            }
            FontAwesome { 
                id: forward; text: "\uf0a9"; size: 48; enabled: webview.canGoForward;  
                color: enabled ? hifi.colors.text : hifi.colors.disabledText
                MouseArea { anchors.fill: parent;  onClicked: webview.goBack() }
            }
            FontAwesome { 
                id: reload; size: 48; text: webview.loading ? "\uf057" : "\uf021" 
                MouseArea { anchors.fill: parent;  onClicked: webview.loading ? webview.stop() : webview.reload() }
            }
        }

        Border {
            height: 48
            radius: 8
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

                Keys.onPressed: {
                    switch(event.key) {
                        case Qt.Key_Enter:
                        case Qt.Key_Return:
                            event.accepted = true
                            if (text.indexOf("http") != 0) {
                                text = "http://" + text
                            }
                            webview.url = text
                            break;
                    }
                }
            }
        }

        WebEngineView {
            id: webview
            url: "http://highfidelity.com"
            anchors.top: buttons.bottom
            anchors.topMargin: 8
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            onLoadingChanged: {
                if (loadRequest.status == WebEngineView.LoadSucceededStatus) {
                    addressBar.text = loadRequest.url
                }
            }
            onIconChanged: {
                console.log("New icon: " + icon)
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
