import QtQuick 2.5
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQml 2.2
import "."

Item {
    id: tabletMenu
    objectName: "tabletMenu"

    width: 480
    height: 720

    property var rootMenu: Menu { objectName:"rootMenu" }
    property var point: Qt.point(50, 50)

    TabletMouseHandler { id: menuPopperUpper }

    Rectangle {
        id: bgNavBar
        height: 90
        z: 1
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

        Image {
            id: menuRootIcon
            width: 40
            height: 40
            source: "../../../icons/tablet-icons/menu-i.svg"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 15

            MouseArea {
                anchors.fill: parent
                // navigate back to top level menu
                onClicked: buildMenu();
            }
        }

        ColorOverlay {
            id: iconColorOverlay
            anchors.fill: menuRootIcon
            source: menuRootIcon
            color: "#ffffff"
        }
    }

    function setRootMenu(menu) {
        tabletMenu.rootMenu = menu
        buildMenu()
    }
    function buildMenu() {
        menuPopperUpper.popup(tabletMenu, rootMenu.items)
    }
}
