//
//  localVoxelsExample.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var TREE_SCALE = 16384;
var tree = LocalVoxels("tree");
tree.setVoxel(0, 0, 0,
              0.5 * TREE_SCALE,
              255, 0, 0);
tree.setVoxel(0.5 * TREE_SCALE,
              0.5 * TREE_SCALE,
              0.5 * TREE_SCALE,
              0.5 * TREE_SCALE,
              0, 255, 0);

var copy = LocalVoxels("copy");
tree.pasteFrom(0, 0, 0, TREE_SCALE, "copy");
tree.pasteFrom(0, 0, 0, TREE_SCALE, "clipboard");
    
var overlay1 = Overlays.addOverlay("localvoxels", {
                                   position: {x: 1, y: 1, z: 1},
                                   size: 1,
                                   name: "tree"
                                   });
var overlay2 = Overlays.addOverlay("localvoxels", {
                                   position: {x: 1, y: 2, z: 1},
                                   size: 1,
                                   name: "tree"
                                   });
var overlay3 = Overlays.addOverlay("localvoxels", {
                                   position: {x: 1, y: 3, z: 1},
                                   size: 1,
                                   name: "tree"
                                   });
var overlay4 = Overlays.addOverlay("localvoxels", {
                                   position: {x: 1, y: 4, z: 1},
                                   size: 1,
                                   name: "copy"
                                   });

var clipboard = Overlays.addOverlay("localvoxels", {
                                   position: {x: 1, y: 5, z: 1},
                                   size: 1,
                                   name: "clipboard"
                                   });



// When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    Overlays.deleteOverlay(overlay1);
    Overlays.deleteOverlay(overlay2);
    Overlays.deleteOverlay(overlay3);
    Overlays.deleteOverlay(overlay4);
    Overlays.deleteOverlay(clipboard);
}
Script.scriptEnding.connect(scriptEnding);