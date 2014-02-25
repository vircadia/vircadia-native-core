//
//  menuExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/24/14
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Menu object
//


function setupMenus() {
    Menu.addMenu("Foo");
    Menu.addMenuItem("Foo","Foo item 1", "SHIFT+CTRL+F" );
    Menu.addMenuItem("Foo","Foo item 2", "SHIFT+F" );
    Menu.addMenuItem("Foo","Foo item 3", "META+F" );
    Menu.addCheckableMenuItem("Foo","Foo item 4", "ALT+F", true);
    Menu.addSeparator("Foo","Removable Tools");
    Menu.addMenuItem("Foo","Remove Foo item 4");
    Menu.addMenuItem("Foo","Remove Foo");
    Menu.addMenu("Bar");
    Menu.addMenuItemWithKeyEvent("Bar","Bar item 1", { text: "b" } );
    Menu.addMenuItemWithKeyEvent("Bar","Bar item 2", { text: "B", isControl: true } );
    Menu.addMenu("Bar > Spam");
    Menu.addMenuItem("Bar > Spam","Spam item 1");
    Menu.addCheckableMenuItem("Bar > Spam","Spam item 2",false);
    Menu.addSeparator("Bar > Spam","Other Items");
    Menu.addMenuItem("Bar > Spam","Remove Spam item 2");
    Menu.addMenuItem("Foo","Remove Spam item 2");
}

function scriptEnding() {
    print("SCRIPT ENDNG!!!\n");

    Menu.removeMenu("Foo");
    Menu.removeMenu("Bar");
}

function menuItemEvent(menuItem) {
    print("menuItemEvent() in JS... menuItem=" + menuItem);
    if (menuItem == "Foo item 4") {
        print("  checked=" + Menu.isOptionChecked("Foo item 4"));
    }
    if (menuItem == "Remove Foo item 4") {
        Menu.removeMenuItem("Foo", "Foo item 4");
    }
    if (menuItem == "Remove Foo") {
        Menu.removeMenu("Foo");
    }
    if (menuItem == "Remove Spam item 2") {
        Menu.removeMenuItem("Bar > Spam", "Spam item 2");
    }
}

setupMenus();

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);

Menu.menuItemEvent.connect(menuItemEvent);
