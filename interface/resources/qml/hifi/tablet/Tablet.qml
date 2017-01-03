import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: tablet
    objectName: "tablet"

    property double micLevel: 0.8

    width: parent.width
    height: parent.height

    // called by C++ code to keep mic level display bar UI updated
    function setMicLevel(newMicLevel) {
        tablet.micLevel = newMicLevel;
    }

    // used to look up a button by its uuid
    function findButtonIndex(uuid) {
        if (!uuid) {
            return -1;
        }

        for (var i in flowMain.children) {
            var child = flowMain.children[i];
            if (child.uuid === uuid) {
                return i;
            }
        }
        return -1;
    }

    // called by C++ code when a button should be added to the tablet
    function addButtonProxy(properties) {
        var component = Qt.createComponent("TabletButton.qml");
        var button = component.createObject(flowMain);

        // copy all properites to button
        var keys = Object.keys(properties).forEach(function (key) {
            button[key] = properties[key];
        });

        return button;
    }

    // called by C++ code when a button should be removed from the tablet
    function removeButtonProxy(properties) {
        var index = findButtonIndex(properties.uuid);
        if (index < 0) {
            console.log("Warning: Tablet.qml could not find button with uuid = " + properties.uuid);
        } else {
            flowMain.children[index].destroy();
        }
    }

    Rectangle {
        id: bgTopBar
        height: 90
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
            }

            GradientStop {
                position: 1
                color: "#1e1e1e"
            }
        }
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.topMargin: 0
        anchors.top: parent.top

        Row {
            id: rowAudio1
            height: parent.height
            anchors.topMargin: 0
            anchors.top: parent.top
            anchors.leftMargin: 30
            anchors.left: parent.left
            anchors.rightMargin: 30
            anchors.right: parent.right
            spacing: 5

            Image {
                id: muteIcon
                width: 40
                height: 40
                source: "../../../icons/tablet-mute-icon.svg"
                anchors.verticalCenter: parent.verticalCenter
            }

            Item {
                id: item1
                width: 225
                height: 10
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    id: audioBarBase
                    color: "#333333"
                    radius: 5
                    anchors.fill: parent
                }
                Rectangle {
                    id: audioBarMask
                    width: parent.width * tablet.micLevel
                    color: "#333333"
                    radius: 5
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 0
                    anchors.top: parent.top
                    anchors.topMargin: 0
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                }
                LinearGradient {
                    anchors.fill: audioBarMask
                    source: audioBarMask
                    start: Qt.point(0, 0)
                    end: Qt.point(225, 0)
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: "#00b8ff"
                        }
                        GradientStop {
                            position: 1
                            color: "#ff2d73"
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: bgMain
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"

            }

            GradientStop {
                position: 1
                color: "#0f212e"
            }
        }
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: bgTopBar.bottom
        anchors.topMargin: 0

        Flow {
            id: flowMain
            spacing: 16
            anchors.right: parent.right
            anchors.rightMargin: 30
            anchors.left: parent.left
            anchors.leftMargin: 30
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 30
            anchors.top: parent.top
            anchors.topMargin: 30
        }

        Component.onCompleted: {
            console.log("Tablet.onCompleted!");
            var component = Qt.createComponent("TabletButton.qml");
            var buttons = [];
            for (var i = 0; i < 6; i++) {
                var button = component.createObject(flowMain);
                button.inDebugMode = true;
                buttons.push(button);
            }

            // set button text
            buttons[0].text = "MUTE";
            buttons[1].text = "VR";
            buttons[2].text = "MENU";
            buttons[3].text = "BUBBLE";
            buttons[4].text = "SNAP";
            buttons[5].text = "HELP";

            // set button icon
            buttons[0].icon = "icons/tablet-mute-icon.svg"
        }
    }
    states: [
        State {
            name: "muted state"

            PropertyChanges {
                target: muteText
                text: "UNMUTE"
            }

            PropertyChanges {
                target: muteIcon
                source: "../../../icons/tablet-unmute-icon.svg"
            }

            PropertyChanges {
                target: tablet
                micLevel: 0
            }
        }
    ]
}

