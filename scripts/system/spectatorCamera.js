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
    // windowAspectRatio: The ratio of the Interface windows's sizeX/sizeY
    // previewAspectRatio: The ratio of the camera preview's sizeX/sizeY
    // vFoV: The vertical field of view of the spectator camera
    // nearClipPlaneDistance: The near clip plane distance of the spectator camera
    // farClipPlaneDistance: The far clip plane distance of the spectator camera
    // 
    // Arguments:
    // None
    // 
    // Description:
    // Call this function to set up the spectator camera and
    //     spawn the camera entity.
    //
    var isUpdateRenderWired = false;
    var windowAspectRatio;
    var previewAspectRatio = 16 / 9;
    var vFoV = 45.0;
    var nearClipPlaneDistance = 0.1;
    var farClipPlaneDistance = 100.0;
    function spectatorCameraOn() {
        // Set the special texture size based on the window in which it will eventually be displayed.
        var size = Controller.getViewportDimensions(); // FIXME: Need a signal to hook into when the dimensions change.
        var sizeX = Window.innerWidth;
        var sizeY = Window.innerHeight;
        windowAspectRatio = sizeX/sizeY;
        spectatorFrameRenderConfig.resetSizeSpectatorCamera(sizeX, sizeY);
        spectatorFrameRenderConfig.enabled = beginSpectatorFrameRenderConfig.enabled = true;
        beginSpectatorFrameRenderConfig.vFoV = vFoV;
        beginSpectatorFrameRenderConfig.nearClipPlaneDistance = nearClipPlaneDistance;
        beginSpectatorFrameRenderConfig.farClipPlaneDistance = farClipPlaneDistance;
        var cameraRotation = MyAvatar.orientation, cameraPosition = inFrontOf(1, Vec3.sum(MyAvatar.position, { x: 0, y: 0.3, z: 0 }));
        Script.update.connect(updateRenderFromCamera);
        isUpdateRenderWired = true;
        camera = Entities.addEntity({
            "angularDamping": 0.98000001907348633,
            "collisionsWillMove": 0,
            "damping": 0.98000001907348633,
            "dimensions": {
                "x": 0.2338641881942749,
                "y": 0.407032310962677,
                "z": 0.38702544569969177
            },
            "dynamic": cameraIsDynamic,
            "modelURL": "http://hifi-content.s3.amazonaws.com/alan/dev/spectator-camera.fbx",
            "queryAACube": {
                "scale": 0.60840487480163574,
                "x": -0.30420243740081787,
                "y": -0.30420243740081787,
                "z": -0.30420243740081787
            },
            "rotation": { x: 0, y: 0, z: 0 },
            "position": { x: 0, y: 0, z: 0 },
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
            position: { x: 0.007, y: 0.15, z: -0.005 },
            dimensions: { x: 0.16, y: -0.16 * windowAspectRatio / previewAspectRatio, z: 0 }
            // Negative dimension for viewfinder is necessary for now due to the way Image3DOverlay
            // draws textures.
            // See Image3DOverlay.cpp:91. If you change the two lines there to:
            //     glm::vec2 topLeft(-x, -y);
            //     glm::vec2 bottomRight(x, y);
            // the viewfinder will appear rightside up without this negative y-dimension.
            // However, other Image3DOverlay textures (like the PAUSED one) will appear upside-down. *Why?*
            // FIXME: This code will stretch the preview as the window aspect ratio changes. Fix that!
        });
        Entities.editEntity(camera, { position: cameraPosition, rotation: cameraRotation });
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

    const SWITCH_VIEW_FROM_CONTROLLER_DEFAULT = false;
    var switchViewFromController = !!Settings.getValue('spectatorCamera/switchViewFromController', SWITCH_VIEW_FROM_CONTROLLER_DEFAULT);
    function setControllerMappingStatus(status) {
        if (status === true) {
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
        var controllersDeviceNames = Controller.getDeviceNames();
        if (controllersDeviceNames.indexOf("OculusTouch") !== -1) {
            controllerType = "OculusTouch";
        } else if (controllerDeviceNames.indexOf("Vive") !== -1) {
            controllerType = "Vive";
        }

        controllerMappingName = 'Hifi-SpectatorCamera-Mapping-' + Math.random();
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
        sendToQml({ method: 'updateControllerMappingCheckbox', params: switchViewFromController });
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
            sendToQml({ method: 'updateMonitorShowsSwitch', params: !!Settings.getValue('spectatorCamera/monitorShowsCameraView', false) });
            setMonitorShowsCameraViewAndSendToQml(monitorShowsCameraView);
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
