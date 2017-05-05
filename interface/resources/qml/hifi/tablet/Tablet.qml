import QtQuick 2.5
import QtGraphicalEffects 1.0
import "../../styles-uit"

Item {
    id: tablet
    objectName: "tablet"
    property double micLevel: 0.8
    property bool micEnabled: true
    property int rowIndex: 0
    property int columnIndex: 0
    property int count: (flowMain.children.length - 1)

    // called by C++ code to keep mic state updated
    function setMicEnabled(newMicEnabled) {
        tablet.micEnabled = newMicEnabled;
    }

    // called by C++ code to keep audio bar updated
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

    function sortButtons() {
        var children = [];
        for (var i = 0; i < flowMain.children.length; i++) {
            children[i] = flowMain.children[i];
        }

        children.sort(function (a, b) {
            if (a.sortOrder === b.sortOrder) {
                // subsort by stableOrder, because JS sort is not stable in qml.
                return a.stableOrder - b.stableOrder;
            } else {
                return a.sortOrder - b.sortOrder;
            }
        });

        flowMain.children = children;
    }

    // called by C++ code when a button should be added to the tablet
    function addButtonProxy(properties) {
        var component = Qt.createComponent("TabletButton.qml");
        var button = component.createObject(flowMain);

        // copy all properites to button
        var keys = Object.keys(properties).forEach(function (key) {
            button[key] = properties[key];
        });

        // pass a reference to the tabletRoot object to the button.
        if (tabletRoot) {
            button.tabletRoot = tabletRoot;
        } else {
            button.tabletRoot = parent.parent;
        }

        sortButtons();

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

        Item {
            id: audioIcon
            anchors.verticalCenter: parent.verticalCenter
            width: 40
            height: 40
            anchors.left: parent.left
            anchors.leftMargin: 5

            Image {
                id: micIcon
                source: "../../../icons/tablet-icons/mic.svg"
            }

            Item {
                visible: (!tablet.micEnabled && !toggleMuteMouseArea.containsMouse)
                         || (tablet.micEnabled && toggleMuteMouseArea.containsMouse)

                Image {
                    id: muteIcon
                    source: "../../../icons/tablet-icons/mic-mute.svg"
                }

                ColorOverlay {
                    anchors.fill: muteIcon
                    source: muteIcon
                    color: toggleMuteMouseArea.containsMouse ? "#a0a0a0" : "#ff0000"
                }
            }
        }

        Item {
            id: audioBar
            width: 170
            height: 10
            anchors.left: parent.left
            anchors.leftMargin: 50
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
                end: Qt.point(170, 0)
                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: "#2c8e72"
                    }
                    GradientStop {
                        position: 0.8
                        color: "#1fc6a6"
                    }
                    GradientStop {
                        position: 0.81
                        color: "#ea4c5f"
                    }
                    GradientStop {
                        position: 1
                        color: "#ea4c5f"
                    }
                }
            }
        }

        MouseArea {
            id: toggleMuteMouseArea
            anchors {
                left: audioIcon.left
                right: audioBar.right
                top: audioIcon.top
                bottom: audioIcon.bottom
            }

            hoverEnabled: true
            preventStealing: true
            propagateComposedEvents: false
            scrollGestureEnabled: false
            onClicked: tabletRoot.toggleMicEnabled()
        }

        RalewaySemiBold {
            id: usernameText
            text: tabletRoot.username
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 20
            horizontalAlignment: Text.AlignRight
            font.pixelSize: 20
            color: "#afafaf"
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

        Flickable {
            id: flickable
            width: parent.width
            height: parent.height
            contentWidth: parent.width
            contentHeight: flowMain.childrenRect.height + flowMain.anchors.topMargin + flowMain.anchors.bottomMargin + flowMain.spacing
            clip: true
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
                visible: micEnabled
            }

            PropertyChanges {
                target: tablet
                micLevel: 0
            }
        }
    ]

    function setCurrentItemState(state) {
        var index = rowIndex + columnIndex;

        if (index >= 0 && index <= count ) {
            flowMain.children[index].state = state;
        }
    }

    function nextItem() {
        setCurrentItemState("base state");
        var nextColumnIndex = (columnIndex + 3 + 1) % 3;
        var nextIndex = rowIndex + nextColumnIndex;
        if(nextIndex <= count) {
            columnIndex = nextColumnIndex;
        };
        setCurrentItemState("hover state");
    }

    function previousItem() {
        setCurrentItemState("base state");
        var prevIndex = (columnIndex + 3 - 1) % 3;
        if((rowIndex + prevIndex) <= count){
            columnIndex = prevIndex;
        }
        setCurrentItemState("hover state");
    }

    function upItem() {
        setCurrentItemState("base state");
        rowIndex = rowIndex - 3;
        if (rowIndex < 0 ) {
            rowIndex =  (count - (count % 3));
            var index = rowIndex + columnIndex;
            if(index  > count) {
                rowIndex = rowIndex - 3;
            }
        }
        setCurrentItemState("hover state");
    }

    function downItem() {
        setCurrentItemState("base state");
        rowIndex = rowIndex + 3;
        var index = rowIndex + columnIndex;
        if (index  > count ) {
            rowIndex = 0;
        }
        setCurrentItemState("hover state");
    }

    function selectItem() {
        flowMain.children[rowIndex + columnIndex].clicked();
        if (tabletRoot) {
            tabletRoot.playButtonClickSound();
        }
    }

    Keys.onRightPressed: nextItem();
    Keys.onLeftPressed: previousItem();
    Keys.onDownPressed: downItem();
    Keys.onUpPressed: upItem();
    Keys.onReturnPressed: selectItem();
}
