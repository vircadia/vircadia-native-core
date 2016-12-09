(function () {
    var SNAPSHOT_DELAY = 500; // 500ms
    var PHOTOBOOTH_WINDOW_HTML_URL = Script.resolvePath("./html/photobooth.html");
    var PHOTOBOOTH_SETUP_JSON_URL = Script.resolvePath("./photoboothSetup.json");
    var toolbar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
    var MODEL_BOUNDING_BOX_DIMENSIONS = {x: 1.0174,y: 1.1925,z: 1.0165};

    var PhotoBooth = {};
    PhotoBooth.init = function () {
        var success = Clipboard.importEntities(PHOTOBOOTH_SETUP_JSON_URL);
        var frontFactor = 10;
        var frontUnitVec = Vec3.normalize(Quat.getFront(MyAvatar.orientation));
        var frontOffset = Vec3.multiply(frontUnitVec,frontFactor);
        var rightFactor = 3;
        var rightUnitVec = Vec3.normalize(Quat.getRight(MyAvatar.orientation));
        var spawnLocation = Vec3.sum(Vec3.sum(MyAvatar.position,frontOffset),rightFactor);
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
    };

    var main = function () {
        PhotoBooth.init();

        var photoboothWindowListener = {};
        photoboothWindowListener.onLoad = function (event) {
            print("loaded" + event.value);
            if (!event.hasOwnProperty("value")){
                return;
            }
        };

        photoboothWindowListener.onSelectCamera = function (event) {
            print("selected camera " + event.value);
            if (!event.hasOwnProperty("value")){
                return;
            }
            if (event.value === "First Person Camera") {
                Camera.mode = "first person";
            } else {
                Camera.mode = "entity";
                var cameraID = PhotoBooth.cameraEntities[event.value];
                Camera.setCameraEntity(cameraID);
            }
        };

        photoboothWindowListener.onSelectLightingPreset = function (event) {
            print("selected lighting preset" + event.value);
            if (!event.hasOwnProperty("value")){
                return;
            }
        };

        photoboothWindowListener.onClickPictureButton = function (event) {
            print("clicked picture button");
            // hide HUD tool bar
            toolbar.writeProperty("visible", false);
            // hide Overlays (such as Running Scripts or other Dialog UI)
            Menu.setIsOptionChecked("Overlays", false);
            // hide mouse cursor
            Reticle.visible = false;
            // giving a delay here before snapshotting so that there is time to hide toolbar and other UIs
            // void WindowScriptingInterface::takeSnapshot(bool notify, bool includeAnimated, float aspectRatio)
            Script.setTimeout(function () {
                Window.takeSnapshot(false, false, 1.91);
                // show hidden items after snapshot is taken
                toolbar.writeProperty("visible", true);
                Menu.setIsOptionChecked("Overlays", true);
                // unknown issue: somehow we don't need to reset cursor to visible in script and the mouse still returns after snapshot
                // Reticle.visible = true;
            }, SNAPSHOT_DELAY);
        };

        photoboothWindowListener.onClickReloadModelButton = function (event) {
            print("clicked reload model button " + event.value);
            PhotoBooth.changeModel(event.value);
        };
        
        var photoboothWindow = new OverlayWebWindow({
          title: 'Photo Booth',
          source: PHOTOBOOTH_WINDOW_HTML_URL,
          width: 450,
          height: 450,
          visible: true
        });

        photoboothWindow.webEventReceived.connect(function (data) {
            var event = JSON.parse(data);
            if (photoboothWindowListener[event.type]) {
                photoboothWindowListener[event.type](event);
            }
        });
    };
    main();
    
    function cleanup() {
        Camera.mode = "first person";
        PhotoBooth.destroy();
    }
    Script.scriptEnding.connect(cleanup);
}());