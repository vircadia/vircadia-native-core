var controlHeld = false;
var shiftHeld = false;

Script.include([
    "./libraries/toolBars.js",
    "./libraries/utils.js",
]);

var isActive = false;
var toolIconUrl = Script.resolvePath("assets/images/tools/");

var toolHeight = 50;
var toolWidth = 50;

var addingVoxels = false;
var deletingVoxels = false;
var addingSpheres = false;
var deletingSpheres = false;

var offAlpha = 0.8;
var onAlpha = 1.0;
var editSphereRadius = 4;

function floorVector(v) {
    return {
        x: Math.floor(v.x),
        y: Math.floor(v.y),
        z: Math.floor(v.z)
    };
}

var toolBar = (function() {
    var that = {},
        toolBar,
        activeButton,
        addVoxelButton,
        deleteVoxelButton,
        addSphereButton,
        deleteSphereButton,
        addTerrainButton;

    function initialize() {
        toolBar = new ToolBar(0, 0, ToolBar.VERTICAL, "highfidelity.voxel.toolbar", function(windowDimensions, toolbar) {
            return {
                x: windowDimensions.x - 8 * 2 - toolbar.width * 2,
                y: (windowDimensions.y - toolbar.height) / 2
            };
        });

        activeButton = toolBar.addTool({
            imageURL: toolIconUrl+"voxels.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: onAlpha,
            visible: true,
        }, false);

        addVoxelButton = toolBar.addTool({
            imageURL: toolIconUrl + "voxel-add.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: offAlpha,
            visible: false
        }, false);

        deleteVoxelButton = toolBar.addTool({
            imageURL: toolIconUrl + "voxel-delete.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: offAlpha,
            visible: false
        }, false);

        addSphereButton = toolBar.addTool({
            imageURL: toolIconUrl + "sphere-add.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: offAlpha,
            visible: false
        }, false);

        deleteSphereButton = toolBar.addTool({
            imageURL: toolIconUrl + "sphere-delete.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: offAlpha,
            visible: false
        }, false);

        addTerrainButton = toolBar.addTool({
            imageURL: toolIconUrl + "voxel-terrain.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: onAlpha,
            visible: false
        }, false);

        that.setActive(false);
    }

    function disableAllButtons() {
        addingVoxels = false;
        deletingVoxels = false;
        addingSpheres = false;
        deletingSpheres = false;

        toolBar.setAlpha(offAlpha, addVoxelButton);
        toolBar.setAlpha(offAlpha, deleteVoxelButton);
        toolBar.setAlpha(offAlpha, addSphereButton);
        toolBar.setAlpha(offAlpha, deleteSphereButton);

        toolBar.selectTool(addVoxelButton, false);
        toolBar.selectTool(deleteVoxelButton, false);
        toolBar.selectTool(addSphereButton, false);
        toolBar.selectTool(deleteSphereButton, false);
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
        toolBar.showTool(addSphereButton, doShow);
        toolBar.showTool(deleteSphereButton, doShow);
        toolBar.showTool(addTerrainButton, doShow);
    };

    that.mousePressEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });

        if (activeButton === toolBar.clicked(clickedOverlay)) {
            that.setActive(!isActive);
            return true;
        }

        if (addVoxelButton === toolBar.clicked(clickedOverlay)) {
            var wasAddingVoxels = addingVoxels;
            disableAllButtons()
            if (!wasAddingVoxels) {
                addingVoxels = true;
                toolBar.setAlpha(onAlpha, addVoxelButton);
            }
            return true;
        }

        if (deleteVoxelButton === toolBar.clicked(clickedOverlay)) {
            var wasDeletingVoxels = deletingVoxels;
            disableAllButtons()
            if (!wasDeletingVoxels) {
                deletingVoxels = true;
                toolBar.setAlpha(onAlpha, deleteVoxelButton);
            }
            return true;
        }

        if (addSphereButton === toolBar.clicked(clickedOverlay)) {
            var wasAddingSpheres = addingSpheres
            disableAllButtons()
            if (!wasAddingSpheres) {
                addingSpheres = true;
                toolBar.setAlpha(onAlpha, addSphereButton);
            }
            return true;
        }

        if (deleteSphereButton === toolBar.clicked(clickedOverlay)) {
            var wasDeletingSpheres = deletingSpheres;
            disableAllButtons()
            if (!wasDeletingSpheres) {
                deletingSpheres = true;
                toolBar.setAlpha(onAlpha, deleteSphereButton);
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

    that.cleanup = function() {
        toolBar.cleanup();
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


function grabLowestJointY() {
    var jointNames = MyAvatar.getJointNames();
    var floorY = MyAvatar.position.y;
    for (var jointName in jointNames) {
        if (MyAvatar.getJointPosition(jointNames[jointName]).y < floorY) {
            floorY = MyAvatar.getJointPosition(jointNames[jointName]).y;
        }
    }
    return floorY;
}


function addTerrainBlock() {
    var baseLocation = getTerrainAlignedLocation(Vec3.sum(MyAvatar.position, {
        x: 8,
        y: 8,
        z: 8
    }));
    if (baseLocation.y > MyAvatar.position.y) {
        baseLocation.y -= 16;
    }

    var alreadyThere = lookupTerrainForLocation(baseLocation);
    if (alreadyThere) {
        // there is already a terrain block under MyAvatar.
        // try in front of the avatar.
        facingPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(8.0, Quat.getFront(Camera.getOrientation())));
        facingPosition = Vec3.sum(facingPosition, {
            x: 8,
            y: 8,
            z: 8
        });
        baseLocation = getTerrainAlignedLocation(facingPosition);
        alreadyThere = lookupTerrainForLocation(baseLocation);
        if (alreadyThere) {
            return null;
        }
    }

    var polyVoxID = addTerrainBlockNearLocation(baseLocation);

    if (polyVoxID) {
        var AvatarPositionInVoxelCoords = Entities.worldCoordsToVoxelCoords(polyVoxID, MyAvatar.position);
        // TODO -- how to find the avatar's feet?
        var topY = Math.round(AvatarPositionInVoxelCoords.y) - 4;
        Entities.setVoxelsInCuboid(polyVoxID, {
            x: 0,
            y: 0,
            z: 0
        }, {
            x: 16,
            y: topY,
            z: 16
        }, 255);
    }
}

function addTerrainBlockNearLocation(baseLocation) {
    var alreadyThere = lookupTerrainForLocation(baseLocation);
    if (alreadyThere) {
        return null;
    }

    var polyVoxID = Entities.addEntity({
        type: "PolyVox",
        name: "terrain",
        position: baseLocation,
        dimensions: {
            x: 16,
            y: 16,
            z: 16
        },
        voxelVolumeSize: {
            x: 16,
            y: 64,
            z: 16
        },
        voxelSurfaceStyle: 0,
        xTextureURL: Script.resolvePath("assets/images/textures/dirt.jpeg"),
        yTextureURL: Script.resolvePath("assets/images/textures/grass.png"),
        zTextureURL: Script.resolvePath("assets/images/textures/dirt.jpeg")
    });

    //////////
    // stitch together the terrain with x/y/z NeighorID properties
    //////////

    // link neighbors to this plot
    imXNNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 16,
        y: 0,
        z: 0
    }));
    imYNNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 0,
        y: 16,
        z: 0
    }));
    imZNNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 0,
        y: 0,
        z: 16
    }));
    imXPNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: -16,
        y: 0,
        z: 0
    }));
    imYPNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 0,
        y: -16,
        z: 0
    }));
    imZPNeighborFor = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 0,
        y: 0,
        z: -16
    }));

    if (imXNNeighborFor) {
        var properties = Entities.getEntityProperties(imXNNeighborFor);
        properties.xNNeighborID = polyVoxID;
        Entities.editEntity(imXNNeighborFor, properties);
    }
    if (imYNNeighborFor) {
        var properties = Entities.getEntityProperties(imYNNeighborFor);
        properties.yNNeighborID = polyVoxID;
        Entities.editEntity(imYNNeighborFor, properties);
    }
    if (imZNNeighborFor) {
        var properties = Entities.getEntityProperties(imZNNeighborFor);
        properties.zNNeighborID = polyVoxID;
        Entities.editEntity(imZNNeighborFor, properties);
    }

    if (imXPNeighborFor) {
        var properties = Entities.getEntityProperties(imXPNeighborFor);
        properties.xPNeighborID = polyVoxID;
        Entities.editEntity(imXPNeighborFor, properties);
    }
    if (imYPNeighborFor) {
        var properties = Entities.getEntityProperties(imYPNeighborFor);
        properties.yPNeighborID = polyVoxID;
        Entities.editEntity(imYPNeighborFor, properties);
    }
    if (imZPNeighborFor) {
        var properties = Entities.getEntityProperties(imZPNeighborFor);
        properties.zPNeighborID = polyVoxID;
        Entities.editEntity(imZPNeighborFor, properties);
    }


    // link this plot to its neighbors
    var properties = Entities.getEntityProperties(polyVoxID);
    properties.xNNeighborID = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: -16,
        y: 0,
        z: 0
    }));
    properties.yNNeighborID = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 0,
        y: -16,
        z: 0
    }));
    properties.zNNeighborID = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 0,
        y: 0,
        z: -16
    }));
    properties.xPNeighborID = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 16,
        y: 0,
        z: 0
    }));
    properties.yPNeighborID = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 0,
        y: 16,
        z: 0
    }));
    properties.zPNeighborID = lookupTerrainForLocation(Vec3.sum(baseLocation, {
        x: 0,
        y: 0,
        z: 16
    }));
    Entities.editEntity(polyVoxID, properties);

    return polyVoxID;
}


