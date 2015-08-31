var controlHeld = false;
var shiftHeld = false;

Script.include([
    "libraries/toolBars.js",
]);

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



function getTerrainAlignedLocation(pos) {
    var posDiv16 = Vec3.multiply(pos, 1.0 / 16.0);
    var posDiv16Floored = floorVector(posDiv16);
    return Vec3.multiply(posDiv16Floored, 16.0);
}


function lookupTerrainForLocation(pos) {
    var baseLocation = getTerrainAlignedLocation(pos);
    entitiesAtLoc = Entities.findEntities(baseLocation, 1.0);
    for (var i = 0; i < entitiesAtLoc.length; i++) {
        var id = entitiesAtLoc[i];
        var properties = Entities.getEntityProperties(id);
        if (properties.name == "terrain") {
            return id;
        }
    }

    return false;
}


function addTerrainBlock() {
    var baseLocation = getTerrainAlignedLocation(Vec3.sum(MyAvatar.position, {x:8, y:8, z:8}));
    if (baseLocation.y > MyAvatar.position.y) {
        baseLocation.y -= 16;
    }

    var alreadyThere = lookupTerrainForLocation(baseLocation);
    if (alreadyThere) {
        return;
    }

    var polyVoxId = Entities.addEntity({
        type: "PolyVox",
        name: "terrain",
        position: baseLocation,
        dimensions: { x:16, y:16, z:16 },
        voxelVolumeSize: {x:16, y:32, z:16},
        voxelSurfaceStyle: 0,
        xTextureURL: "http://headache.hungry.com/~seth/hifi/dirt.jpeg",
        yTextureURL: "http://headache.hungry.com/~seth/hifi/grass.png",
        zTextureURL: "http://headache.hungry.com/~seth/hifi/dirt.jpeg"
    });
    Entities.setAllVoxels(polyVoxId, 255);

    // XXX use setCuboid
    for (var y = 16; y < 32; y++) {
        for (var x = 0; x < 16; x++) {
            for (var z = 0; z < 16; z++) {
                Entities.setVoxel(polyVoxId, {x:x, y:y, z:z}, 0);
            }
        }
    }

    //////////
    // stitch together the terrain with x/y/z NeighorID properties
    //////////

    // link plots which are lower on the axes to this one
    imXNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {x:-16, y:0, z:0}));
    if (imXNeighborFor) {
        var properties = Entities.getEntityProperties(imXNeighborFor);
        properties.xNeighborID = polyVoxId;
        Entities.editEntity(imXNeighborFor, properties);
    }

    imYNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {x:0, y:-16, z:0}));
    if (imYNeighborFor) {
        var properties = Entities.getEntityProperties(imYNeighborFor);
        properties.yNeighborID = polyVoxId;
        Entities.editEntity(imYNeighborFor, properties);
    }

    imZNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {x:0, y:0, z:-16}));
    if (imZNeighborFor) {
        var properties = Entities.getEntityProperties(imZNeighborFor);
        properties.zNeighborID = polyVoxId;
        Entities.editEntity(imZNeighborFor, properties);
    }


    // link this plot to plots which are higher on the axes
    xNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {x:16, y:0, z:0}));
    if (xNeighborFor) {
        var properties = Entities.getEntityProperties(polyVoxId);
        properties.xNeighborID = xNeighborFor;
        Entities.editEntity(polyVoxId, properties);
    }

    yNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {x:0, y:16, z:0}));
    if (yNeighborFor) {
        var properties = Entities.getEntityProperties(polyVoxId);
        properties.yNeighborID = yNeighborFor;
        Entities.editEntity(polyVoxId, properties);
    }

    zNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {x:0, y:0, z:16}));
    if (zNeighborFor) {
        var properties = Entities.getEntityProperties(polyVoxId);
        properties.zNeighborID = zNeighborFor;
        Entities.editEntity(polyVoxId, properties);
    }

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
    var pickRayDirInVoxelSpace = Entities.localCoordsToVoxelCoords(intersection.entityID, pickRayDir);
    pickRayDirInVoxelSpace = Vec3.normalize(pickRayDirInVoxelSpace);

    var doAdd = addingVoxels;
    var doDelete = deletingVoxels;

    if (controlHeld) {
        doAdd = deletingVoxels;
        doDelete = addingVoxels;
    }

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
