import QtQuick 2.5
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQml 2.2
import "."


Item {
    id: tabletMenu
    objectName: "tabletMenu"

    width: parent.width
    height: parent.height

    property var rootMenu: Menu { objectName:"rootMenu" }
    property var point: Qt.point(50, 50)

    TabletMouseHandler { id: menuPopperUpper }

    function setRootMenu(menu) {
        tabletMenu.rootMenu = menu
        buildMenu()
    }
    function buildMenu() {
        menuPopperUpper.popup(tabletMenu, rootMenu.items)
    }
}
