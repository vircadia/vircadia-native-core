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
    Menu.addTopMenu("Foo");
    Menu.addMenuItem("Foo","Foo item 1", "SHIFT+CTRL+F" );
    Menu.addMenuItem("Foo","Foo item 2", "SHIFT+F" );
    Menu.addMenuItem("Foo","Foo item 3", "META+F" );
    Menu.addMenuItem("Foo","Foo item 4", "ALT+F" );
    Menu.addTopMenu("Bar");
    Menu.addMenuItemWithKeyEvent("Bar","Bar item 1", { text: "b" } );
    Menu.addMenuItemWithKeyEvent("Bar","Bar item 2", { text: "B", isControl: true } );
}

function scriptEnding() {
    print("SCRIPT ENDNG!!!\n");

    Menu.removeTopMenu("Foo");
    Menu.removeTopMenu("Bar");
}

function menuItemEvent(menuItem) {
    print("menuItemEvent() in JS... menuItem=" + menuItem);
}

setupMenus();

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);

Menu.menuItemEvent.connect(menuItemEvent);
