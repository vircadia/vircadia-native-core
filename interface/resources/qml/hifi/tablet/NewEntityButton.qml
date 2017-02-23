import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: tabletButton
    property var uuid;
    property string text: "EDIT"
    property string icon: "icons/edit-icon.svg"
    property string activeText: tabletButton.text
    property string activeIcon: tabletButton.icon
    property bool isActive: false
    property bool inDebugMode: false
    property bool isEntered: false
    property double sortOrder: 100
    property int stableOrder: 0
    property var tabletRoot;
    width: 100
    height: 100

    signal clicked()

    function changeProperty(key, value) {
        tabletButton[key] = value;
    }

    onIsActiveChanged: {
        if (tabletButton.isEntered) {
            tabletButton.state = (tabletButton.isActive) ? "hover active state" : "hover sate";
        } else {
            tabletButton.state = (tabletButton.isActive) ? "active state" : "base sate";
        }
    }

    Rectangle {
        id: buttonBg
        color: "#1c1c1c"
        opacity: 1
        radius: 8
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }

    function urlHelper(src) {
        if (src.match(/\bhttp/)) {
            return src;
        } else {
            return "../../../" + src;
        }
    }

    Image {
        id: icon
        width: 50
        height: 50
        visible: false
        anchors.bottom: text.top
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        fillMode: Image.Stretch
        source: tabletButton.urlHelper(tabletButton.icon)
    }

    ColorOverlay {
        id: iconColorOverlay
        anchors.fill: icon
        source: icon
        color: "#ffffff"
    }

    Text {
        id: text
        color: "#ffffff"
        text: tabletButton.text
        font.bold: true
        font.pixelSize: 18
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        enabled: true
        onClicked: {
            console.log("Tablet Button Clicked!");
            if (tabletButton.inDebugMode) {
                if (tabletButton.isActive) {
                    tabletButton.isActive = false;
                } else {
                    tabletButton.isActive = true;
                }
            }
            tabletButton.clicked();
            if (tabletRoot) {
                tabletRoot.playButtonClickSound();
            }
        }
        onEntered: {
            tabletButton.isEntered = true;
            if (tabletButton.isActive) {
                tabletButton.state = "hover active state";
            } else {
                tabletButton.state = "hover state";
            }
        }
        onExited: {
            tabletButton.isEntered = false;
            if (tabletButton.isActive) {
                tabletButton.state = "active state";
            } else {
                tabletButton.state = "base state";
            }
        }
    }

    states: [
        State {
            name: "hover state"

            PropertyChanges {
                target: buttonOutline
                border.color: "#1fc6a6"
                opacity: 1
            }

            PropertyChanges {
                target: glow
                visible: true
            }
        },
        State {
            name: "active state"

            PropertyChanges {
                target: buttonOutline
                border.color: "#1fc6a6"
                opacity: 1
            }

            PropertyChanges {
                target: buttonBg
                color: "#1fc6a6"
                opacity: 1
            }

            PropertyChanges {
                target: text
                color: "#333333"
                text: tabletButton.activeText
            }

            PropertyChanges {
                target: iconColorOverlay
                color: "#333333"
            }

            PropertyChanges {
                target: icon
                source: tabletButton.urlHelper(tabletButton.activeIcon)
            }
        },
        State {
            name: "hover active state"

            PropertyChanges {
                target: glow
                visible: true
            }

            PropertyChanges {
                target: buttonOutline
                border.color: "#ffffff"
                opacity: 1
            }

            PropertyChanges {
                target: buttonBg
                color: "#1fc6a6"
                opacity: 1
            }

            PropertyChanges {
                target: text
                color: "#333333"
            }

            PropertyChanges {
                target: iconColorOverlay
                color: "#333333"
            }

        }
    ]
}


