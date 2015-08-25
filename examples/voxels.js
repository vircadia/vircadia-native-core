var controlHeld = false;
var shiftHeld = false;


Script.include([
    "libraries/toolBars.js",
]);


// http://headache.hungry.com/~seth/hifi/voxel-add.svg
// http://headache.hungry.com/~seth/hifi/voxel-add.svg
// http://headache.hungry.com/~seth/hifi/voxel-delete.svg
// http://headache.hungry.com/~seth/hifi/voxel-terrain.svg

var isActive = false;
var toolIconUrl = "http://headache.hungry.com/~seth/hifi/";
var toolHeight = 50;
var toolWidth = 50;

var addingVoxels = false;
var deletingVoxels = false;

offAlpha = 0.5;
onAlpha = 0.9;


function floorVector(v) {
    return {x: Math.floor(v.x), y: Math.floor(v.y), z: Math.floor(v.z)};
}

function vectorToString(v){
    return "{" + v.x + ", " + v.x + ", " + v.x + "}";
}

var toolBar = (function () {
    var that = {},
        toolBar,
        activeButton,
        addVoxelButton,
        deleteVoxelButton,
        addTerrainButton;

    function initialize() {
        toolBar = new ToolBar(0, 0, ToolBar.VERTICAL, "highfidelity.voxel.toolbar", function (windowDimensions, toolbar) {
            return {
                x: windowDimensions.x - 8*2 - toolbar.width * 2,
                y: (windowDimensions.y - toolbar.height) / 2
            };
        });

        activeButton = toolBar.addTool({
            imageURL: "http://s3.amazonaws.com/hifi-public/images/tools/polyvox.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: onAlpha,
            visible: true,
        });

        addVoxelButton = toolBar.addTool({
            imageURL: toolIconUrl + "voxel-add.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: offAlpha,
            visible: false
        });

        deleteVoxelButton = toolBar.addTool({
            imageURL: toolIconUrl + "voxel-delete.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: offAlpha,
            visible: false
        });

        addTerrainButton = toolBar.addTool({
            imageURL: toolIconUrl + "voxel-terrain.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: onAlpha,
            visible: false
        });

        that.setActive(false);
    }


    that.setActive = function(active) {
        if (active != isActive) {
            isActive = active;
            that.showTools(isActive);
        }
        toolBar.selectTool(activeButton, isActive);
    };

    // Sets visibility of tool buttons, excluding the power button
    that.showTools = function(doShow) {
        toolBar.showTool(addVoxelButton, doShow);
        toolBar.showTool(deleteVoxelButton, doShow);
        toolBar.showTool(addTerrainButton, doShow);
    };

    that.mousePressEvent = function (event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (activeButton === toolBar.clicked(clickedOverlay)) {
            that.setActive(!isActive);
            return true;
        }

        if (addVoxelButton === toolBar.clicked(clickedOverlay)) {
            if (addingVoxels) {
                addingVoxels = false;
                deletingVoxels = false;
                toolBar.setAlpha(offAlpha, addVoxelButton);
                toolBar.setAlpha(offAlpha, deleteVoxelButton);
                toolBar.selectTool(addVoxelButton, false);
                toolBar.selectTool(deleteVoxelButton, false);
            } else {
                addingVoxels = true;
                deletingVoxels = false;
                toolBar.setAlpha(onAlpha, addVoxelButton);
                toolBar.setAlpha(offAlpha, deleteVoxelButton);
            }
            return true;
        }

        if (deleteVoxelButton === toolBar.clicked(clickedOverlay)) {
            if (deletingVoxels) {
                deletingVoxels = false;
                addingVoxels = false;
                toolBar.setAlpha(offAlpha, addVoxelButton);
                toolBar.setAlpha(offAlpha, deleteVoxelButton);
            } else {
                deletingVoxels = true;
                addingVoxels = false;
                toolBar.setAlpha(offAlpha, addVoxelButton);
                toolBar.setAlpha(onAlpha, deleteVoxelButton);
            }
            return true;
        }

        if (addTerrainButton === toolBar.clicked(clickedOverlay)) {
            addTerrainBlock();
            return true;
        }
    }

    Window.domainChanged.connect(function() {
        that.setActive(false);
    });

    that.cleanup = function () {
        toolBar.cleanup();
        // Overlays.deleteOverlay(activeButton);
    };


    initialize();
    return that;
}());


