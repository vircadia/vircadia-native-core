//
//  clipboardExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Clipboard class
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var selectedVoxel = { x: 0, y: 0, z: 0, s: 0 };
var selectedSize = 4;

function setupMenus() {
    // hook up menus
    Menu.menuItemEvent.connect(menuItemEvent);

    // delete the standard application menu item
    Menu.removeMenuItem("Edit", "Cut");
    Menu.removeMenuItem("Edit", "Copy");
    Menu.removeMenuItem("Edit", "Paste");
    Menu.removeMenuItem("Edit", "Delete");
    Menu.removeMenuItem("Edit", "Nudge");
    Menu.removeMenuItem("Edit", "Replace from File");
    Menu.removeMenuItem("File", "Export Voxels");
    Menu.removeMenuItem("File", "Import Voxels");

    // delete the standard application menu item
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Cut", shortcutKey: "CTRL+X", afterItem: "Voxels" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Copy", shortcutKey: "CTRL+C", afterItem: "Cut" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Paste", shortcutKey: "CTRL+V", afterItem: "Copy" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Nudge", shortcutKey: "CTRL+N", afterItem: "Paste" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Replace from File", shortcutKey: "CTRL+R", afterItem: "Nudge" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Delete", shortcutKeyEvent: { text: "backspace" }, afterItem: "Nudge" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Export Voxels", shortcutKey: "CTRL+E", afterItem: "Voxels" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Import Voxels", shortcutKey: "CTRL+I", afterItem: "Export Voxels" });
}

function menuItemEvent(menuItem) {
    var debug = true;
    if (debug) {
        print("menuItemEvent " + menuItem);
    }
    
    // Note: this sample uses Alt+ as the key codes for these clipboard items
    if (menuItem == "Copy") {
        print("copying...");
        Clipboard.copyVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if (menuItem == "Cut") {
        print("cutting...");
        Clipboard.cutVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if (menuItem == "Paste") {
        print("pasting...");
        Clipboard.pasteVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if (menuItem == "Delete") {
        print("deleting...");
        Clipboard.deleteVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if (menuItem == "Export Voxels") {
        print("export");
        Clipboard.exportVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s);
    }
    if (menuItem == "Import Voxels") {
        print("import");
        Clipboard.importVoxels();
    }
    if (menuItem == "Nudge") {
        print("nudge");
        Clipboard.nudgeVoxel(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z, selectedVoxel.s, { x: -1, y: 0, z: 0 });
    }
    if (menuItem == "Replace from File") {
        var filename = Window.browse("Select file to load replacement", "", "Voxel Files (*.png *.svo *.schematic)");
        if (filename) {
            Clipboard.importVoxel(filename, selectedVoxel);
        }
    }
}

var selectCube = Overlays.addOverlay("cube", {
                    position: { x: 0, y: 0, z: 0},
                    size: selectedSize,
                    color: { red: 255, green: 255, blue: 0},
                    alpha: 1,
                    solid: false,
                    visible: false,
                    lineWidth: 4
                });


function mouseMoveEvent(event) {

    var pickRay = Camera.computePickRay(event.x, event.y);

    var debug = false;
    if (debug) {
        print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
        print("called Camera.computePickRay()");
        print("computePickRay origin=" + pickRay.origin.x + ", " + pickRay.origin.y + ", " + pickRay.origin.z);
        print("computePickRay direction=" + pickRay.direction.x + ", " + pickRay.direction.y + ", " + pickRay.direction.z);
    }

    var intersection = Voxels.findRayIntersection(pickRay);

    if (intersection.intersects) {
        if (debug) {
            print("intersection voxel.red/green/blue=" + intersection.voxel.red + ", " 
                        + intersection.voxel.green + ", " + intersection.voxel.blue);
            print("intersection voxel.x/y/z/s=" + intersection.voxel.x + ", " 
                        + intersection.voxel.y + ", " + intersection.voxel.z+ ": " + intersection.voxel.s);
            print("intersection face=" + intersection.face);
            print("intersection distance=" + intersection.distance);
            print("intersection intersection.x/y/z=" + intersection.intersection.x + ", " 
                        + intersection.intersection.y + ", " + intersection.intersection.z);
        }                    
                    
        
        
        var x = Math.floor(intersection.voxel.x / selectedSize) * selectedSize;
        var y = Math.floor(intersection.voxel.y / selectedSize) * selectedSize;
        var z = Math.floor(intersection.voxel.z / selectedSize) * selectedSize;
        selectedVoxel = { x: x, y: y, z: z, s: selectedSize };
        Overlays.editOverlay(selectCube, { position: selectedVoxel, size: selectedSize, visible: true } );
    } else {
        Overlays.editOverlay(selectCube, { visible: false } );
        selectedVoxel = { x: 0, y: 0, z: 0, s: 0 };
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);

function wheelEvent(event) {
    var debug = false;
    if (debug) {
        print("wheelEvent");
        print("    event.x,y=" + event.x + ", " + event.y);
        print("    event.delta=" + event.delta);
        print("    event.orientation=" + event.orientation);
        print("    event.isLeftButton=" + event.isLeftButton);
        print("    event.isRightButton=" + event.isRightButton);
        print("    event.isMiddleButton=" + event.isMiddleButton);
        print("    event.isShifted=" + event.isShifted);
        print("    event.isControl=" + event.isControl);
        print("    event.isMeta=" + event.isMeta);
        print("    event.isAlt=" + event.isAlt);
    }
}

Controller.wheelEvent.connect(wheelEvent);

function scriptEnding() {
    Overlays.deleteOverlay(selectCube);
}

Script.scriptEnding.connect(scriptEnding);

setupMenus();