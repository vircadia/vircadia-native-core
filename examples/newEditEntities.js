//
//  newEditEntities.js
//  examples
//
//  Created by Brad Hefta-Gaub on 10/2/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script allows you to edit entities with a new UI/UX for mouse and trackpad based editing
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");
Script.include("libraries/stringHelpers.js");
Script.include("libraries/dataviewHelpers.js");
Script.include("libraries/httpMultiPart.js");
Script.include("libraries/modelUploader.js");
Script.include("libraries/toolBars.js");
Script.include("libraries/progressDialog.js");

Script.include("libraries/entitySelectionTool.js");
var selectionDisplay = SelectionDisplay;
var selectionManager = SelectionManager;

Script.include("libraries/ModelImporter.js");
var modelImporter = new ModelImporter();

Script.include("libraries/ExportMenu.js");
Script.include("libraries/ToolTip.js");

Script.include("libraries/entityPropertyDialogBox.js");
var entityPropertyDialogBox = EntityPropertyDialogBox;

Script.include("libraries/entityCameraTool.js");
var cameraManager = new CameraManager();

selectionManager.setEventListener(selectionDisplay.updateHandles);

var windowDimensions = Controller.getViewportDimensions();
var toolIconUrl = HIFI_PUBLIC_BUCKET + "images/tools/";
var toolHeight = 50;
var toolWidth = 50;

var MIN_ANGULAR_SIZE = 2;
var MAX_ANGULAR_SIZE = 45;
var allowLargeModels = false;
var allowSmallModels = false;
var wantEntityGlow = false;

var SPAWN_DISTANCE = 1;
var DEFAULT_DIMENSION = 0.20;

var modelURLs = [
        HIFI_PUBLIC_BUCKET + "meshes/Feisar_Ship.FBX",
        HIFI_PUBLIC_BUCKET + "meshes/birarda/birarda_head.fbx",
        HIFI_PUBLIC_BUCKET + "meshes/pug.fbx",
        HIFI_PUBLIC_BUCKET + "meshes/newInvader16x16-large-purple.svo",
        HIFI_PUBLIC_BUCKET + "meshes/minotaur/mino_full.fbx",
        HIFI_PUBLIC_BUCKET + "meshes/Combat_tank_V01.FBX",
        HIFI_PUBLIC_BUCKET + "meshes/orc.fbx",
        HIFI_PUBLIC_BUCKET + "meshes/slimer.fbx"
    ];

var mode = 0;
var isActive = false;


