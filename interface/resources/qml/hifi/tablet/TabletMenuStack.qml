//
//  MessageDialog.qml
//
//  Created by Dante Ruiz  on 13 Feb 2017
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

    StackView {
        anchors.fill: parent
        id: d
        objectName: "stack"
        initialItem: topMenu

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

        function pushSource(path) {
            d.push(Qt.resolvedUrl(path));
            d.currentItem.sendToScript.connect(tabletMenu.sendToScript);
            d.currentItem.focus = true;
            d.currentItem.forceActiveFocus();
            breadcrumbText.text = d.currentItem.title;
            if (typeof bgNavBar !== "undefined") {
                d.currentItem.y = bgNavBar.height;
                d.currentItem.height -= bgNavBar.height;
            }
        }

        function popSource() {
            console.log("trying to pop page");
            d.pop();
        }

        function toModel(items, newMenu) {
            var result = modelMaker.createObject(tabletMenu);
            var exclusionGroups = {};

            for (var i = 0; i < items.length; ++i) {
                var item = items[i];
                if (!item.visible) continue;
                switch (item.type) {
                case MenuItemType.Menu:
                    result.append({"name": item.title, "item": item})
                    break;
                case MenuItemType.Item:
                    if (item.text !== "Users Online") {
                        result.append({"name": item.text, "item": item})
                    }

                    for(var j = 0; j < tabletMenu.rootMenu.exclusionGroupsByMenuItem.count; ++j)
                    {
                        var entry = tabletMenu.rootMenu.exclusionGroupsByMenuItem.get(j);
                        if(entry.menuItem == item.toString())
                        {
                            var exclusionGroupId = entry.exclusionGroup;
                            console.debug('item exclusionGroupId: ', exclusionGroupId)

                            if(!exclusionGroups[exclusionGroupId])
                            {
                                exclusionGroups[exclusionGroupId] = exclusiveGroupMaker.createObject(newMenu);
                                console.debug('new exclusion group created: ', exclusionGroups[exclusionGroupId])
                            }

                            var exclusionGroup = exclusionGroups[exclusionGroupId];

                            item.exclusiveGroup = exclusionGroup
                            console.debug('item.exclusiveGroup: ', item.exclusiveGroup)                        
                        }
                    }

                    break;
                case MenuItemType.Separator:
                    result.append({"name": "", "item": item})
                    break;
                }
            }
            return result;
        }

        function popMenu() {
            if (d.depth) {
                d.pop();
            }
            if (d.depth) {
                topMenu = d.currentItem;
                topMenu.focus = true;
                topMenu.forceActiveFocus();
                // show current menu level on nav bar
                if (topMenu.objectName === "" || d.depth === 1) {
                    breadcrumbText.text = "Menu";
                } else {
                    breadcrumbText.text = topMenu.objectName;
                }
            } else {
                breadcrumbText.text = "Menu";
                topMenu = null;
            }
        }

        function pushMenu(newMenu) {
            d.push({ item:newMenu, destroyOnPop: true});
            topMenu = newMenu;
            topMenu.focus = true;
            topMenu.forceActiveFocus();
        }

        function clearMenus() {
            topMenu = null
            d.clear()
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

        property Component exclusiveGroupMaker: Component {
            ExclusiveGroup {
            }
        }

        function buildMenu(items) {
            // Menus must be childed to desktop for Z-ordering
            var newMenu = menuViewMaker.createObject(tabletMenu);
            console.debug('newMenu created: ', newMenu)

            var model = toModel(items, newMenu);

            newMenu.model = model;
            newMenu.isSubMenu = topMenu !== null;

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
                    // show current menu level on nav bar
                    breadcrumbText.text = item.title;
                    break;

                case MenuItemType.Item:
                    console.log("Triggering " + item.text)
                    // Don't block waiting for modal dialogs and such that the menu might open.
                    delay.trigger(item);
                    break;
                }
        }

    }

    function popup(items) {
        d.clearMenus();
        d.buildMenu(items);
    }

    function closeLastMenu() {
        if (d.depth > 1) {
            d.popMenu();
            return true;
        }
        return false;
    }

    function previousItem() { d.topMenu.previousItem(); }
    function nextItem() { d.topMenu.nextItem(); }
    function selectCurrentItem() { d.topMenu.selectCurrentItem(); }
    function previousPage() { d.topMenu.previousPage(); }

}
