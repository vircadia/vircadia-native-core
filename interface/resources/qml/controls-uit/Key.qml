import QtQuick 2.0
import TabletScriptingInterface 1.0

Item {
    id: keyItem
    width: 45
    height: 50

    property int contentPadding: 4
    property string glyph: "a"
    property bool toggle: false   // does this button have the toggle behaivor?
    property bool toggled: false  // is this button currently toggled?
    property alias mouseArea: mouseArea1
    property alias fontFamily: letter.font.family;
    property alias fontPixelSize: letter.font.pixelSize
    property alias verticalAlignment: letter.verticalAlignment
    property alias letterAnchors: letter.anchors

    function resetToggledMode(mode) {
        toggled = mode;
        if (toggled) {
            state = "mouseDepressed";
        } else {
            state = "";
        }
    }

    MouseArea {
        id: mouseArea1
        width: 36
        anchors.fill: parent
        hoverEnabled: true

        onCanceled: {
            if (toggled) {
                keyItem.state = "mouseDepressed";
            } else {
                keyItem.state = "";
            }
        }

        onContainsMouseChanged: {
            if (containsMouse) {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
        }

        onClicked: {
            mouse.accepted = true;
            Tablet.playSound(TabletEnums.ButtonClick);

            webEntity.synthesizeKeyPress(glyph);
            webEntity.synthesizeKeyPress(glyph, mirrorText);

            if (toggle) {
                toggled = !toggled;
            }
        }

        onDoubleClicked: {
            mouse.accepted = true;
        }

        property var _HAPTIC_STRENGTH: 0.1;
        property var _HAPTIC_DURATION: 3.0;
        property var leftHand: 0;
        property var rightHand: 1;

        onEntered: {
            keyItem.state = "mouseOver";

            var globalPosition = keyItem.mapToGlobal(mouseArea1.mouseX, mouseArea1.mouseY);
            var pointerID = Web3DOverlay.deviceIdByTouchPoint(globalPosition.x, globalPosition.y);

            if (Pointers.isLeftHand(pointerID)) {
                Controller.triggerHapticPulse(_HAPTIC_STRENGTH, _HAPTIC_DURATION, leftHand);
            } else if (Pointers.isRightHand(pointerID)) {
                Controller.triggerHapticPulse(_HAPTIC_STRENGTH, _HAPTIC_DURATION, rightHand);
            }
        }

        onExited: {
            if (toggled) {
                keyItem.state = "mouseDepressed";
            } else {
                keyItem.state = "";
            }
        }

        onPressed: {
            keyItem.state = "mouseClicked";
            mouse.accepted = true;
        }

        onReleased: {
            if (containsMouse) {
                keyItem.state = "mouseOver";
            } else {
                if (toggled) {
                    keyItem.state = "mouseDepressed";
                } else {
                    keyItem.state = "";
                }
            }
            mouse.accepted = true;
        }
    }

    Rectangle {
        id: roundedRect
        width: 30
        color: "#121212"
        radius: 2
        border.color: "#00000000"
        anchors.fill: parent
        anchors.margins: contentPadding
    }

    Text {
        id: letter
        y: 6
        width: 50
        color: "#ffffff"
        text: glyph
        style: Text.Normal
        font.family: "Tahoma"
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 8
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 28
    }

    states: [
        State {
            name: "mouseOver"

            PropertyChanges {
                target: roundedRect
                color: "#121212"
                radius: 3
                border.width: 2
                border.color: "#00b4ef"
            }

            PropertyChanges {
                target: letter
                color: "#00b4ef"
                style: Text.Normal
            }
        },
        State {
            name: "mouseClicked"
            PropertyChanges {
                target: roundedRect
                color: "#1080b8"
                border.width: 2
                border.color: "#00b4ef"
            }

            PropertyChanges {
                target: letter
                color: "#121212"
                styleColor: "#00000000"
                style: Text.Normal
            }
        },
        State {
            name: "mouseDepressed"
            PropertyChanges {
                target: roundedRect
                color: "#0578b1"
                border.width: 0
            }

            PropertyChanges {
                target: letter
                color: "#121212"
                styleColor: "#00000000"
                style: Text.Normal
            }
        }
    ]
}