function attemptVoxelChangeForEntity(entityID, pickRayDir, intersectionLocation) {

    var properties = Entities.getEntityProperties(entityID);
    if (properties.type != "PolyVox") {
        return false;
    }

    if (addingVoxels == false && deletingVoxels == false && addingSpheres == false && deletingSpheres == false) {
        return false;
    }

    var voxelOrigin = Entities.worldCoordsToVoxelCoords(entityID, Vec3.subtract(intersectionLocation, pickRayDir));
    var voxelPosition = Entities.worldCoordsToVoxelCoords(entityID, intersectionLocation);
    var pickRayDirInVoxelSpace = Vec3.subtract(voxelPosition, voxelOrigin);
    pickRayDirInVoxelSpace = Vec3.normalize(pickRayDirInVoxelSpace);

    var doAdd = addingVoxels;
    var doDelete = deletingVoxels;
    var doAddSphere = addingSpheres;
    var doDeleteSphere = deletingSpheres;

    if (controlHeld) {
        if (doAdd) {
            doAdd = false;
            doDelete = true;
        } else if (doDelete) {
            doDelete = false;
            doAdd = true;
        } else if (doAddSphere) {
            doAddSphere = false;
            doDeleteSphere = true;
        } else if (doDeleteSphere) {
            doDeleteSphere = false;
            doAddSphere = true;
        }
    }

    if (doDelete) {
        var toErasePosition = Vec3.sum(voxelPosition, Vec3.multiply(pickRayDirInVoxelSpace, 0.1));
        return Entities.setVoxel(entityID, floorVector(toErasePosition), 0);
    }
    if (doAdd) {
        var toDrawPosition = Vec3.subtract(voxelPosition, Vec3.multiply(pickRayDirInVoxelSpace, 0.1));
        return Entities.setVoxel(entityID, floorVector(toDrawPosition), 255);
    }
    if (doDeleteSphere) {
        var toErasePosition = intersectionLocation;
        return Entities.setVoxelSphere(entityID, floorVector(toErasePosition), editSphereRadius, 0);
    }
    if (doAddSphere) {
        var toDrawPosition = intersectionLocation;
        return Entities.setVoxelSphere(entityID, floorVector(toDrawPosition), editSphereRadius, 255);
    }
}


function attemptVoxelChange(pickRayDir, intersection) {

    var ids;

    ids = Entities.findEntities(intersection.intersection, editSphereRadius + 1.0);
    if (ids.indexOf(intersection.entityID) < 0) {
        ids.push(intersection.entityID);
    }

    var success = false;
    for (var i = 0; i < ids.length; i++) {
        var entityID = ids[i];
        success |= attemptVoxelChangeForEntity(entityID, pickRayDir, intersection.intersection)
    }
    return success;
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
    toolBar.cleanup();
}


Controller.mousePressEvent.connect(mousePressEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.scriptEnding.connect(cleanup);