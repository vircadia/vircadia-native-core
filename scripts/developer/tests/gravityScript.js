//
// Gravity Script 1.0
// ************
//
// Created by Cain Kilgore on 9/14/2017

// Javascript for the Gravity Modifier Implementation to test
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


function menuParameters(menuNameSelection, menuItemNameSelection) {
  Menu.addMenuItem({
    menuName: menuNameSelection,
    menuItemName: menuItemNameSelection,
    isCheckable: false
  });
}

function setupMenu() {
    if (!Menu.menuExists("Gravity")) {
        Menu.addMenu("Gravity");
        for (var i = -5; i <= 5; i++) {
            menuParameters("Gravity", i);
        }
    }   
}

function menuItemEvent(menuItem) {
    for (var i = -5; i <= 5; i++) {
        if (menuItem == i) {
            MyAvatar.setGravity(i);
        }
    }
}

function onScriptEnding() {
  Menu.removeMenu("Gravity");
}

setupMenu();
Menu.menuItemEvent.connect(menuItemEvent);
Script.scriptEnding.connect(onScriptEnding);
