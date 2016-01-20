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
        offscreenFlags.navigationFocused = enabled;
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
        VrMenuView {
            property int menuDepth: root.models.length - 1
            model: root.models[menuDepth]
            
            function fit(position, size, maxposition) {
                var padding = 8;
                if (position < padding) {
                    position = padding;
                } else if (position + size + padding > maxposition) {
                    position = maxposition - (size + padding); 
                }
                return position;
            }

            Component.onCompleted: {
                if (menuDepth === 0) {
                    x = lastMousePosition.x - 20
                    y = lastMousePosition.y - 20
                } else {
                    var lastColumn = root.columns[menuDepth - 1]
                    x = lastColumn.x + 64;
                    y = lastMousePosition.y - height / 2;
                }
                x = fit(x, width, parent.width);
                y = fit(y, height, parent.height);
            }

            onSelected: {
                root.selectItem(menuDepth, item)
            }
        }
    }

    function lastColumn() {
        return columns[root.columns.length - 1];
    }

    function pushColumn(items) {
        models.push(itemsToModel(items))
        if (columns.length) {
            var oldColumn = lastColumn();
            //oldColumn.enabled = false
        }
        var newColumn = menuBuilder.createObject(root);
        columns.push(newColumn);
        forceActiveFocus();
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

    function itemsToModel(items) {
        var newListModel = Qt.createQmlObject('import QtQuick 2.2; ListModel {}', root);
        for (var i = 0; i < items.length; ++i) {
            var item = items[i];
            switch (item.type) {
            case 2:
                newListModel.append({"type":item.type, "name": item.title, "item": item})
                break;
            case 1:
                newListModel.append({"type":item.type, "name": item.text, "item": item})
                break;
            case 0:
                newListModel.append({"type":item.type, "name": "-----", "item": item})
                break;
            }
        }
        return newListModel;
    }

    function selectItem(depth, source) {
        var popped = false;
        while (depth + 1 < columns.length) {
            popColumn()
            popped = true
        }

        switch (source.type) {
        case 2:
            lastColumn().enabled = false
            pushColumn(source.items)
            break;
        case 1:
            if (!popped) source.trigger()
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

    function previousItem() {
        if (columns.length) {
            lastColumn().incrementCurrentIndex()
        }
    }

    function nextItem() {
        if (columns.length) {
            lastColumn().decrementCurrentIndex()
        }
    }

    function selectCurrentItem() {
        if (columns.length) {
            var depth = columns.length - 1;
            var index = lastColumn().currentIndex;
            if (index >= 0) {
                var model = models[depth];
                var item = model.get(index).item;
                selectItem(depth, item);
            }
        }
    }

    Keys.onDownPressed: previousItem();
    Keys.onUpPressed: nextItem();
    Keys.onSpacePressed: selectCurrentItem();
    Keys.onReturnPressed: selectCurrentItem();
    Keys.onRightPressed: selectCurrentItem();
    Keys.onLeftPressed: popColumn();
    Keys.onEscapePressed: popColumn();
}
