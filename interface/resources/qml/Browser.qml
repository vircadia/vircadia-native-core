import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebKit 3.0
import "controls"
import "styles"

VrDialog {
    id: root
    HifiConstants { id: hifi }
    title: "Browser"
    resizable: true
    contentImplicitWidth: clientArea.implicitWidth
    contentImplicitHeight: clientArea.implicitHeight
    backgroundColor: "#7f000000"


    Component.onCompleted: {
        enabled = true
        addressBar.text = webview.url
    }

    onParentChanged: {
        if (visible && enabled) {
            addressBar.forceActiveFocus();
            addressBar.selectAll()
        }
    }

    Item {
        id: clientArea
        implicitHeight: 600
        implicitWidth: 800
        x: root.clientX
        y: root.clientY
        width: root.clientWidth
        height: root.clientHeight
        
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: scrollView.top
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

        ScrollView {
            id: scrollView
            anchors.top: buttons.bottom
            anchors.topMargin: 8
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
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
