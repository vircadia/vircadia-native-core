import Hifi 1.0 as Hifi
import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"


Hifi.VrMenu {
    id: root
    HifiConstants { id: hifi }

    anchors.fill: parent

    objectName: "VrMenu"
    enabled: false
    opacity: 0.0
    z: 10000

    property int animationDuration: 200
    property var models: []
    property var columns: []


    onEnabledChanged: {
        if (enabled && columns.length == 0) {
            pushColumn(rootMenu.items);
        }
        opacity = enabled ? 1.0 : 0.0
        if (enabled) {
            forceActiveFocus()
        }
    }

    // The actual animator
    Behavior on opacity {
        NumberAnimation {
            duration: root.animationDuration
            easing.type: Easing.InOutBounce
        }
    }

    onOpacityChanged: {
        visible = (opacity != 0.0);
    }

    onVisibleChanged: {
        if (!visible) reset();
    }

    property var menuBuilder: Component {
        Border {
            HifiConstants { id: hifi }
            property int menuDepth

            Component.onCompleted: {
                menuDepth = root.models.length - 1
                if (menuDepth == 0) {
                    x = lastMousePosition.x - 20
                    y = lastMousePosition.y - 20
                } else {
                    var lastColumn = root.columns[menuDepth - 1]
                    x = lastColumn.x + 64;
                    y = lastMousePosition.y - height / 2;
                }
            }

            border.color: hifi.colors.hifiBlue
            color: hifi.colors.window
            implicitHeight: listView.implicitHeight + 16
            implicitWidth: listView.implicitWidth + 16

            Column {
                id: listView
                property real minWidth: 0
                anchors {
                    top: parent.top
                    topMargin: 8
                    left: parent.left
                    leftMargin: 8
                    right: parent.right
                    rightMargin: 8
                }

                Repeater {
                    model: root.models[menuDepth]
                    delegate: Loader {
                        id: loader
                        source: "VrMenuItem.qml"
                        Binding {
                            target: loader.item
                            property: "menuContainer"
                            value: root
                            when: loader.status == Loader.Ready
                        }
                        Binding {
                            target: loader.item
                            property: "source"
                            value: modelData
                            when: loader.status == Loader.Ready
                        }
                        Binding {
                            target: loader.item
                            property: "listView"
                            value: listView
                            when: loader.status == Loader.Ready
                        }
                    }
                }
            }
        }
    }

    function lastColumn() {
        return columns[root.columns.length - 1];
    }

    function pushColumn(items) {
        models.push(items)
        if (columns.length) {
            var oldColumn = lastColumn();
            //oldColumn.enabled = false
        }
        var newColumn = menuBuilder.createObject(root);
        columns.push(newColumn);
        newColumn.forceActiveFocus();
    }

    function popColumn() {
        if (columns.length > 0) {
            var curColumn = columns.pop();
            curColumn.visible = false;
            curColumn.destroy();
            models.pop();
        }

        if (columns.length == 0) {
            enabled = false;
            return;
        }

        curColumn = lastColumn();
        curColumn.enabled = true;
        curColumn.opacity = 1.0;
        curColumn.forceActiveFocus();
    }

    function selectItem(source) {
        switch (source.type) {
        case 2:
            pushColumn(source.items)
            break;
        case 1:
            source.trigger()
            enabled = false
            break;
        case 0:
            break;
        }
    }

    function reset() {
        while (columns.length > 0) {
            popColumn();
        }
    }

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Escape:
            root.popColumn()
            event.accepted = true;
        }
    }

    MouseArea {
        anchors.fill: parent
        id: mouseArea
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button == Qt.RightButton) {
                root.popColumn();
            } else {
                root.enabled = false;
            }
        }
    }

    function addMenu(menu, newMenu) {
        return menu.addMenu(newMenu);
    }

    function addItem(menu, newMenuItem) {
        return menu.addItem(newMenuItem);
    }

    function insertItem(menu, beforeItem, newMenuItem) {
        for (var i = 0; i < menu.items.length; ++i) {
            if (menu.items[i] === beforeItem) {
                return menu.insertItem(i, newMenuItem);
            }
        }
        return addItem(menu, newMenuItem);
    }

    function removeItem(menu, menuItem) {
        menu.removeItem(menuItem);
    }
}
