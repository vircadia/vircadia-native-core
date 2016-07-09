import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebEngine 1.1

import "controls-uit"
import "styles" as HifiStyles
import "styles-uit"
import "windows"

ScrollingWindow {
    id: root
    HifiConstants { id: hifi }
    HifiStyles.HifiConstants { id: hifistyles }
    title: "Browser"
    resizable: true
    destroyOnHidden: true
    width: 800
    height: 600
    property alias webView: webview
    x: 100
    y: 100

    Component.onCompleted: {
        shown = true
        addressBar.text = webview.url
    }

    onParentChanged: {
        if (visible) {
            addressBar.forceActiveFocus();
            addressBar.selectAll()
        }
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
                color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
                size: 48
                MouseArea { anchors.fill: parent;  onClicked: webview.goBack() }
            }

            HiFiGlyphs {
                id: forward;
                enabled: webview.canGoForward;
                text: hifi.glyphs.forward
                color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
                size: 48
                MouseArea { anchors.fill: parent;  onClicked: webview.goForward() }
            }

            HiFiGlyphs {
                id: reload;
                enabled: webview.canGoForward;
                text: webview.loading ? hifi.glyphs.close : hifi.glyphs.reload
                color: enabled ? hifistyles.colors.text : hifistyles.colors.disabledText
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
                colorScheme: hifi.colorSchemes.dark
                placeholderText: "Enter URL"
                Component.onCompleted: ScriptDiscoveryService.scriptsModelFilter.filterRegExp = new RegExp("^.*$", "i")
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
                if (loadRequest.status === WebEngineView.LoadSucceededStatus) {
                    addressBar.text = loadRequest.url
                }
            }
            onIconChanged: {
                console.log("New icon: " + icon)
            }
            
            //profile: desktop.browserProfile
    
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
