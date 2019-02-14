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
            // colorScheme: root.colorScheme
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
            //topMargin: 73
            horizontalCenter: parent.horizontalCenter
        }
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: errorReportIcon.bottom
            topMargin: 27
            leftMargin: 13
            rightMargin: 13
        }
        spacing: 7

        Repeater {
            id: errorRepeater
            /*model: [
                {message: "Rig is not Hifi compatible", url: "http://www.highfidelity.com/"},
                {message: "Bone limit exceeds 256", url: "http://www.highfidelity.com/2"},
                {message: "Unsupported Texture", url: "http://www.highfidelity.com/texture"},
                {message: "Rig is not Hifi compatible", url: "http://www.highfidelity.com/"},
                {message: "Bone limit exceeds 256", url: "http://www.highfidelity.com/2"},
                {message: "Unsupported Texture", url: "http://www.highfidelity.com/texture"}
            ]*/

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
                    }
                }

                RalewayRegular {
                    id: errorLink
                    anchors {
                        top: parent.top
                        left: errorIcon.right
                        right: parent.right
                    }
                    linkColor: "#00B4EF"// style.colors.blueHighlight
                    size: 28
                    text: "<a href='javascript:void'>" + modelData.message + "</a>"
                    onLinkActivated: Qt.openUrlExternally(modelData.url)
                }
            }
        }
    }


}
