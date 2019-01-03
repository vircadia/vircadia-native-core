import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

import TabletScriptingInterface 1.0

Item {
    id: root

    readonly property bool pressed: mouseArea.state == "pressed"
    readonly property bool hovered: mouseArea.state == "hovering"

    signal clicked()

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        hoverEnabled: true

        onClicked: {
            root.focus = true
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
                StateChangeScript {
                    script: {
                        mouseArea.lastState = mouseArea.state
                    }
                }
            },
            State {
                name: "hovering"
                when: mouseArea.containsMouse
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