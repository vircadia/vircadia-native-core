import QtQuick 2.3
import QtQuick.Controls 1.3 as Original
import QtQuick.Controls.Styles 1.3 as OriginalStyles

import "."
import "../styles"

Original.Button {
    id: root
    property color iconColor: "black"
    property real size: 32
    SystemPalette { id: palette; colorGroup: SystemPalette.Active }
    SystemPalette { id: disabledPalette; colorGroup: SystemPalette.Disabled }
    style: OriginalStyles.ButtonStyle {
        label: FontAwesome {
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: control.size - 4
            color: control.enabled ? palette.buttonText : disabledPalette.buttonText
            text: control.text
        }
        background: Rectangle {
            id: background
            implicitWidth: size
            implicitHeight: size
            border.width: 1
            border.color: "black"
            radius: 4
            color: control.enabled ? palette.button : disabledPalette.button

            Connections {
                target: control
                onActiveFocusChanged: {
                    if (control.activeFocus) {
                        pulseAnimation.restart();
                    } else {
                        pulseAnimation.stop();
                    }
                    background.border.width = 1;
                }
            }

            SequentialAnimation {
                id: pulseAnimation;
                running: false
                NumberAnimation { target: border; property: "width"; to: 3; duration: 500 }
                NumberAnimation { target: border; property: "width"; to: 1; duration: 500 }
                onStopped: if (control.activeFocus) { start(); }
            }
        }
    }
    Keys.onEnterPressed: root.clicked();
    Keys.onReturnPressed: root.clicked();
}