var toolBar = (function () {
    var that = {},
        toolBar,
        activeButton,
        newModelButton,
        newCubeButton,
        newSphereButton,
        newLightButton,
        browseModelsButton,
        loadURLMenuItem,
        loadFileMenuItem,
        menuItemWidth = 125,
        menuItemOffset,
        menuItemHeight,
        menuItemMargin = 5,
        menuTextColor = { red: 255, green: 255, blue: 255 },
        menuBackgoundColor = { red: 18, green: 66, blue: 66 };

    function initialize() {
        toolBar = new ToolBar(0, 0, ToolBar.VERTICAL);

        activeButton = toolBar.addTool({
            imageURL: toolIconUrl + "models-tool.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        }, true, false);

        newModelButton = toolBar.addTool({
            imageURL: toolIconUrl + "add-model-tool.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        }, true, false);

        browseModelsButton = toolBar.addTool({
            imageURL: toolIconUrl + "list-icon.svg",
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        });

        menuItemOffset = toolBar.height / 3 + 2;
        menuItemHeight = Tool.IMAGE_HEIGHT / 2 - 2;

        loadURLMenuItem = Overlays.addOverlay("text", {
            x: newModelButton.x - menuItemWidth,
            y: newModelButton.y + menuItemOffset,
            width: menuItemWidth,
            height: menuItemHeight,
            backgroundColor: menuBackgoundColor,
            topMargin: menuItemMargin,
            text: "Model URL",
            alpha: 0.9,
            visible: false
        });

        loadFileMenuItem = Overlays.addOverlay("text", {
            x: newModelButton.x - menuItemWidth,
            y: newModelButton.y + menuItemOffset + menuItemHeight,
            width: menuItemWidth,
            height: menuItemHeight,
            backgroundColor: menuBackgoundColor,
            topMargin: menuItemMargin,
            text: "Model File",
            alpha: 0.9,
            visible: false
        });

        newCubeButton = toolBar.addTool({
            imageURL: toolIconUrl + "add-cube.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        });

        newSphereButton = toolBar.addTool({
            imageURL: toolIconUrl + "add-sphere.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        });

        newLightButton = toolBar.addTool({
            imageURL: toolIconUrl + "light.svg",
            subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
            width: toolWidth,
            height: toolHeight,
            alpha: 0.9,
            visible: true
        });

    }

    function toggleNewModelButton(active) {
        if (active === undefined) {
            active = !toolBar.toolSelected(newModelButton);
        }
        toolBar.selectTool(newModelButton, active);

        Overlays.editOverlay(loadURLMenuItem, { visible: active });
        Overlays.editOverlay(loadFileMenuItem, { visible: active });
    }

    var RESIZE_INTERVAL = 50;
    var RESIZE_TIMEOUT = 20000;
    var RESIZE_MAX_CHECKS = RESIZE_TIMEOUT / RESIZE_INTERVAL;
    function addModel(url) {
        var position;

        position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

        if (position.x > 0 && position.y > 0 && position.z > 0) {
            var entityId = Entities.addEntity({
                type: "Model",
                position: position,
                dimensions: { x: DEFAULT_DIMENSION, y: DEFAULT_DIMENSION, z: DEFAULT_DIMENSION },
                modelURL: url
            });
            print("Model added: " + url);

            var checkCount = 0;
            function resize() {
                var entityProperties = Entities.getEntityProperties(entityId);
                var naturalDimensions = entityProperties.naturalDimensions;

                checkCount++;

                if (naturalDimensions.x == 0 && naturalDimensions.y == 0 && naturalDimensions.z == 0) {
                    if (checkCount < RESIZE_MAX_CHECKS) {
                        Script.setTimeout(resize, RESIZE_INTERVAL);
                    } else {
                        print("Resize failed: timed out waiting for model (" + url + ") to load");
                    }
                } else {
                    entityProperties.dimensions = naturalDimensions;
                    Entities.editEntity(entityId, entityProperties);
                }
            }

            Script.setTimeout(resize, RESIZE_INTERVAL);
        } else {
            print("Can't add model: Model would be out of bounds.");
        }
    }

    that.move = function () {
        var newViewPort,
            toolsX,
            toolsY;

        newViewPort = Controller.getViewportDimensions();

        if (toolBar === undefined) {
            initialize();

        } else if (windowDimensions.x === newViewPort.x &&
                   windowDimensions.y === newViewPort.y) {
            return;
        }

        windowDimensions = newViewPort;
        toolsX = windowDimensions.x - 8 - toolBar.width;
        toolsY = (windowDimensions.y - toolBar.height) / 2;

        toolBar.move(toolsX, toolsY);

        Overlays.editOverlay(loadURLMenuItem, { x: toolsX - menuItemWidth, y: toolsY + menuItemOffset });
        Overlays.editOverlay(loadFileMenuItem, { x: toolsX - menuItemWidth, y: toolsY + menuItemOffset + menuItemHeight });
    };

    that.mousePressEvent = function (event) {
        var clickedOverlay,
            url,
            file;

        clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (activeButton === toolBar.clicked(clickedOverlay)) {
            isActive = !isActive;
            if (!isActive) {
                selectionDisplay.unselectAll();
                cameraManager.disable();
            } else {
                cameraManager.enable();
            }
            return true;
        }

        if (newModelButton === toolBar.clicked(clickedOverlay)) {
            toggleNewModelButton();
            return true;
        }

        if (clickedOverlay === loadURLMenuItem) {
            toggleNewModelButton(false);
            url = Window.prompt("Model URL", modelURLs[Math.floor(Math.random() * modelURLs.length)]);
            if (url !== null && url !== "") {
                addModel(url);
            }
            return true;
        }

        if (clickedOverlay === loadFileMenuItem) {
            toggleNewModelButton(false);

            file = Window.browse("Select your model file ...",
                Settings.getValue("LastModelUploadLocation").path(), 
                "Model files (*.fst *.fbx)");
                //"Model files (*.fst *.fbx *.svo)");
            if (file !== null) {
                Settings.setValue("LastModelUploadLocation", file);
                modelUploader.upload(file, addModel);
            }
            return true;
        }

        if (browseModelsButton === toolBar.clicked(clickedOverlay)) {
            toggleNewModelButton(false);
            url = Window.s3Browse(".*(fbx|FBX)");
            if (url !== null && url !== "") {
                addModel(url);
            }
            return true;
        }

        if (newCubeButton === toolBar.clicked(clickedOverlay)) {
            var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

            if (position.x > 0 && position.y > 0 && position.z > 0) {
                Entities.addEntity({ 
                                type: "Box",
                                position: position,
                                dimensions: { x: DEFAULT_DIMENSION, y: DEFAULT_DIMENSION, z: DEFAULT_DIMENSION },
                                color: { red: 255, green: 0, blue: 0 }

                                });
            } else {
                print("Can't create box: Box would be out of bounds.");
            }
            return true;
        }

        if (newSphereButton === toolBar.clicked(clickedOverlay)) {
            var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

            if (position.x > 0 && position.y > 0 && position.z > 0) {
                Entities.addEntity({ 
                                type: "Sphere",
                                position: position,
                                dimensions: { x: DEFAULT_DIMENSION, y: DEFAULT_DIMENSION, z: DEFAULT_DIMENSION },
                                color: { red: 255, green: 0, blue: 0 }
                                });
            } else {
                print("Can't create sphere: Sphere would be out of bounds.");
            }
            return true;
        }

        if (newLightButton === toolBar.clicked(clickedOverlay)) {
            var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

            if (position.x > 0 && position.y > 0 && position.z > 0) {
                Entities.addEntity({ 
                                type: "Light",
                                position: position,
                                dimensions: { x: DEFAULT_DIMENSION, y: DEFAULT_DIMENSION, z: DEFAULT_DIMENSION },
                                isSpotlight: false,
                                diffuseColor: { red: 255, green: 255, blue: 255 },
                                ambientColor: { red: 255, green: 255, blue: 255 },
                                specularColor: { red: 0, green: 0, blue: 0 },

                                constantAttenuation: 1,
                                linearAttenuation: 0,
                                quadraticAttenuation: 0,
                                exponent: 0,
                                cutoff: 180, // in degrees
                                });
            } else {
                print("Can't create Light: Light would be out of bounds.");
            }
            return true;
        }



        return false;
    };

    that.cleanup = function () {
        toolBar.cleanup();
        Overlays.deleteOverlay(loadURLMenuItem);
        Overlays.deleteOverlay(loadFileMenuItem);
    };

    return that;
}());


var exportMenu = null;

function isLocked(properties) {
    // special case to lock the ground plane model in hq.
    if (location.hostname == "hq.highfidelity.io" &&
        properties.modelURL == HIFI_PUBLIC_BUCKET + "ozan/Terrain_Reduce_forAlpha.fbx") {
        return true;
    }
    return false;
}


var entitySelected = false;
var selectedEntityID;
var selectedEntityProperties;
var mouseLastPosition;
var orientation;
var intersection;


var SCALE_FACTOR = 200.0;

function rayPlaneIntersection(pickRay, point, normal) {
    var d = -Vec3.dot(point, normal);
    var t = -(Vec3.dot(pickRay.origin, normal) + d) / Vec3.dot(pickRay.direction, normal);

    return Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction, t));
}


function mousePressEvent(event) {
    mouseLastPosition = { x: event.x, y: event.y };
    var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

    if (toolBar.mousePressEvent(event) || progressDialog.mousePressEvent(event)
        || cameraManager.mousePressEvent(event) || selectionDisplay.mousePressEvent(event)) {
        // Event handled; do nothing.
        return;
    } else {
        entitySelected = false;
        selectionDisplay.unselectAll();

        // If we aren't active and didn't click on an overlay: quit
        if (!isActive) {
            return;
        }

        var pickRay = Camera.computePickRay(event.x, event.y);
        Vec3.print("[Mouse] Looking at: ", pickRay.origin);
        var foundIntersection = Entities.findRayIntersection(pickRay);

        if(!foundIntersection.accurate) {
            return;
        }
        var foundEntity = foundIntersection.entityID;

        if (!foundEntity.isKnownID) {
            var identify = Entities.identifyEntity(foundEntity);
            if (!identify.isKnownID) {
                print("Unknown ID " + identify.id + " (update loop " + foundEntity.id + ")");
                return;
            }
            foundEntity = identify;
        }

        var properties = Entities.getEntityProperties(foundEntity);
        if (isLocked(properties)) {
            print("Model locked " + properties.id);
        } else {
            var halfDiagonal = Vec3.length(properties.dimensions) / 2.0;

            print("Checking properties: " + properties.id + " " + properties.isKnownID + " - Half Diagonal:" + halfDiagonal);
            //                P         P - Model
            //               /|         A - Palm
            //              / | d       B - unit vector toward tip
            //             /  |         X - base of the perpendicular line
            //            A---X----->B  d - distance fom axis
            //              x           x - distance from A
            //
            //            |X-A| = (P-A).B
            //            X == A + ((P-A).B)B
            //            d = |P-X|

            var A = pickRay.origin;
            var B = Vec3.normalize(pickRay.direction);
            var P = properties.position;

            var x = Vec3.dot(Vec3.subtract(P, A), B);
            var X = Vec3.sum(A, Vec3.multiply(B, x));
            var d = Vec3.length(Vec3.subtract(P, X));
            var halfDiagonal = Vec3.length(properties.dimensions) / 2.0;

            var angularSize = 2 * Math.atan(halfDiagonal / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14;

            var sizeOK = (allowLargeModels || angularSize < MAX_ANGULAR_SIZE) 
                            && (allowSmallModels || angularSize > MIN_ANGULAR_SIZE);

            if (0 < x && sizeOK) {
                entitySelected = true;
                selectedEntityID = foundEntity;
                selectedEntityProperties = properties;
                orientation = MyAvatar.orientation;
                intersection = rayPlaneIntersection(pickRay, P, Quat.getFront(orientation));

                if (!event.isShifted) {
                    selectionManager.clearSelections();
                }
                selectionManager.addEntity(foundEntity);

                print("Model selected selectedEntityID:" + selectedEntityID.id);

            }
        }
    }
    if (entitySelected) {
        selectedEntityProperties.oldDimensions = selectedEntityProperties.dimensions;
        selectedEntityProperties.oldPosition = {
            x: selectedEntityProperties.position.x,
            y: selectedEntityProperties.position.y,
            z: selectedEntityProperties.position.z,
        };
        selectedEntityProperties.oldRotation = {
            x: selectedEntityProperties.rotation.x,
            y: selectedEntityProperties.rotation.y,
            z: selectedEntityProperties.rotation.z,
            w: selectedEntityProperties.rotation.w,
        };
        selectedEntityProperties.glowLevel = 0.0;
        
        print("Clicked on " + selectedEntityID.id + " " +  entitySelected);
        tooltip.updateText(selectedEntityProperties);
        tooltip.show(true);
        selectionDisplay.select(selectedEntityID, event);
    }
}

var highlightedEntityID = { isKnownID: false };

function mouseMoveEvent(event) {
    if (!isActive) {
        return;
    }
    
    // allow the selectionDisplay and cameraManager to handle the event first, if it doesn't handle it, then do our own thing
    if (selectionDisplay.mouseMoveEvent(event) || cameraManager.mouseMoveEvent(event)) {
        return;
    }

    var pickRay = Camera.computePickRay(event.x, event.y);
    var entityIntersection = Entities.findRayIntersection(pickRay);
    if (entityIntersection.accurate) {
        if(highlightedEntityID.isKnownID && highlightedEntityID.id != entityIntersection.entityID.id) {
            selectionDisplay.unhighlightSelectable(highlightedEntityID);
            highlightedEntityID = { id: -1, isKnownID: false };
        }

        var halfDiagonal = Vec3.length(entityIntersection.properties.dimensions) / 2.0;
        
        var angularSize = 2 * Math.atan(halfDiagonal / Vec3.distance(Camera.getPosition(), 
                                        entityIntersection.properties.position)) * 180 / 3.14;

        var sizeOK = (allowLargeModels || angularSize < MAX_ANGULAR_SIZE) 
                        && (allowSmallModels || angularSize > MIN_ANGULAR_SIZE);

        if (entityIntersection.entityID.isKnownID && sizeOK) {
            if (wantEntityGlow) {
                Entities.editEntity(entityIntersection.entityID, { glowLevel: 0.25 });
            }
            highlightedEntityID = entityIntersection.entityID;
            selectionDisplay.highlightSelectable(entityIntersection.entityID);
        }
        
    }
}


function mouseReleaseEvent(event) {
    if (!isActive) {
        return;
    }
    if (entitySelected) {
        tooltip.show(false);
    }
    cameraManager.mouseReleaseEvent(event);
}

Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);


// In order for editVoxels and editModels to play nice together, they each check to see if a "delete" menu item already
// exists. If it doesn't they add it. If it does they don't. They also only delete the menu item if they were the one that
// added it.
var modelMenuAddedDelete = false;
function setupModelMenus() {
    print("setupModelMenus()");
    // adj our menuitems
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Models", isSeparator: true, beforeItem: "Physics" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Edit Properties...",
        shortcutKeyEvent: { text: "`" }, afterItem: "Models" });
    if (!Menu.menuItemExists("Edit", "Delete")) {
        print("no delete... adding ours");
        Menu.addMenuItem({ menuName: "Edit", menuItemName: "Delete",
            shortcutKeyEvent: { text: "backspace" }, afterItem: "Models" });
        modelMenuAddedDelete = true;
    } else {
        print("delete exists... don't add ours");
    }

    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Paste Models", shortcutKey: "CTRL+META+V", afterItem: "Edit Properties..." });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Allow Select Large Models", shortcutKey: "CTRL+META+L", 
                        afterItem: "Paste Models", isCheckable: true });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Allow Select Small Models", shortcutKey: "CTRL+META+S", 
                        afterItem: "Allow Select Large Models", isCheckable: true });

    Menu.addMenuItem({ menuName: "File", menuItemName: "Models", isSeparator: true, beforeItem: "Settings" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Export Models", shortcutKey: "CTRL+META+E", afterItem: "Models" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Import Models", shortcutKey: "CTRL+META+I", afterItem: "Export Models" });
    Menu.addMenuItem({ menuName: "Developer", menuItemName: "Debug Ryans Rotation Problems", isCheckable: true });
}

