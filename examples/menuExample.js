//
//  menuExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/24/14
//  Copyright (c) 2013 HighFidelity, Inc.
//
//  This is an example script that demonstrates use of the Menu object
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


function setupMenus() {
    Menu.addMenu("Foo");
    Menu.addMenuItem("Foo","Foo item 1", "SHIFT+CTRL+F" );
    Menu.addMenuItem("Foo","Foo item 2", "SHIFT+F" );
    Menu.addMenuItem("Foo","Foo item 3", "META+F" );
    Menu.addMenuItem({ 
        menuName: "Foo", 
        menuItemName: "Foo item 4", 
        isCheckable: true, 
        isChecked: true 
    });

    Menu.addMenuItem({
        menuName: "Foo",
        menuItemName: "Foo item 5", 
        shortcutKey: "ALT+F", 
        isCheckable: true
    });


    Menu.addSeparator("Foo","Removable Tools");
    Menu.addMenuItem("Foo","Remove Foo item 4");
    Menu.addMenuItem("Foo","Remove Foo");
    Menu.addMenuItem("Foo","Remove Bar-Spam");
    Menu.addMenu("Bar");

    Menu.addMenuItem("Bar","Bar item 1", "b");
    Menu.addMenuItem({
                        menuName: "Bar",
                        menuItemName: "Bar item 2", 
                        shortcutKeyEvent: { text: "B", isControl: true } 
                    });

    Menu.addMenu("Bar > Spam");
    Menu.addMenuItem("Bar > Spam","Spam item 1");
    Menu.addMenuItem({ 
                        menuName: "Bar > Spam", 
                        menuItemName: "Spam item 2", 
                        isCheckable: true, 
                        isChecked: false 
                    });
                    
    Menu.addSeparator("Bar > Spam","Other Items");
    Menu.addMenuItem("Bar > Spam","Remove Spam item 2");
    Menu.addMenuItem("Foo","Remove Spam item 2");

    Menu.addMenuItem({ 
                        menuName: "Foo",
                        menuItemName: "Remove Spam item 2" 
                     });

    Menu.addMenuItem({ 
                        menuName: "Edit",
                        menuItemName: "before Cut",
                        beforeItem: "Cut"
                     });

    Menu.addMenuItem({ 
                        menuName: "Edit",
                        menuItemName: "after Nudge",
                        afterItem: "Nudge"
                     });

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
    if (menuItem == "Remove Bar-Spam") {
        Menu.removeMenu("Bar > Spam");
    }

    if (menuItem == "Remove Spam item 2") {
        Menu.removeMenuItem("Bar > Spam", "Spam item 2");
    }
}

setupMenus();

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);

Menu.menuItemEvent.connect(menuItemEvent);
