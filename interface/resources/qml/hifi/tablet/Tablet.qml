import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: tablet
    objectName: "tablet"

    property double miclevel: 0.8

    width: 480
    height: 720

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
        }
        flowMain.children[index].destroy();
    }

    Rectangle {
        id: bgAudio
        height: 90
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#6b6b6b"
            }

            GradientStop {
                position: 1
                color: "#595959"
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

            Rectangle {
                id: muteButton
                width: 128
                height: 40
                color: "#464646"
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: muteText
                    color: "#ffffff"
                    text: "MUTE"
                    z: 1
                    verticalAlignment: Text.AlignVCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 16
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        console.log("Mute Botton Hovered!");
                        parent.color = "#2d2d2d";
                        parent.border.width = 2;
                        parent.border.color = "#ffffff";
                    }
                    onExited: {
                        console.log("Mute Botton Unhovered!");
                        parent.color = "#464646";
                        parent.border.width = 0;
                    }

                    onClicked: {
                        console.log("Mute Button Clicked! current tablet state: " + tablet.state );
                        if (tablet.state === "muted state") {
                            tablet.state = "base state";
                        } else {
                            tablet.state = "muted state";
                        }
                    }
                }
            }

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
                    width: parent.width * tablet.miclevel
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
                color: "#787878"

            }

            GradientStop {
                position: 1
                color: "#000000"
            }
        }
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: bgAudio.bottom
        anchors.topMargin: 0

        Flow {
            id: flowMain
            spacing: 12
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
            for (var i = 0; i < 9; i++) {
                buttons.push(component.createObject(flowMain));
            }

            // set button color
            var green = "#1FC6A6";
            var lightblue = "#00B4EF";
            buttons[3].color = lightblue;
            buttons[4].color = lightblue;
            buttons[5].color = lightblue;
            buttons[6].color = green;
            buttons[7].color = green;
            buttons[8].color = green;

            // set button text
            buttons[0].text = "GO TO";
            buttons[1].text = "BUBBLE";
            buttons[2].text = "MENU";
            buttons[3].text = "MARKET";
            buttons[4].text = "SWITCH";
            buttons[5].text = "PAUSE";
            buttons[6].text = "EDIT";
            buttons[7].text = "SNAP";
            buttons[8].text = "SCRIPTS";
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
                miclevel: 0
            }
        }
    ]
}

