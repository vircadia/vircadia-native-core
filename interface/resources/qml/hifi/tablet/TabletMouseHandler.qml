//
//  MessageDialog.qml
//
//  Created by Bradley Austin Davis on 18 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "."

Item {
    id: root
    anchors.fill: parent
    objectName: "tabletMenuHandlerItem"

    MouseArea {
        id: menuRoot;
        objectName: "tabletMenuHandlerMouseArea"
        anchors.fill: parent
        enabled: d.topMenu !== null
        onClicked: {
            d.clearMenus();
        }
    }

    QtObject {
        id: d
        property var menuStack: []
        property var topMenu: null;
        property var modelMaker: Component { ListModel { } }
        property var menuViewMaker: Component {
            TabletMenuView {
                id: subMenu
                onSelected: d.handleSelection(subMenu, currentItem, item)
            }
        }
        property var delay: Timer { // No setTimeout in QML.
            property var menuItem: null;
            interval: 0
            repeat: false
            running: false
            function trigger(item) { // Capture item and schedule asynchronous Timer.
                menuItem = item;
                start();
            }
            onTriggered: {
                menuItem.trigger(); // Now trigger the item.
            }
        }

        function toModel(items) {
            var result = modelMaker.createObject(tabletMenu);
            for (var i = 0; i < items.length; ++i) {
                var item = items[i];
                if (!item.visible) continue;
                console.log(item.title)
                switch (item.type) {
                case MenuItemType.Menu:
                    result.append({"name": item.title, "item": item})
                    break;
                case MenuItemType.Item:
                    result.append({"name": item.text, "item": item})
                    break;
                case MenuItemType.Separator:
                    result.append({"name": "", "item": item})
                    break;
                }
            }
            return result;
        }

        function popMenu() {
            if (menuStack.length) {
                menuStack.pop().destroy();
            }
            if (menuStack.length) {
                topMenu = menuStack[menuStack.length - 1];
                topMenu.focus = true;
            } else {
                topMenu = null;
                //offscreenFlags.navigationFocused = false;
                menuRoot.enabled = false;
            }
        }

        function pushMenu(newMenu) {
            menuStack.push(newMenu);
            topMenu = newMenu;
            topMenu.focus = true;
            //offscreenFlags.navigationFocused = true;
        }

        function clearMenus() {
            while (menuStack.length) {
                popMenu()
            }
        }

        function clampMenuPosition(menu) {
            var margins = 0;
            if (menu.x < margins) {
                menu.x = margins
            } else if ((menu.x + menu.width + margins) > root.width) {
                menu.x = root.width - (menu.width + margins);
            }

            if (menu.y < 0) {
                menu.y = margins
            } else if ((menu.y + menu.height + margins) > root.height) {
                menu.y = root.height - (menu.height + margins);
            }
        }

        function buildMenu(items, targetPosition) {
            var model = toModel(items);
            // Menus must be childed to desktop for Z-ordering
            var newMenu = menuViewMaker.createObject(tabletMenu, { model: model, isSubMenu: topMenu !== null });
            pushMenu(newMenu);
            return newMenu;
        }

        function handleSelection(parentMenu, selectedItem, item) {
            while (topMenu && topMenu !== parentMenu) {
                popMenu();
            }

            switch (item.type) {
                case MenuItemType.Menu:
                    var target = Qt.vector2d(topMenu.x, topMenu.y).plus(Qt.vector2d(selectedItem.x + 96, selectedItem.y));
                    buildMenu(item.items, target).objectName = item.title;
                    break;

                case MenuItemType.Item:
                    console.log("Triggering " + item.text)
                    // Don't block waiting for modal dialogs and such that the menu might open.
                    delay.trigger(item);
                    clearMenus();
                    break;
                }
        }

    }

    function popup(parent, items) {
        d.clearMenus();
        menuRoot.enabled = true;
        d.buildMenu(items, point);
    }

    function closeLastMenu() {
        if (d.menuStack.length) {
            d.popMenu();
            return true;
        }
        return false;
    }

}
