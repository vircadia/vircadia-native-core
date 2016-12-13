import QtQuick 2.0

Item {
    id: tablet

    width: 480
    height: 720

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
            spacing: 10
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
                    }
                    onExited: {
                        console.log("Mute Botton Unhovered!");
                        parent.color = "#464646";
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
            Rectangle {
                id: audioBarBase
                color: "#333333"
                radius: 5
                width: 225
                height: 10
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    id: audioBarColor
                    radius: 5
                    anchors.verticalCenterOffset: 0
                    anchors.left: parent.left
                    anchors.leftMargin: 85
                    anchors.verticalCenter: parent.verticalCenter
                    rotation: -90
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
                    width: parent.height
                    height: parent.width*0.8
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
            console.log("AJT: Tablet.onCompleted!");
            // AJT: test flow by adding buttons
            var component = Qt.createComponent("TabletButton.qml");
            var buttons = [];
            for (var i = 0; i < 9; i++) {
                buttons.push(component.createObject(flowMain));
            }

            var green = "#1fc6a6";
            var lightblue = "#1DB5EC";
            buttons[3].color = lightblue;
            buttons[4].color = lightblue;
            buttons[5].color = lightblue;
            buttons[6].color = green;
            buttons[7].color = green;
            buttons[8].color = green;
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
        }
    ]
}