setupModelMenus(); // do this when first running our script.

function cleanupModelMenus() {
    Menu.removeSeparator("Edit", "Models");
    Menu.removeMenuItem("Edit", "Edit Properties...");
    if (modelMenuAddedDelete) {
        // delete our menuitems
        Menu.removeMenuItem("Edit", "Delete");
    }

    Menu.removeMenuItem("Edit", "Paste Models");
    Menu.removeMenuItem("Edit", "Allow Select Large Models");
    Menu.removeMenuItem("Edit", "Allow Select Small Models");

    Menu.removeSeparator("File", "Models");
    Menu.removeMenuItem("File", "Export Models");
    Menu.removeMenuItem("File", "Import Models");
    Menu.removeMenuItem("Developer", "Debug Ryans Rotation Problems");
}

Script.scriptEnding.connect(function() {
    progressDialog.cleanup();
    toolBar.cleanup();
    cleanupModelMenus();
    tooltip.cleanup();
    modelImporter.cleanup();
    selectionDisplay.cleanup();
    if (exportMenu) {
        exportMenu.close();
    }
});

// Do some stuff regularly, like check for placement of various overlays
Script.update.connect(function (deltaTime) {
    toolBar.move();
    progressDialog.move();
    selectionDisplay.checkMove();
});

function handeMenuEvent(menuItem) {
    if (menuItem == "Allow Select Small Models") {
        allowSmallModels = Menu.isOptionChecked("Allow Select Small Models");
    } else if (menuItem == "Allow Select Large Models") {
        allowLargeModels = Menu.isOptionChecked("Allow Select Large Models");
    } else if (menuItem == "Delete") {
        if (entitySelected) {
            print("  Delete Entity.... selectedEntityID="+ selectedEntityID);
            for (var i = 0; i < selectionManager.selections.length; i++) {
                Entities.deleteEntity(selectionManager.selections[i]);
            }
            selectionDisplay.unselect(selectedEntityID);
            entitySelected = false;
            selectionManager.clearSelections();
        } else {
            print("  Delete Entity.... not holding...");
        }
    } else if (menuItem == "Edit Properties...") {
        // good place to put the properties dialog

        editModelID = -1;
        if (selectionManager.selections.length == 1) {
            print("  Edit Properties.... selectedEntityID="+ selectedEntityID);
            editModelID = selectedEntityID;
        } else {
            print("  Edit Properties.... not holding...");
        }
        if (editModelID != -1) {
            print("  Edit Properties.... about to edit properties...");
            entityPropertyDialogBox.openDialog(editModelID);
        }
        
    } else if (menuItem == "Paste Models") {
        modelImporter.paste();
    } else if (menuItem == "Export Models") {
        if (!exportMenu) {
            exportMenu = new ExportMenu({
                onClose: function () {
                    exportMenu = null;
                }
            });
        }
    } else if (menuItem == "Import Models") {
        modelImporter.doImport();
    }
    tooltip.show(false);
}

Menu.menuItemEvent.connect(handeMenuEvent);

Controller.keyReleaseEvent.connect(function (event) {
    // since sometimes our menu shortcut keys don't work, trap our menu items here also and fire the appropriate menu items
    if (event.text == "`") {
        handeMenuEvent("Edit Properties...");
    }
    if (event.text == "BACKSPACE" || event.text == "DELETE") {
        handeMenuEvent("Delete");
    } else if (event.text == "TAB") {
        selectionDisplay.toggleSpaceMode();
    } else if (event.text == "ESC") {
        selectionDisplay.cancelTool();
    } else if (event.text == "f") {
        if (entitySelected) {
            // Get latest properties
            var properties = Entities.getEntityProperties(selectedEntityID);
            cameraManager.focus(properties);
        }
    } else if (event.text == '[') {
        if (isActive) {
            cameraManager.enable();
        }
    }
});



