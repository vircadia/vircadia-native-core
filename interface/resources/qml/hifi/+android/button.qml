import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3

Item {
    id: button
    property string icon: "icons/edit-icon.svg"
    property string hoverIcon: button.icon
    property string activeIcon: button.icon
    property string activeHoverIcon: button.activeIcon
    property int stableOrder: 0

    property int iconSize: 165
    property string text: "."
    property string hoverText: button.text
    property string activeText: button.text
    property string activeHoverText: button.activeText
    
    property string bgColor: "#ffffff"
    property string hoverBgColor: button.bgColor
    property string activeBgColor: button.bgColor
    property string activeHoverBgColor: button.bgColor

    property real bgOpacity: 0
    property real hoverBgOpacity: 1
    property real activeBgOpacity: 0.5
    property real activeHoverBgOpacity: 1

    property string textColor: "#ffffff"
    property int textSize: 54
    property string hoverTextColor: "#ffffff"
    property string activeTextColor: "#ffffff"
    property string activeHoverTextColor: "#ffffff"
    property string fontFamily: "FiraSans"
    property bool fontBold: false

    property int bottomMargin: 30
    
    property bool isEntered: false
    property double sortOrder: 100

    property bool isActive: false

    signal clicked()
    signal entered()
    signal exited()

    onIsActiveChanged: {
        if (button.isEntered) {
            button.state = (button.isActive) ? "hover active state" : "hover state";
        } else {
            button.state = (button.isActive) ? "active state" : "base state";
        }
    }

    function editProperties(props) {
        for (var prop in props) {
            button[prop] = props[prop];
        }
    }


    width: 300
    height: 300

    Rectangle {
        id: buttonBg
        color: bgColor
        opacity: bgOpacity
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }
    Image {
        id: icon
        width: iconSize
        height: iconSize
        anchors.bottom: text.top
        anchors.bottomMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        fillMode: Image.Stretch
        source: urlHelper(button.icon)
    }
    FontLoader {
        id: firaSans
        source: "../../../fonts/FiraSans-Regular.ttf"
    }
    Text {
        id: text
        color: "#ffffff"
        text: button.text
        font.family: button.fontFamily
        font.bold: button.fontBold
        font.pixelSize: textSize
        anchors.bottom: parent.bottom
        anchors.bottomMargin: bottomMargin
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
    }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        enabled: true
        onClicked: {
            console.log("Bottom bar button clicked!!");
            /*if (tabletButton.inDebugMode) {
                if (tabletButton.isActive) {
                    tabletButton.isActive = false;
                } else {
                    tabletButton.isActive = true;
                }
            }*/
            button.clicked();
            /*if (tabletRoot) {
                tabletRoot.playButtonClickSound();
            }*/
        }
        onPressed: {
            button.isEntered = true;
            button.entered();
            if (button.isActive) {
                button.state = "hover active state";
            } else {
                button.state = "hover state";
            }
        }
        onReleased: {
            button.isEntered = false;
            button.exited()
            if (button.isActive) {
                button.state = "active state";
            } else {
                button.state = "base state";
            }
        }
    }
    states: [
        State {
            name: "hover state"

            PropertyChanges {
                target: buttonBg
                color: button.hoverBgColor
                opacity: button.hoverBgOpacity
            }

            PropertyChanges {
                target: text
                color: button.hoverTextColor
                text: button.hoverText
            }

            PropertyChanges {
                target: icon
                source: urlHelper(button.hoverIcon)
            }
        },
        State {
            name: "active state"

            PropertyChanges {
                target: buttonBg
                color: button.activeBgColor
                opacity: button.activeBgOpacity
            }

            PropertyChanges {
                target: text
                color: button.activeTextColor
                text: button.activeText
            }

            PropertyChanges {
                target: icon
                source: urlHelper(button.activeIcon)
            }
        },
        State {
            name: "hover active state"

            PropertyChanges {
                target: buttonBg
                color: button.activeHoverBgColor
                opacity: button.activeHoverBgOpacity
            }

            PropertyChanges {
                target: text
                color: button.activeHoverTextColor
                text: button.activeHoverText
            }

            PropertyChanges {
                target: icon
                source: urlHelper(button.activeHoverIcon)
            }
        },
        State {
            name: "base state"

            PropertyChanges {
                target: buttonBg
                color: button.bgColor
                opacity: button.bgOpacity
            }

            PropertyChanges {
                target: text
                color: button.textColor
                text: button.text
            }

            PropertyChanges {
                target: icon
                source: urlHelper(button.icon)
            }
        }
    ]
}