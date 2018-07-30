import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../styles-uit"
import "../../controls-uit" as HifiControlsUit
import "../../controls" as HifiControls

Rectangle {
    id: root

    color: 'white'
    visible: false;

    signal scaleChanged(real scale);

    property alias onSaveClicked: dialogButtons.onYesClicked
    property alias onCancelClicked: dialogButtons.onNoClicked

    property real scaleValue: scaleSlider.value / 10
    property alias dominantHandIsLeft: leftHandRadioButton.checked
    property alias avatarCollisionsOn: collisionsEnabledRadiobutton.checked
    property alias avatarAnimationOverrideJSON: avatarAnimationUrlInputText.text
    property alias avatarAnimationJSON: avatarAnimationUrlInputText.placeholderText
    property alias avatarCollisionSoundUrl: avatarCollisionSoundUrlInputText.text

    property real avatarScaleBackup;
    function open(settings, avatarScale) {
        console.debug('Settings.qml: open: ', JSON.stringify(settings, 0, 4));
        avatarScaleBackup = avatarScale;

        scaleSlider.notify = false;
        scaleSlider.value = Math.round(avatarScale * 10);
        scaleSlider.notify = true;;

        if(settings.dominantHand === 'left') {
            leftHandRadioButton.checked = true;
        } else {
            rightHandRadioButton.checked = true;
        }

        if(settings.collisionsEnabled) {
            collisionsEnabledRadiobutton.checked = true;
        } else {
            collisionsDisabledRadioButton.checked = true;
        }

        avatarAnimationJSON = settings.animGraphUrl;
        avatarAnimationOverrideJSON = settings.animGraphOverrideUrl;
        avatarCollisionSoundUrl = settings.collisionSoundUrl;

        visible = true;
    }

    function close() {
        visible = false
    }

    // This object is always used in a popup.
    // This MouseArea is used to prevent a user from being
    //     able to click on a button/mouseArea underneath the popup.
    MouseArea {
        anchors.fill: parent;
        propagateComposedEvents: false;
        hoverEnabled: true;
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

            // TextStyle9
            RalewaySemiBold {
                size: 17;
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

                HifiControlsUit.Slider {
                    id: scaleSlider
                    property bool notify: false;

                    from: 1
                    to: 40

                    onValueChanged: {
                        console.debug('value changed: ', value);
                        if(notify) {
                            console.debug('notifying.. ');
                            root.scaleChanged(value / 10);
                        }
                    }

                    anchors.verticalCenter: parent.verticalCenter
                    Layout.fillWidth: true

                    // TextStyle9
                    RalewaySemiBold {
                        size: 17;
                        anchors.left: scaleSlider.left
                        anchors.leftMargin: 5
                        anchors.top: scaleSlider.bottom
                        anchors.topMargin: 2
                        text: String(scaleSlider.from / 10) + 'x'
                    }

                    // TextStyle9
                    RalewaySemiBold {
                        size: 17;
                        anchors.right: scaleSlider.right
                        anchors.rightMargin: 5
                        anchors.top: scaleSlider.bottom
                        anchors.topMargin: 2
                        text: String(scaleSlider.to / 10) + 'x'
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
                width: 37
                height: 28
                AvatarAppStyle {
                    id: style
                }

                gradient: Gradient {
                    GradientStop { position: 0.0; color: style.colors.blueHighlight }
                    GradientStop { position: 1.0; color: style.colors.blueAccent }
                }

                radius: 3

                RalewaySemiBold {
                    color: 'white'
                    anchors.centerIn: parent
                    text: "1x"
                    size: 18
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        scaleSlider.value = 10
                    }
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

            // TextStyle9
            RalewaySemiBold {
                size: 17;
                Layout.row: 0
                Layout.column: 0

                text: "Dominant Hand"
            }

            ButtonGroup {
                id: leftRight
            }

            HifiControlsUit.RadioButton {
                id: leftHandRadioButton

                Layout.row: 0
                Layout.column: 1
                Layout.leftMargin: -40

                ButtonGroup.group: leftRight
                checked: true

                colorScheme: hifi.colorSchemes.light
                fontSize: 17
                letterSpacing: 1.4
                text: "Left"
                boxSize: 20
            }

            HifiControlsUit.RadioButton {
                id: rightHandRadioButton

                Layout.row: 0
                Layout.column: 2
                Layout.rightMargin: 20

                ButtonGroup.group: leftRight

                colorScheme: hifi.colorSchemes.light
                fontSize: 17
                letterSpacing: 1.4
                text: "Right"
                boxSize: 20
            }

            // TextStyle9
            RalewaySemiBold {
                size: 17;
                Layout.row: 1
                Layout.column: 0

                text: "Avatar Collisions"
            }

            ButtonGroup {
                id: onOff
            }

            HifiControlsUit.RadioButton {
                id: collisionsEnabledRadiobutton

                Layout.row: 1
                Layout.column: 1
                Layout.leftMargin: -40
                ButtonGroup.group: onOff

                colorScheme: hifi.colorSchemes.light
                fontSize: 17
                letterSpacing: 1.4
                checked: true

                text: "ON"
                boxSize: 20
            }

            HifiConstants {
                id: hifi
            }

            HifiControlsUit.RadioButton {
                id: collisionsDisabledRadioButton

                Layout.row: 1
                Layout.column: 2
                Layout.rightMargin: 20

                ButtonGroup.group: onOff
                colorScheme: hifi.colorSchemes.light
                fontSize: 17
                letterSpacing: 1.4

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

            // TextStyle9
            RalewaySemiBold {
                size: 17;
                text: "Avatar Animation JSON"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
            }

            InputTextStyle4 {
                id: avatarAnimationUrlInputText
                font.pixelSize: 17
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

            // TextStyle9
            RalewaySemiBold {
                size: 17;
                text: "Avatar collision sound URL (optional)"
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
            }

            InputTextStyle4 {
                id: avatarCollisionSoundUrlInputText
                font.pixelSize: 17
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
