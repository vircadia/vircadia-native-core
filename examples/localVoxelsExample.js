
var tree = LocalVoxels("tree");
tree.setVoxel(0, 0, 0, 0.5, 255, 0, 0);
tree.setVoxel(0.5, 0.5, 0.5, 0.5, 0, 255, 0);
    
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
                                   name: "tree"
                                   });


// When our script shuts down, we should clean up all of our overlays
function scriptEnding() {
    Overlays.deleteOverlay(overlay1);
    Overlays.deleteOverlay(overlay2);
    Overlays.deleteOverlay(overlay3);
    Overlays.deleteOverlay(overlay4);
}
Script.scriptEnding.connect(scriptEnding);