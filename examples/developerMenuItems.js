//
//  developerMenuItems.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/24/14
//  Copyright 2013 High Fidelity, Inc.
//
//  Adds a bunch of developer and debugging menu items
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


function setupMenus() {
    if (!Menu.menuExists("Developer")) {
        Menu.addMenu("Developer");
    }
    if (!Menu.menuExists("Developer > Entities")) {
        Menu.addMenu("Developer > Entities");
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Display Model Bounds", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Display Model Triangles", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Display Model Element Bounds", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Display Model Element Children", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Don't Do Precision Picking", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Don't Attempt to Reduce Material Switches", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Don't Attempt Render Entities as Scene", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Don't Do Precision Picking", isCheckable: true, isChecked: false });
        Menu.addMenu("Developer > Entities > Culling");
        Menu.addMenuItem({ menuName: "Developer > Entities > Culling", menuItemName: "Don't Cull Out Of View Mesh Parts", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities > Culling", menuItemName: "Don't Cull Too Small Mesh Parts", isCheckable: true, isChecked: false });
        Menu.addMenuItem({ menuName: "Developer > Entities", menuItemName: "Disable Light Entities", isCheckable: true, isChecked: false });
    }
}

function scriptEnding() {
    Menu.removeMenu("Developer > Entities");
}
setupMenus();
Script.scriptEnding.connect(scriptEnding);
