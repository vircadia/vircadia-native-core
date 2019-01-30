import QtQuick 2.5
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQml 2.2
import QtWebChannel 1.0
import QtWebEngine  1.1


import "."
import stylesUit 1.0
import "../../controls"

FocusScope {
    id: tabletMenu
    objectName: "tabletMenu"

    width: parent.width
    height: parent.height

    property var rootMenu: Menu { objectName:"rootMenu" }
    property var point: Qt.point(50, 50);
    TabletMenuStack { id: menuPopperUpper }
    property string subMenu: ""
    signal sendToScript(var message);

    HifiConstants { id: hifi }

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

        HiFiGlyphs {
            id: menuRootIcon
            text: breadcrumbText.text !== "Menu" ? hifi.glyphs.backward : ""
            size: 72
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            width: breadcrumbText.text === "Menu" ? 32 : 50
            visible: breadcrumbText.text !== "Menu"

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: iconColorOverlay.color = "#1fc6a6";
                onExited: iconColorOverlay.color = "#34a2c7";
                onClicked: {
                    menuPopperUpper.closeLastMenu();
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
            color: "#e3e3e3"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: menuRootIcon.right
            anchors.leftMargin: 15
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