function addTerrainBlock() {

    var myPosDiv16 = Vec3.multiply(Vec3.sum(MyAvatar.position, {x:8, x:8, z:8}), 1.0 / 16.0);
    var myPosDiv16Floored = floorVector(myPosDiv16);
    var baseLocation = Vec3.multiply(myPosDiv16Floored, 16.0);

    if (baseLocation.y + 8 > MyAvatar.position.y) {
        baseLocation.y -= 16;
    }

    print("myPosDiv16 is " + vectorToString(myPosDiv16));
    print("MyPosDiv16Floored is " + vectorToString(myPosDiv16Floored));
    print("baseLocation is " + vectorToString(baseLocation));

    alreadyThere = Entities.findEntities(baseLocation, 1.0);
    for (var i = 0; i < alreadyThere.length; i++) {
        var id = alreadyThere[i];
        var properties = Entities.getEntityProperties(id);
        if (properties.name == "terrain") {
            print("already terrain there");
            return;
        }
    }

    var polyVoxId = Entities.addEntity({
        type: "PolyVox",
        name: "terrain",
        position: baseLocation,
        dimensions: { x: 16, y: 16, z: 16 },
        voxelVolumeSize: {x:16, y:16, z:16},
        voxelSurfaceStyle: 3
    });
    Entities.setAllVoxels(polyVoxId, 255);

    for (var y = 8; y < 16; y++) {
        for (var x = 0; x < 16; x++) {
            for (var z = 0; z < 16; z++) {
                Entities.setVoxel(polyVoxId, {x: x, y: y, z: z}, 0);
            }
        }
    }

    // for (var x = 1; x <= 14; x++) {
    //     Entities.setVoxel(polyVoxId, {x: x, y: 1, z: 1}, 255);
    //     Entities.setVoxel(polyVoxId, {x: x, y: 14, z: 1}, 255);
    //     Entities.setVoxel(polyVoxId, {x: x, y: 1, z: 14}, 255);
    //     Entities.setVoxel(polyVoxId, {x: x, y: 14, z: 14}, 255);
    // }
    // for (var y = 2; y <= 13; y++) {
    //     Entities.setVoxel(polyVoxId, {x: 1, y: y, z: 1}, 255);
    //     Entities.setVoxel(polyVoxId, {x: 14, y: y, z: 1}, 255);
    //     Entities.setVoxel(polyVoxId, {x: 1, y: y, z: 14}, 255);
    //     Entities.setVoxel(polyVoxId, {x: 14, y: y, z: 14}, 255);
    // }
    // for (var z = 2; z <= 13; z++) {
    //     Entities.setVoxel(polyVoxId, {x: 1, y: 1, z: z}, 255);
    //     Entities.setVoxel(polyVoxId, {x: 14, y: 1, z: z}, 255);
    //     Entities.setVoxel(polyVoxId, {x: 1, y: 14, z: z}, 255);
    //     Entities.setVoxel(polyVoxId, {x: 14, y: 14, z: z}, 255);
    // }

    return true;
}


function attemptVoxelChange(pickRayDir, intersection) {

    var properties = Entities.getEntityProperties(intersection.entityID);
    if (properties.type != "PolyVox") {
        return false;
    }

    if (addingVoxels == false && deletingVoxels == false) {
        return false;
    }

    var voxelPosition = Entities.worldCoordsToVoxelCoords(intersection.entityID, intersection.intersection);
    voxelPosition = Vec3.subtract(voxelPosition, {x: 0.5, y: 0.5, z: 0.5});
    var pickRayDirInVoxelSpace = Entities.localCoordsToVoxelCoords(intersection.entityID, pickRayDir);
    pickRayDirInVoxelSpace = Vec3.normalize(pickRayDirInVoxelSpace);

    var doAdd = addingVoxels;
    var doDelete = deletingVoxels;

    if (controlHeld) {
        doAdd = deletingVoxels;
        doDelete = addingVoxels;
    }

    // } else if (shiftHeld) {
    //     // return Entities.setAllVoxels(intersection.entityID, 255);
    // }

    // Entities.setVoxelSphere(id, intersection.intersection, radius, 0)

    if (doDelete) {
        var toErasePosition = Vec3.sum(voxelPosition, Vec3.multiply(pickRayDirInVoxelSpace, 0.1));
        return Entities.setVoxel(intersection.entityID, floorVector(toErasePosition), 0);
    }
    if (doAdd) {
        var toDrawPosition = Vec3.subtract(voxelPosition, Vec3.multiply(pickRayDirInVoxelSpace, 0.1));
        return Entities.setVoxel(intersection.entityID, floorVector(toDrawPosition), 255);
    }
}

function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }

    if (toolBar.mousePressEvent(event)) {
        return;
    }

    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Entities.findRayIntersection(pickRay, true); // accurate picking

    if (intersection.intersects) {
        if (attemptVoxelChange(pickRay.direction, intersection)) {
            return;
        }
    }

    // if the PolyVox entity is empty, we can't pick against its "on" voxels.  try picking against its
    // bounding box, instead.
    intersection = Entities.findRayIntersection(pickRay, false); // bounding box picking
    if (intersection.intersects) {
        attemptVoxelChange(pickRay.direction, intersection);
    }
}


function keyPressEvent(event) {
    if (event.text == "CONTROL") {
        controlHeld = true;
    }
    if (event.text == "SHIFT") {
        shiftHeld = true;
    }
}


function keyReleaseEvent(event) {
    if (event.text == "CONTROL") {
        controlHeld = false;
    }
    if (event.text == "SHIFT") {
        shiftHeld = false;
    }
}


function cleanup() {
    for (var i = 0; i < overlays.length; i++) {
        Overlays.deleteOverlay(overlays[i]);
    }
    toolBar.cleanup();
}


Controller.mousePressEvent.connect(mousePressEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.scriptEnding.connect(cleanup);
