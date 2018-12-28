import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Rectangle {
    id: root 

    width: parent.width
    height: 74
    color: "#252525"

    property alias title: title.text
    property alias faqEnabled: faq.visible
    property bool backButtonVisible: true // If false, is not visible and does not take up space
    property bool backButtonEnabled: true // If false, is not visible but does not affect space
    property bool canRename: false;
    signal backButtonClicked

    RalewaySemiBold {
        id: back
        visible: backButtonEnabled && backButtonVisible
        size: 28
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: back.verticalCenter
        text: "◀"
        color: "white"
        MouseArea {
            anchors.fill: parent
            onClicked: root.backButtonClicked()
            hoverEnabled: true
            onEntered: { state = "hovering" }
            onExited: { state = "" }
            states: [
                State {
                    name: "hovering"
                    PropertyChanges {
                        target: back
                        color: "gray"
                    }
                }
            ]
        }
    }

    RalewaySemiBold {
        id: title
        size: 28
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: root.backButtonVisible ? back.right : parent.left
        anchors.leftMargin: root.backButtonVisible ? 11 : 21
        anchors.verticalCenter: title.verticalCenter
        text: qsTr("Avatar Packager")
        color: "white"
    }

    RalewaySemiBold {
        id: faq
        visible: false
        size: 28
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: faq.verticalCenter
        text: qsTr("Docs")
        color: "white"
    }
}
