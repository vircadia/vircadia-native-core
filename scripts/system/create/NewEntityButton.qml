import QtQuick 2.0
import QtGraphicalEffects 1.0
import TabletScriptingInterface 1.0

Item {
    id: newEntityButton
    property var uuid;
    property string text: "ENTITY"
    property string icon: "icons/edit-icon.svg"
    property string activeText: newEntityButton.text
    property string activeIcon: newEntityButton.icon
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

    Rectangle {
        id: buttonOutline
        color: "#00000000"
        opacity: 0
        radius: 8
        z: 1
        border.width: 2
        border.color: "#ffffff"
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }

    DropShadow {
        id: glow
        visible: false
        anchors.fill: parent
        horizontalOffset: 0
        verticalOffset: 0
        color: "#ffffff"
        radius: 20
        z: -1
        samples: 41
        source: buttonOutline
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
        source: newEntityButton.urlHelper(newEntityButton.icon)
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
        text: newEntityButton.text
        font.bold: true
        font.pixelSize: 16
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        enabled: true
        onClicked: {
            Tablet.playSound(TabletEnums.ButtonClick);
            newEntityButton.clicked();
        }
        onEntered: {
            Tablet.playSound(TabletEnums.ButtonHover);
            newEntityButton.state = "hover state";
        }
        onExited: {
            newEntityButton.state = "base state";
        }
    }

    states: [
        State {
            name: "hover state"

            PropertyChanges {
                target: buttonOutline
                opacity: 1
            }

            PropertyChanges {
                target: glow
                visible: true
            }
        },
        State {
            name: "base state"

            PropertyChanges {
                target: glow
                visible: false
            }
        }
    ]
}


