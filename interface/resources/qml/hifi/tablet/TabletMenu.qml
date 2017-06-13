import QtQuick 2.5
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQml 2.2
import QtWebChannel 1.0
import QtWebEngine  1.1


import "."
import "../../styles-uit"
import "../../controls"

FocusScope {
    id: tabletMenu
    objectName: "tabletMenu"

    width: 480
    height: 720

    property var rootMenu: Menu { objectName:"rootMenu" }
    property var point: Qt.point(50, 50);
    TabletMenuStack { id: menuPopperUpper }
    property string subMenu: ""
    signal sendToScript(var message);

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
                hoverEnabled: true
                onEntered: iconColorOverlay.color = "#1fc6a6";
                onExited: iconColorOverlay.color = "#34a2c7";
                // navigate back to root level menu
                onClicked: {
                    buildMenu();
                    breadcrumbText.text = "Menu";
                    tabletRoot.playButtonClickSound();
                }
            }
        }

        ColorOverlay {
            id: iconColorOverlay
            anchors.fill: menuRootIcon
            source: menuRootIcon
            color: "#34a2c7"
        }

        RalewayBold {
            id: breadcrumbText
            text: "Menu"
            size: 26
            color: "#34a2c7"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: menuRootIcon.right
            anchors.leftMargin: 15
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: breadcrumbText.color = "#1fc6a6";
                onExited: breadcrumbText.color = "#34a2c7";
                // navigate back to parent level menu if there is one
                onClicked: { 
                    if (breadcrumbText.text !== "Menu") {
                        menuPopperUpper.closeLastMenu();
                    }
                    tabletRoot.playButtonClickSound();
                }
            }
        }
    }

    function pop() {
        menuPopperUpper.closeLastMenu();
    }


    function setRootMenu(rootMenu, subMenu) {
        tabletMenu.subMenu = subMenu;
        tabletMenu.rootMenu = rootMenu;
        buildMenu()
    }

    function buildMenu() {
        // Build submenu if specified.
        if (subMenu !== "") {
            var index = 0;
            var found = false;
            while (!found && index < rootMenu.items.length) {
                found = rootMenu.items[index].title === subMenu;
                if (!found) {
                    index += 1;
                }
            }
            subMenu = "";  // Continue with full menu after initially displaying submenu.
            if (found) {
                menuPopperUpper.popup(rootMenu.items[index].items);
                return;
            }
        }

        // Otherwise build whole menu.
        menuPopperUpper.popup(rootMenu.items);
    }
}
