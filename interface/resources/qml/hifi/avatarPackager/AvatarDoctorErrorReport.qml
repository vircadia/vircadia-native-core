import QtQuick 2.0

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Item {
    id: errorReport

    visible: false

    property alias errors: errorRepeater.model

    property var footer: Item {
        anchors.fill: parent
        anchors.rightMargin: 17
        HifiControls.Button {
            id: tryAgainButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: continueButton.left
            anchors.rightMargin: 22
            height: 40
            width: 134
            text: qsTr("Try Again")
            onClicked: {
                avatarPackager.state = AvatarPackagerState.avatarDoctorDiagnose;
            }
        }

        HifiControls.Button {
            id: continueButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            height: 40
            width: 133
            text: qsTr("Continue")
            color: hifi.buttons.blue
            colorScheme: root.colorScheme
            onClicked: {
                avatarPackager.state = AvatarPackagerState.project;
            }
        }
    }

    HiFiGlyphs {
        id: errorReportIcon
        text: hifi.glyphs.alert
        size: 315
        color: "#EA4C5F"
        anchors {
            top: parent.top
            topMargin: -20
            horizontalCenter: parent.horizontalCenter
        }
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: errorReportIcon.bottom
            topMargin: -40
            leftMargin: 13
            rightMargin: 13
        }
        spacing: 7

        Repeater {
            id: errorRepeater
            Item {
                height: 37
                width: parent.width

                HiFiGlyphs {
                    id: errorIcon
                    text: hifi.glyphs.alert
                    size: 56
                    color: "#EA4C5F"
                    anchors {
                        top: parent.top
                        left: parent.left
                        leftMargin: -5
                    }
                }

                RalewayRegular {
                    id: errorLink
                    anchors {
                        top: parent.top
                        topMargin: 5
                        left: errorIcon.right
                        right: parent.right
                    }
                    color: "#00B4EF"
                    linkColor: "#00B4EF"
                    size: 28
                    text: "<a href='javascript:void'>" + modelData.message + "</a>"
                    onLinkActivated: Qt.openUrlExternally(modelData.url)
                    elide: Text.ElideRight
                }
            }
        }
    }
}
