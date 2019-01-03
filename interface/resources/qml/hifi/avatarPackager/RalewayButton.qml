import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

import TabletScriptingInterface 1.0

RalewaySemiBold {
    id: root

    text: "no text"

    signal clicked()

    color: "white"

    property var hoverColor: "#AFAFAF"
    property var pressedColor: "#575757"

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        hoverEnabled: true

        onClicked: {
            Tablet.playSound(TabletEnums.ButtonClick);
            root.clicked()
        }

        property string lastState: ""

        states: [
            State {
                name: ""
                StateChangeScript {
                    script: {
                        mouseArea.lastState = mouseArea.state
                    }
                }
            },
            State {
                name: "pressed"
                when: mouseArea.containsMouse && mouseArea.pressed
                PropertyChanges { target: root; color: pressedColor }
                StateChangeScript {
                    script: {
                        mouseArea.lastState = mouseArea.state
                    }
                }
            },
            State {
                name: "hovering"
                when: mouseArea.containsMouse
                PropertyChanges { target: root; color: hoverColor }
                StateChangeScript {
                    script: {
                        if (mouseArea.lastState == "") {
                            Tablet.playSound(TabletEnums.ButtonHover);
                        }
                        mouseArea.lastState = mouseArea.state
                    }
                }
            }
        ]
    }
}