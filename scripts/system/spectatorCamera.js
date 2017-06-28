"use strict";
/*jslint vars:true, plusplus:true, forin:true*/
/*global Tablet, Script,  */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// spectatorCamera.js
//
// Created by Zach Fox on 2017-06-05
// Copyright 2017 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () { // BEGIN LOCAL_SCOPE

    //
    // FUNCTION VAR DECLARATIONS
    //
    var sendToQml, addOrRemoveButton, onTabletScreenChanged, fromQml,
        onTabletButtonClicked, wireEventBridge, startup, shutdown, registerButtonMappings;

    //
    // Function Name: inFrontOf()
    // 
    // Description:
    // Spectator camera utility functions and variables.
    //
    function inFrontOf(distance, position, orientation) {
        return Vec3.sum(position || MyAvatar.position,
                        Vec3.multiply(distance, Quat.getForward(orientation || MyAvatar.orientation)));
    }

    //
    // Function Name: updateRenderFromCamera()
    //
    // Relevant Variables:
    // spectatorFrameRenderConfig: The render configuration of the spectator camera
    //      render job. Controls size.
    // beginSpectatorFrameRenderConfig: The render configuration of the spectator camera
    //     render job. Controls position and orientation.
    // viewFinderOverlay: The in-world overlay that displays the spectator camera's view.
    // camera: The in-world entity that corresponds to the spectator camera.
    // cameraIsDynamic: "false" for now while we figure out why dynamic, parented overlays
    //     drift with respect to their parent
    // lastCameraPosition: Holds the last known camera position
    // lastCameraRotation: Holds the last known camera rotation
    // 
    // Arguments:
    // None
    // 
    // Description:
    // The update function for the spectator camera. Modifies the camera's position
    //     and orientation.
    //
    var spectatorFrameRenderConfig = Render.getConfig("SecondaryCameraFrame");
    var beginSpectatorFrameRenderConfig = Render.getConfig("BeginSecondaryCamera");
    var viewFinderOverlay = false;
    var camera = false;
    var cameraIsDynamic = false;
    var lastCameraPosition = false;
    var lastCameraRotation = false;
    function updateRenderFromCamera() {
        var cameraData = Entities.getEntityProperties(camera, ['position', 'rotation']);
        if (JSON.stringify(lastCameraRotation) !== JSON.stringify(cameraData.rotation)) {
            lastCameraRotation = cameraData.rotation;
            beginSpectatorFrameRenderConfig.orientation = lastCameraRotation;
        }
        if (JSON.stringify(lastCameraPosition) !== JSON.stringify(cameraData.position)) {
            lastCameraPosition = cameraData.position;
            beginSpectatorFrameRenderConfig.position = Vec3.sum(inFrontOf(0.17, lastCameraPosition, lastCameraRotation), {x: 0, y: 0.02, z: 0});
        }
    }

    //
    // Function Name: spectatorCameraOn()
    //
    // Relevant Variables:
    // isUpdateRenderWired: Bool storing whether or not the camera's update
    //     function is wired.
    // vFoV: The vertical field of view of the spectator camera
    // nearClipPlaneDistance: The near clip plane distance of the spectator camera (aka "camera")
    // farClipPlaneDistance: The far clip plane distance of the spectator camera
    // cameraRotation: The rotation of the spectator camera
    // cameraPosition: The position of the spectator camera
    // glassPaneWidth: The width of the glass pane above the spectator camera that holds the viewFinderOverlay
    // viewFinderOverlayDim: The x, y, and z dimensions of the viewFinderOverlay
    // 
    // Arguments:
    // None
    // 
    // Description:
    // Call this function to set up the spectator camera and
    //     spawn the camera entity.
    //
    var isUpdateRenderWired = false;
    var vFoV = 45.0;
    var nearClipPlaneDistance = 0.1;
    var farClipPlaneDistance = 100.0;
    var cameraRotation;
    var cameraPosition;
    //The negative y dimension for viewFinderOverlay is necessary for now due to the way Image3DOverlay
    //    draws textures, but should be looked into at some point. Also the z dimension shouldn't affect
    //    the overlay since it is an Image3DOverlay so it is set to 0
    var glassPaneWidth = 0.16;
    var viewFinderOverlayDim = { x: glassPaneWidth, y: -glassPaneWidth, z: 0 };
    function spectatorCameraOn() {
        // Sets the special texture size based on the window it is displayed in, which doesn't include the menu bar
        spectatorFrameRenderConfig.resetSizeSpectatorCamera(Window.innerWidth, Window.innerHeight);
        spectatorFrameRenderConfig.enabled = beginSpectatorFrameRenderConfig.enabled = true;
        beginSpectatorFrameRenderConfig.vFoV = vFoV;
        beginSpectatorFrameRenderConfig.nearClipPlaneDistance = nearClipPlaneDistance;
        beginSpectatorFrameRenderConfig.farClipPlaneDistance = farClipPlaneDistance;
        cameraRotation = MyAvatar.orientation, cameraPosition = inFrontOf(1, Vec3.sum(MyAvatar.position, { x: 0, y: 0.3, z: 0 }));
        Script.update.connect(updateRenderFromCamera);
        isUpdateRenderWired = true;
        camera = Entities.addEntity({
            "angularDamping": 0.98000001907348633,
            "collisionsWillMove": 0,
            "damping": 0.98000001907348633,
            "dynamic": cameraIsDynamic,
            "modelURL": "http://hifi-content.s3.amazonaws.com/alan/dev/spectator-camera.fbx?1",
            "rotation": cameraRotation,
            "position": cameraPosition,
            "shapeType": "simple-compound",
            "type": "Model",
            "userData": "{\"grabbableKey\":{\"grabbable\":true}}"
        }, true);
        // This image3d overlay acts as the camera's preview screen.
        viewFinderOverlay = Overlays.addOverlay("image3d", {
            url: "resource://spectatorCameraFrame",
            emissive: true,
            parentID: camera,
            alpha: 1,
            localRotation: { x: 0, y: 0, z: 0 },
            localPosition: { x: 0.007, y: 0.15, z: -0.005 },
            dimensions: viewFinderOverlayDim
        });
        setDisplay(monitorShowsCameraView);
    }

    //
    // Function Name: spectatorCameraOff()
    //
    // Relevant Variables:
    // None
    // 
    // Arguments:
    // None
    // 
    // Description:
    // Call this function to shut down the spectator camera and
    //     destroy the camera entity.
    //
    function spectatorCameraOff() {
        spectatorFrameRenderConfig.enabled = beginSpectatorFrameRenderConfig.enabled = false;
        if (isUpdateRenderWired) {
            Script.update.disconnect(updateRenderFromCamera);
            isUpdateRenderWired = false;
        }
        if (camera) {
            Entities.deleteEntity(camera);
        }
        if (viewFinderOverlay) {
            Overlays.deleteOverlay(viewFinderOverlay);
        }
        camera = false;
        viewFinderOverlay = false;
        setDisplay(monitorShowsCameraView);
    }

    //
    // Function Name: addOrRemoveButton()
    //
    // Relevant Variables:
    // button: The tablet button.
    // buttonName: The name of the button.
    // tablet: The tablet instance to be modified.
    // showInDesktop: Set to "true" to show the "SPECTATOR" app in desktop mode
    // 
    // Arguments:
    // isShuttingDown: Set to "true" if you're calling this function upon script shutdown
    // isHMDMode: "true" if user is in HMD; false otherwise
    // 
    // Description:
    // Used to add or remove the "SPECTATOR" app button from the HUD/tablet
    //
    var button = false;
    var buttonName = "SPECTATOR";
    var tablet = null;
    var showSpectatorInDesktop = true;
    function addOrRemoveButton(isShuttingDown, isHMDMode) {
        if (!button) {
            if ((isHMDMode || showSpectatorInDesktop) && !isShuttingDown) {
                button = tablet.addButton({
                    text: buttonName
                });
                button.clicked.connect(onTabletButtonClicked);
            }
        } else if (button) {
            if ((!showSpectatorInDesktop || isShuttingDown) && !isHMDMode) {
                button.clicked.disconnect(onTabletButtonClicked);
                tablet.removeButton(button);
                button = false;
            }
        } else {
            print("ERROR adding/removing Spectator button!");
        }
    }

    //
    // Function Name: startup()
    //
    // Relevant Variables:
    // None
    // 
    // Arguments:
    // None
    // 
    // Description:
    // startup() will be called when the script is loaded.
    //
    function startup() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        addOrRemoveButton(false, HMD.active);
        tablet.screenChanged.connect(onTabletScreenChanged);
        Window.domainChanged.connect(spectatorCameraOff);
        Window.geometryChanged.connect(resizeViewFinderOverlay);
        Controller.keyPressEvent.connect(keyPressEvent);
        HMD.displayModeChanged.connect(onHMDChanged);
        viewFinderOverlay = false;
        camera = false;
        registerButtonMappings();
    }

    //
    // Function Name: wireEventBridge()
    //
    // Relevant Variables:
    // hasEventBridge: true/false depending on whether we've already connected the event bridge
    // 
    // Arguments:
    // on: Enable or disable the event bridge
    // 
    // Description:
    // Used to connect/disconnect the script's response to the tablet's "fromQml" signal.
    //
    var hasEventBridge = false;
    function wireEventBridge(on) {
        if (on) {
            if (!hasEventBridge) {
                tablet.fromQml.connect(fromQml);
                hasEventBridge = true;
            }
        } else {
            if (hasEventBridge) {
                tablet.fromQml.disconnect(fromQml);
                hasEventBridge = false;
            }
        }
    }

    function setDisplay(showCameraView) {
        // It would be fancy if (showCameraView && !isUpdateRenderWired) would show instructions, but that's out of scope for now.
        var url = (showCameraView && isUpdateRenderWired) ? "resource://spectatorCameraFrame" : "";
        Window.setDisplayTexture(url);
    }
    const MONITOR_SHOWS_CAMERA_VIEW_DEFAULT = false;
    var monitorShowsCameraView = !!Settings.getValue('spectatorCamera/monitorShowsCameraView', MONITOR_SHOWS_CAMERA_VIEW_DEFAULT);
    function setMonitorShowsCameraView(showCameraView) {
        if (showCameraView === monitorShowsCameraView) {
            return;
        }
        monitorShowsCameraView = showCameraView;
        setDisplay(showCameraView);
        Settings.setValue('spectatorCamera/monitorShowsCameraView', showCameraView);
    }
    function setMonitorShowsCameraViewAndSendToQml(showCameraView) {
        setMonitorShowsCameraView(showCameraView);
        sendToQml({ method: 'updateMonitorShowsSwitch', params: showCameraView });
    }
    function keyPressEvent(event) {
        if ((event.text === "0") && !event.isAutoRepeat && !event.isShifted && !event.isMeta && event.isControl && !event.isAlt) {
            setMonitorShowsCameraViewAndSendToQml(!monitorShowsCameraView);
        }
    }

    //
    // Function Name: resizeViewFinderOverlay()
    //
    // Relevant Variables:
    // glassPaneRatio: The aspect ratio of the glass pane, currently set as a 16:9 aspect ratio (change if model changes)
    // verticalScale: The amount the viewFinderOverlay should be scaled if the window size is vertical
    // squareScale: The amount the viewFinderOverlay should be scaled if the window size is not vertical but is more square than the
    //     glass pane's aspect ratio
    //
    // Arguments:
    // geometryChanged: The signal argument that gives information on how the window changed, including x, y, width, and height
    //
    // Description:
    // A function called when the window is moved/resized, which changes the viewFinderOverlay's texture and dimensions to be
    //     appropriately altered to fit inside the glass pane while not distorting the texture
    //
    function resizeViewFinderOverlay(geometryChanged) {
        var glassPaneRatio = 16 / 9;
        var verticalScale = 1 / glassPaneRatio;
        var squareScale = verticalScale * (1 + (1 - (1 / (geometryChanged.width / geometryChanged.height))));

        if (geometryChanged.height > geometryChanged.width) { //vertical window size
            viewFinderOverlayDim = { x: (glassPaneWidth * verticalScale), y: (-glassPaneWidth * verticalScale), z: 0 };
        } else if ((geometryChanged.width / geometryChanged.height) < glassPaneRatio) { //square-ish window size, in-between vertical and horizontal
            viewFinderOverlayDim = { x: (glassPaneWidth * squareScale), y: (-glassPaneWidth * squareScale), z: 0 };
        } else { //horizontal window size
            viewFinderOverlayDim = { x: glassPaneWidth, y: -glassPaneWidth, z: 0 };
        }

        // The only way I found to update the viewFinderOverlay without turning the spectator camera on and off is to delete and recreate the
        //     overlay, which is inefficient but resizing the window shouldn't be performed often
        Overlays.deleteOverlay(viewFinderOverlay);
        viewFinderOverlay = Overlays.addOverlay("image3d", {
            url: "resource://spectatorCameraFrame",
            emissive: true,
            parentID: camera,
            alpha: 1,
            localRotation: { x: 0, y: 0, z: 0 },
            localPosition: { x: 0.007, y: 0.15, z: -0.005 },
            dimensions: viewFinderOverlayDim
        });
        spectatorFrameRenderConfig.resetSizeSpectatorCamera(geometryChanged.width, geometryChanged.height);
        setDisplay(monitorShowsCameraView);
    }

    const SWITCH_VIEW_FROM_CONTROLLER_DEFAULT = false;
    var switchViewFromController = !!Settings.getValue('spectatorCamera/switchViewFromController', SWITCH_VIEW_FROM_CONTROLLER_DEFAULT);
    function setControllerMappingStatus(status) {
        if (status) {
            controllerMapping.enable();
        } else {
            controllerMapping.disable();
        }
    }
    function setSwitchViewFromController(setting) {
        if (setting === switchViewFromController) {
            return;
        }
        switchViewFromController = setting;
        setControllerMappingStatus(switchViewFromController);
        Settings.setValue('spectatorCamera/switchViewFromController', setting);
    }

    //
    // Function Name: registerButtonMappings()
    //
    // Relevant Variables:
    // controllerMappingName: The name of the controller mapping
    // controllerMapping: The controller mapping itself
    // controllerType: "OculusTouch", "Vive", "Other"
    // 
    // Arguments:
    // None
    // 
    // Description:
    // Updates controller button mappings for Spectator Camera.
    //
    var controllerMappingName;
    var controllerMapping;
    var controllerType = "Other";
    function registerButtonMappings() {
        var VRDevices = Controller.getDeviceNames().toString();
        if (VRDevices.includes("Vive")) {
            controllerType = "Vive";
        } else if (VRDevices.includes("OculusTouch")) {
            controllerType = "OculusTouch";
        }

        controllerMappingName = 'Hifi-SpectatorCamera-Mapping';
        controllerMapping = Controller.newMapping(controllerMappingName);
        if (controllerType === "OculusTouch") {
            controllerMapping.from(Controller.Standard.LS).to(function (value) {
                if (value === 1.0) {
                    setMonitorShowsCameraViewAndSendToQml(!monitorShowsCameraView);
                }
                return;
            });
        } else if (controllerType === "Vive") {
            controllerMapping.from(Controller.Standard.LeftPrimaryThumb).to(function (value) {
                if (value === 1.0) {
                    setMonitorShowsCameraViewAndSendToQml(!monitorShowsCameraView);
                }
                return;
            });
        }
        setControllerMappingStatus(switchViewFromController);
        sendToQml({ method: 'updateControllerMappingCheckbox', setting: switchViewFromController, controller: controllerType });
    }

    //
    // Function Name: onTabletButtonClicked()
    //
    // Relevant Variables:
    // onSpectatorCameraScreen: true/false depending on whether we're looking at the spectator camera app
    // shouldActivateButton: true/false depending on whether we should show the button as white or gray the
    //     next time we edit the button's properties
    // 
    // Arguments:
    // None
    // 
    // Description:
    // Fired when the Spectator Camera app button is pressed.
    //
    var onSpectatorCameraScreen = false;
    var shouldActivateButton = false;
    function onTabletButtonClicked() {
        if (onSpectatorCameraScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            shouldActivateButton = true;
            tablet.loadQMLSource("../SpectatorCamera.qml");
            onSpectatorCameraScreen = true;
            sendToQml({ method: 'updateSpectatorCameraCheckbox', params: !!camera });
            sendToQml({ method: 'updateMonitorShowsSwitch', params: monitorShowsCameraView });
            sendToQml({ method: 'updateControllerMappingCheckbox', setting: switchViewFromController, controller: controllerType });
            Menu.setIsOptionChecked("Disable Preview", false);
        }
    }

    //
    // Function Name: onTabletScreenChanged()
    //
    // Relevant Variables:
    // None
    // 
    // Arguments:
    // type: "Home", "Web", "Menu", "QML", "Closed"
    // url: Only valid for Web and QML.
    // 
    // Description:
    // Called when the TabletScriptingInterface::screenChanged() signal is emitted.
    //
    function onTabletScreenChanged(type, url) {
        wireEventBridge(shouldActivateButton);
        // for toolbar mode: change button to active when window is first openend, false otherwise.
        if (button) {
            button.editProperties({ isActive: shouldActivateButton });
        }
        shouldActivateButton = false;
        onSpectatorCameraScreen = false;
    }

    //
    // Function Name: sendToQml()
    //
    // Relevant Variables:
    // None
    // 
    // Arguments:
    // message: The message to send to the SpectatorCamera QML.
    //     Messages are in format "{method, params}", like json-rpc. See also fromQml().
    // 
    // Description:
    // Use this function to send a message to the QML (i.e. to change appearances).
    //
    function sendToQml(message) {
        tablet.sendToQml(message);
    }

    //
    // Function Name: fromQml()
    //
    // Relevant Variables:
    // None
    // 
    // Arguments:
    // message: The message sent from the SpectatorCamera QML.
    //     Messages are in format "{method, params}", like json-rpc. See also sendToQml().
    // 
    // Description:
    // Called when a message is received from SpectatorCamera.qml.
    //
    function fromQml(message) {
        switch (message.method) {
            case 'spectatorCameraOn':
                spectatorCameraOn();
                break;
            case 'spectatorCameraOff':
                spectatorCameraOff();
                break;
            case 'setMonitorShowsCameraView':
                setMonitorShowsCameraView(message.params);
                break;
            case 'changeSwitchViewFromControllerPreference':
                setSwitchViewFromController(message.params);
                break;
            default:
                print('Unrecognized message from SpectatorCamera.qml:', JSON.stringify(message));
        }
    }

    //
    // Function Name: onHMDChanged()
    //
    // Relevant Variables:
    // None
    // 
    // Arguments:
    // isHMDMode: "true" if HMD is on; "false" otherwise
    // 
    // Description:
    // Called from C++ when HMD mode is changed
    //
    function onHMDChanged(isHMDMode) {
        setDisplay(monitorShowsCameraView);
        addOrRemoveButton(false, isHMDMode);
        if (!isHMDMode && !showSpectatorInDesktop) {
            spectatorCameraOff();
        }
    }

    //
    // Function Name: shutdown()
    //
    // Relevant Variables:
    // None
    // 
    // Arguments:
    // None
    // 
    // Description:
    // shutdown() will be called when the script ends (i.e. is stopped).
    //
    function shutdown() {
        spectatorCameraOff();
        Window.domainChanged.disconnect(spectatorCameraOff);
        Window.geometryChanged.disconnect(resizeViewFinderOverlay);
        addOrRemoveButton(true, HMD.active);
        tablet.screenChanged.disconnect(onTabletScreenChanged);
        HMD.displayModeChanged.disconnect(onHMDChanged);
        Controller.keyPressEvent.disconnect(keyPressEvent);
        controllerMapping.disable();
    }

    //
    // These functions will be called when the script is loaded.
    //
    startup();
    Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
