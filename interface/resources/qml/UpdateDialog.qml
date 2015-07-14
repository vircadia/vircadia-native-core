import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import QtGraphicalEffects 1.0
import "controls"
import "styles"

DialogContainer {
    id: root
    HifiConstants { id: hifi }

    objectName: "UpdateDialog"

    implicitWidth: updateDialog.implicitWidth
    implicitHeight: updateDialog.implicitHeight

    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0
    property int maximumX: parent ? parent.width - width : 0
    property int maximumY: parent ? parent.height - height : 0

    UpdateDialog {
        id: updateDialog
        
        implicitWidth: backgroundRectangle.width
        implicitHeight: backgroundRectangle.height
        
        readonly property int contentWidth: 500
        readonly property int logoSize: 60
        readonly property int borderWidth: 30
        readonly property int closeMargin: 16
        readonly property int inputSpacing: 16
        readonly property int buttonWidth: 100
        readonly property int buttonHeight: 30
        readonly property int noticeHeight: 15 * inputSpacing
        readonly property string fontFamily: Qt.platform.os === "windows" ? "Trebuchet MS" : "Trebuchet"

        signal triggerBuildDownload
        signal closeUpdateDialog
        
        Rectangle {
            id: backgroundRectangle
            color: "#ffffff"

            width: updateDialog.contentWidth + updateDialog.borderWidth * 2
            height: mainContent.height + updateDialog.borderWidth * 2 - updateDialog.closeMargin / 2

            MouseArea {
                width: parent.width
                height: parent.height
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                drag {
                    target: root
                    minimumX: 0
                    minimumY: 0
                    maximumX: root.parent ? root.maximumX : 0
                    maximumY: root.parent ? root.maximumY : 0
                }
            }
        }

        Image {
            id: logo
            source: "../images/interface-logo.svg"
            width: updateDialog.logoSize
            height: updateDialog.logoSize
            anchors {
                top: mainContent.top
                right: mainContent.right
            }
        }

        Column {
            id: mainContent
            width: updateDialog.contentWidth
            spacing: updateDialog.inputSpacing
            anchors {
                horizontalCenter: parent.horizontalCenter
                topMargin: updateDialog.borderWidth
                top: parent.top
            }
            
            Rectangle {
                id: header
                width: parent.width - updateDialog.logoSize - updateDialog.inputSpacing
                height: updateAvailable.height + versionDetails.height

                Text {
                    id: updateAvailable
                    text: "Update Available"
                    font {
                        family: updateDialog.fontFamily
                        pixelSize: hifi.fonts.pixelSize * 1.5
                        weight: Font.DemiBold
                    }
                    color: "#303030"
                }

                Text {
                    id: versionDetails
                    text: updateDialog.updateAvailableDetails
                    font {
                        family: updateDialog.fontFamily
                        pixelSize: hifi.fonts.pixelSize * 0.6
                        letterSpacing: -0.5
                    }
                    color: hifi.colors.text
                    anchors {
                        top: updateAvailable.bottom
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: updateDialog.noticeHeight

                border {
                    width: 1
                    color: "#a0a0a0"
                }

                ScrollView {
                    id: scrollArea
                    width: parent.width - updateDialog.closeMargin
                    height: parent.height
                    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
                    verticalScrollBarPolicy: Qt.ScrollBarAsNeeded
                    anchors.right: parent.right

                    Text {
                        id: releaseNotes
                        wrapMode: Text.Wrap
                        width: parent.width - updateDialog.closeMargin
                        text: updateDialog.releaseNotes
                        color: hifi.colors.text
                        font {
                            family: updateDialog.fontFamily
                            pixelSize: hifi.fonts.pixelSize * 0.65
                        }
                    }
                }
            }

            Row {
                anchors.right: parent.right
                spacing: updateDialog.inputSpacing
                height: updateDialog.buttonHeight + updateDialog.closeMargin / 2

                Rectangle {
                    id: cancelButton
                    width: updateDialog.buttonWidth
                    height: updateDialog.buttonHeight
                    anchors.bottom: parent.bottom

                    Text {
                        text: "Cancel"
                        color: "#0c9ab4"  // Same as logo
                        font {
                            family: updateDialog.fontFamily
                            pixelSize: hifi.fonts.pixelSize * 1.2
                            weight: Font.DemiBold
                        }
                        anchors {
                            verticalCenter: parent.verticalCenter
                            horizontalCenter: parent.horizontalCenter
                        }
                    }

                    MouseArea {
                        id: cancelButtonAction
                        anchors.fill: parent
                        onClicked: updateDialog.closeDialog()
                        cursorShape: "PointingHandCursor"
                    }
                }

                Rectangle {
                    id: updateButton
                    width: updateDialog.buttonWidth
                    height: updateDialog.buttonHeight
                    anchors.bottom: parent.bottom

                    Text {
                        text: "Update"
                        color: "#0c9ab4"  // Same as logo
                        font {
                            family: updateDialog.fontFamily
                            pixelSize: hifi.fonts.pixelSize * 1.2
                            weight: Font.DemiBold
                        }
                        anchors {
                            verticalCenter: parent.verticalCenter
                            horizontalCenter: parent.horizontalCenter
                        }
                    }

                    MouseArea {
                        id: updateButtonAction
                        anchors.fill: parent
                        onClicked: updateDialog.triggerUpgrade()
                        cursorShape: "PointingHandCursor"
                    }
                }
            }
        }
    }
}
