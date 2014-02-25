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
    Menu.addMenuItem("Foo","Foo item 1", { text: "F", isControl: true} );
    Menu.addMenuItem("Foo","Foo item 2");
    Menu.addTopMenu("Bar");
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
