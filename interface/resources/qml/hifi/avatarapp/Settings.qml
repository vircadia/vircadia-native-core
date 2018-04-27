import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

Rectangle {
    id: settings

    color: 'white'
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: header.bottom
    anchors.bottom: parent.bottom
    visible: false;
    z: 3

    property alias onSaveClicked: dialogButtons.onYesClicked
    property alias onCancelClicked: dialogButtons.onNoClicked

    function open() {
        visible = true;
    }

    function close() {
        visible = false
    }

    Item {
        anchors.left: parent.left
        anchors.leftMargin: 27
        anchors.top: parent.top
        anchors.topMargin: 25
        anchors.right: parent.right
        anchors.rightMargin: 32
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 57

        RowLayout {
            id: avatarScaleRow
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 17

            RalewaySemiBold {
                size: 14;
                text: "Avatar Scale"
                verticalAlignment: Text.AlignVCenter
                anchors.verticalCenter: parent.verticalCenter
            }

            RowLayout {
                anchors.verticalCenter: parent.verticalCenter
                Layout.fillWidth: true

                spacing: 0

                HiFiGlyphs {
                    size: 30
                    text: 'T'
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }

                Slider {
                    id: slider
                    from: 0
                    to: 100
                    anchors.verticalCenter: parent.verticalCenter
                    Layout.fillWidth: true

                    handle: Rectangle {
                        width: 18
                        height: 18
                        color: 'white'
                        radius: 9
                        border.width: 1
                        border.color: 'black'
                        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                        y: slider.topPadding + slider.availableHeight / 2 - height / 2
                    }
                    background: Rectangle {
                        x: slider.leftPadding
                        y: slider.topPadding + slider.availableHeight / 2 - height / 2
                        implicitWidth: 200
                        implicitHeight: 18
                        width: slider.availableWidth
                        height: implicitHeight
                        radius: 9
                        border.color: 'black'
                        border.width: 1
                        color: '#f2f2f2'
                    }
                }

                HiFiGlyphs {
                    size: 40
                    text: 'T'
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            ShadowRectangle {
                width: 28
                height: 28
                color: 'white'

                radius: 3
                border.color: 'black'
                border.width: 1.5
                anchors.verticalCenter: parent.verticalCenter

                RalewaySemiBold {
                    size: 13;
                    text: "1x"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        GridLayout {
            id: handAndCollisions
            anchors.top: avatarScaleRow.bottom
            anchors.topMargin: 39
            anchors.left: parent.left
            anchors.right: parent.right

            rows: 2
            rowSpacing: 25

            columns: 3

            RalewaySemiBold {
                Layout.row: 0
                Layout.column: 0

                size: 14;
                text: "Dominant Hand"
            }

            ButtonGroup {
                id: leftRight
            }

            HifiControlsUit.RadioButton {
                id: leftHandRadioButton

                Layout.row: 0
                Layout.column: 1
                Layout.leftMargin: -18

                ButtonGroup.group: leftRight
                checked: true

                colorScheme: hifi.colorSchemes.light
                text: "Left hand"
                boxSize: 20
            }

            HifiControlsUit.RadioButton {
                id: rightHandRadioButton

                Layout.row: 0
                Layout.column: 2
                ButtonGroup.group: leftRight

                colorScheme: hifi.colorSchemes.light
                text: "Right hand"
                boxSize: 20
            }

            RalewaySemiBold {
                Layout.row: 1
                Layout.column: 0

                size: 14;
                text: "Avatar Collisions"
            }

            ButtonGroup {
                id: onOff
            }

            HifiControlsUit.RadioButton {
                id: onRadioButton

                Layout.row: 1
                Layout.column: 1
                Layout.leftMargin: -18
                ButtonGroup.group: onOff

                colorScheme: hifi.colorSchemes.light
                checked: true

                text: "ON"
                boxSize: 20
            }

            HifiConstants {
                id: hifi
            }

            HifiControlsUit.RadioButton {
                id: offRadioButton

                Layout.row: 1
                Layout.column: 2
                ButtonGroup.group: onOff
                colorScheme: hifi.colorSchemes.light

                text: "OFF"
                boxSize: 20
            }
        }

        ColumnLayout {
            id: avatarAnimationLayout
            anchors.top: handAndCollisions.bottom
            anchors.topMargin: 25
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 4

            RalewaySemiBold {
                size: 14;
                text: "Avatar Animation JSON"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
            }

            InputTextStyle4 {
                anchors.left: parent.left
                anchors.right: parent.right
                placeholderText: 'user\\ﬁle\\dir'
            }
        }

        ColumnLayout {
            id: avatarCollisionLayout
            anchors.top: avatarAnimationLayout.bottom
            anchors.topMargin: 25
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 4

            RalewaySemiBold {
                size: 14;
                text: "Avatar collision sound URL (optional)"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
            }

            InputTextStyle4 {
                anchors.left: parent.left
                anchors.right: parent.right
                placeholderText: 'https://hifi-public.s3.amazonaws.com/sounds/Collisions-'
            }
        }

        DialogButtons {
            id: dialogButtons
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            yesText: "SAVE"
            noText: "CANCEL"
        }
    }
}
