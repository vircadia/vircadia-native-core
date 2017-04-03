//
//  photobooth.js
//  scripts/developer/utilities/render/photobooth
//
//  Created by Faye Li on 2 Nov 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals Tablet, Toolbars, Script, HMD, Controller, Menu */
(function () {
    var SNAPSHOT_DELAY = 500; // 500ms
    var PHOTOBOOTH_WINDOW_HTML_URL = Script.resolvePath("./html/photobooth.html");
    var PHOTOBOOTH_SETUP_JSON_URL = Script.resolvePath("./photoboothSetup.json");
    var MODEL_BOUNDING_BOX_DIMENSIONS = {x: 1.0174,y: 1.1925,z: 1.0165};
    var PhotoBooth = {};
    var photoboothCreated = false;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/snap-i.svg",
        text: "PHOTOBOOTH"
    });

    function onClicked() {
        if (photoboothCreated) {
            tablet.gotoHomeScreen();
            PhotoBooth.destroy();
        } else {
            tablet.gotoWebScreen(PHOTOBOOTH_WINDOW_HTML_URL);
            PhotoBooth.init();
        }
    }

    function onScreenChanged() {
        if (photoboothCreated) {
            tablet.gotoHomeScreen();
            PhotoBooth.destroy();
            button.editProperties({isActive: false});
        } else {
            button.editProperties({isActive: true});
        }
    }
    tablet.screenChanged.connect(onScreenChanged);
    button.clicked.connect(onClicked);
    tablet.webEventReceived.connect(onWebEventReceived);


    function onWebEventReceived(event) {
        print("photobooth.js received a web event:" + event);
        // Converts the event to a JavasScript Object
        if (typeof event === "string") {
            event = JSON.parse(event);
        }
        if (event.app === "photobooth") {
            if (event.type === "onClickPictureButton") {
                print("clicked picture button");
                // // hide HUD tool bar
                // toolbar.writeProperty("visible", false);
                // hide Overlays (such as Running Scripts or other Dialog UI)
                Menu.setIsOptionChecked("Overlays", false);
                // hide mouse cursor
                Reticle.visible = false;
                // hide tablet
                HMD.closeTablet();
                // // giving a delay here before snapshotting so that there is time to hide other UIs
                // void WindowScriptingInterface::takeSnapshot(bool notify, bool includeAnimated, float aspectRatio)
                Script.setTimeout(function () {
                    Window.takeSnapshot(false, false, 1.91);
                    // show hidden items after snapshot is taken
                    // issue: currently there's no way to show tablet via a script command. user will have to manually open tablet again
                    // issue: somehow we don't need to reset cursor to visible in script and the mouse still returns after snapshot
                    // Reticle.visible = true;
                    // toolbar.writeProperty("visible", true);
                    Menu.setIsOptionChecked("Overlays", true);
                }, SNAPSHOT_DELAY);
            } else if (event.type === "onClickReloadModelButton") {
                print("clicked reload model button " + event.data.value);
                PhotoBooth.changeModel(event.data.value);
            } else if (event.type === "onSelectCamera") {
                print("selected camera " + event.data.value);
                if (!event.data.hasOwnProperty("value")){
                    return;
                }
                if (event.data.value === "First Person Camera") {
                    Camera.mode = "first person";
                } else {
                    Camera.mode = "entity";
                    var cameraID = PhotoBooth.cameraEntities[event.data.value];
                    Camera.setCameraEntity(cameraID);
                }
            } else if (event.type === "onRotateSlider") {
                var props = {};
                props.rotation = Quat.fromPitchYawRollDegrees(0, event.data.value, 0);
                Entities.editEntity(PhotoBooth.modelEntityID, props);
            }
        }
    }

    PhotoBooth.init = function () {
        photoboothCreated = true;
        var success = Clipboard.importEntities(PHOTOBOOTH_SETUP_JSON_URL);
        var frontFactor = 10;
        // getForward is preffered as getFront function is deprecated
        var frontUnitVec = Vec3.normalize(Quat.getFront(MyAvatar.orientation));
        var frontOffset = Vec3.multiply(frontUnitVec,frontFactor);
        var upFactor = 3;
        var upUnitVec = Vec3.normalize(Quat.getUp(MyAvatar.orientation));
        var upOffset = Vec3.multiply(upUnitVec, upFactor);
        var spawnLocation = Vec3.sum(MyAvatar.position,frontOffset);
        spawnLocation = Vec3.sum(spawnLocation, upOffset);
        if (success) {
            this.pastedEntityIDs = Clipboard.pasteEntities(spawnLocation);
            this.processPastedEntities();
        }
    };

    PhotoBooth.processPastedEntities = function () {
        var cameraResults = {};
        var modelResult;
        var modelPos;
        this.pastedEntityIDs.forEach(function(id) {
            var props = Entities.getEntityProperties(id);
            var parts = props["name"].split(":");
            if (parts[0] === "Photo Booth Camera") {
                cameraResults[parts[1]] = id;
            }
            if (parts[0] === "Photo Booth Model") {
                modelResult = id;
                modelPos = props.position;
            }
        });
        print(JSON.stringify(cameraResults));
        print(JSON.stringify(modelResult));
        this.cameraEntities = cameraResults;
        this.modelEntityID = modelResult;
        this.centrePos = modelPos;
    };

    // replace the model in scene with new model
    PhotoBooth.changeModel = function (newModelURL) {
        // deletes old model
        Entities.deleteEntity(this.modelEntityID);
        // create new model at centre of the photobooth
        var newProps = {
            name: "Photo Booth Model",
            type: "Model",
            modelURL: newModelURL,
            position: this.centrePos
        };
        var newModelEntityID = Entities.addEntity(newProps);

        // scale model dimensions to fit in bounding box
        var scaleModel = function () {
            newProps = Entities.getEntityProperties(newModelEntityID);
            var myDimensions = newProps.dimensions;
            print("myDimensions: " + JSON.stringify(myDimensions));
            var k;
            if (myDimensions.x > MODEL_BOUNDING_BOX_DIMENSIONS.x) {
                k = MODEL_BOUNDING_BOX_DIMENSIONS.x / myDimensions.x;
                myDimensions = Vec3.multiply(k, myDimensions);
            }
            if (myDimensions.y > MODEL_BOUNDING_BOX_DIMENSIONS.y) {
                k = MODEL_BOUNDING_BOX_DIMENSIONS.y / myDimensions.y;
                myDimensions = Vec3.multiply(k, myDimensions);
            }
            if (myDimensions.z > MODEL_BOUNDING_BOX_DIMENSIONS.z) {
                k = MODEL_BOUNDING_BOX_DIMENSIONS.z / myDimensions.z;
                myDimensions = Vec3.multiply(k, myDimensions);
            }
            // position the new model on the table
            var y_offset = (MODEL_BOUNDING_BOX_DIMENSIONS.y - myDimensions.y) / 2;
            var myPosition = Vec3.sum(newProps.position, {x:0, y:-y_offset, z:0});
            Entities.editEntity(newModelEntityID,{position: myPosition, dimensions: myDimensions});
        };

        // add a delay before scaling to make sure the entity server have gotten the right model dimensions
        Script.setTimeout(function () {
            scaleModel();
        }, 400);

        this.modelEntityID = newModelEntityID;
    };

    PhotoBooth.destroy = function () {
        this.pastedEntityIDs.forEach(function(id) {
            Entities.deleteEntity(id);
        });
        Entities.deleteEntity(this.modelEntityID);
        photoboothCreated = false;
        Camera.mode = "first person";
    };
    
    function cleanup() {
        tablet.removeButton(button);
        PhotoBooth.destroy();
    }
    Script.scriptEnding.connect(cleanup);
}());