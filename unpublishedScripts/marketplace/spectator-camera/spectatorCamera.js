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

    // FUNCTION VAR DECLARATIONS
    var sendToQml, addOrRemoveButton, onTabletScreenChanged, fromQml,
        onTabletButtonClicked, wireEventBridge, startup, shutdown, registerButtonMappings;

    // Function Name: inFrontOf()
    //
    // Description:
    //   -Returns the position in front of the given "position" argument, where the forward vector is based off
    //    the "orientation" argument and the amount in front is based off the "distance" argument.
    function inFrontOf(distance, position, orientation) {
        return Vec3.sum(position || MyAvatar.position,
                        Vec3.multiply(distance, Quat.getForward(orientation || MyAvatar.orientation)));
    }

    // Function Name: spectatorCameraOn()
    //
    // Description:
    //   -Call this function to set up the spectator camera and
    //    spawn the camera entity.
    //
    // Relevant Variables:
    //   -spectatorCameraConfig: The render configuration of the spectator camera
    //    render job. It controls various attributes of the Secondary Camera, such as:
    //      -The entity ID to follow
    //      -Position
    //      -Orientation
    //      -Rendered texture size
    //      -Vertical field of view
    //      -Near clip plane distance
    //      -Far clip plane distance
    //   -viewFinderOverlay: The in-world overlay that displays the spectator camera's view.
    //   -camera: The in-world entity that corresponds to the spectator camera.
    //   -cameraIsDynamic: "false" for now - maybe it shouldn't be? False means that the camera won't drift when you let go...
    //   -cameraRotation: The rotation of the spectator camera.
    //   -cameraPosition: The position of the spectator camera.
    //   -glassPaneWidth: The width of the glass pane above the spectator camera that holds the viewFinderOverlay.
    //   -viewFinderOverlayDim: The x, y, and z dimensions of the viewFinderOverlay.
    //   -camera: The camera model which is grabbable.
    //   -viewFinderOverlay: The preview of what the spectator camera is viewing, placed inside the glass pane.
    var spectatorCameraConfig = Render.getConfig("SecondaryCamera");
    var viewFinderOverlay = false;
    var camera = false;
    var cameraIsDynamic = false;
    var cameraRotation;
    var cameraPosition;
    var glassPaneWidth = 0.16;
    // The negative y dimension for viewFinderOverlay is necessary for now due to the way Image3DOverlay
    // draws textures, but should be looked into at some point. Also the z dimension shouldn't affect
    // the overlay since it is an Image3DOverlay so it is set to 0.
    var viewFinderOverlayDim = { x: glassPaneWidth, y: -glassPaneWidth, z: 0 };
    function spectatorCameraOn() {
        // Sets the special texture size based on the window it is displayed in, which doesn't include the menu bar
        spectatorCameraConfig.enableSecondaryCameraRenderConfigs(true);
        spectatorCameraConfig.resetSizeSpectatorCamera(Window.innerWidth, Window.innerHeight);
        cameraRotation = Quat.multiply(MyAvatar.orientation, Quat.fromPitchYawRollDegrees(15, -155, 0)), cameraPosition = inFrontOf(0.85, Vec3.sum(MyAvatar.position, { x: 0, y: 0.28, z: 0 }));
        camera = Entities.addEntity({
            "angularDamping": 1,
            "damping": 1,
            "collidesWith": "static,dynamic,kinematic,",
            "collisionMask": 7,
            "dynamic": cameraIsDynamic,
            "modelURL": Script.resolvePath("spectator-camera.fbx"),
            "registrationPoint": {
                "x": 0.56,
                "y": 0.545,
                "z": 0.23
            },
            "rotation": cameraRotation,
            "position": cameraPosition,
            "shapeType": "simple-compound",
            "type": "Model",
            "userData": "{\"grabbableKey\":{\"grabbable\":true}}"
        }, true);
        spectatorCameraConfig.attachedEntityId = camera;
        updateOverlay();
        setDisplay(monitorShowsCameraView);
        // Change button to active when window is first openend OR if the camera is on, false otherwise.
        if (button) {
            button.editProperties({ isActive: onSpectatorCameraScreen || camera });
        }
        Audio.playSound(CAMERA_ON_SOUND, {
            volume: 0.15,
            position: cameraPosition,
            localOnly: true
        });
    }

    // Function Name: spectatorCameraOff()
    //
    // Description:
    //   -Call this function to shut down the spectator camera and
    //    destroy the camera entity. "isChangingDomains" is true when this function is called
    //    from the "Window.domainChanged()" signal.
    var WAIT_AFTER_DOMAIN_SWITCH_BEFORE_CAMERA_DELETE_MS = 1 * 1000;
    function spectatorCameraOff(isChangingDomains) {
        function deleteCamera() {
            Entities.deleteEntity(camera);
            camera = false;
            if (button) {
                // Change button to active when window is first openend OR if the camera is on, false otherwise.
                button.editProperties({ isActive: onSpectatorCameraScreen || camera });
            }
        }

        spectatorCameraConfig.attachedEntityId = false;
        spectatorCameraConfig.enableSecondaryCameraRenderConfigs(false);
        if (camera) {
            // Workaround for Avatar Entities not immediately having properties after
            // the "Window.domainChanged()" signal is emitted.
            // Should be removed after FB6155 is fixed.
            if (isChangingDomains) {
                Script.setTimeout(function () {
                    deleteCamera();
                    spectatorCameraOn();
                }, WAIT_AFTER_DOMAIN_SWITCH_BEFORE_CAMERA_DELETE_MS);
            } else {
                deleteCamera();
            }
        }
        if (viewFinderOverlay) {
            Overlays.deleteOverlay(viewFinderOverlay);
        }
        viewFinderOverlay = false;
        setDisplay(monitorShowsCameraView);
    }

    // Function Name: addOrRemoveButton()
    //
    // Description:
    //   -Used to add or remove the "SPECTATOR" app button from the HUD/tablet. Set the "isShuttingDown" argument
    //    to true if you're calling this function upon script shutdown. Set the "isHMDmode" to true if the user is
    //    in HMD; otherwise set to false.
    //
    // Relevant Variables:
    //   -button: The tablet button.
    //   -buttonName: The name of the button.
    //   -showSpectatorInDesktop: Set to "true" to show the "SPECTATOR" app in desktop mode.
    var button = false;
    var buttonName = "SPECTATOR";
    var showSpectatorInDesktop = false;
    function addOrRemoveButton(isShuttingDown, isHMDMode) {
        if (!tablet) {
            print("Warning in addOrRemoveButton(): 'tablet' undefined!");
            return;
        }
        if (!button) {
            if ((isHMDMode || showSpectatorInDesktop) && !isShuttingDown) {
                button = tablet.addButton({
                    text: buttonName,
                    icon: "icons/tablet-icons/spectator-i.svg",
                    activeIcon: "icons/tablet-icons/spectator-a.svg"
                });
                button.clicked.connect(onTabletButtonClicked);
            }
        } else if (button) {
            if ((!isHMDMode && !showSpectatorInDesktop) || isShuttingDown) {
                button.clicked.disconnect(onTabletButtonClicked);
                tablet.removeButton(button);
                button = false;
            }
        } else {
            print("ERROR adding/removing Spectator button!");
        }
    }

    // Function Name: startup()
    //
    // Description:
    //   -startup() will be called when the script is loaded.
    //
    // Relevant Variables:
    //   -tablet: The tablet instance to be modified.
    var tablet = null;
    function startup() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        addOrRemoveButton(false, HMD.active);
        tablet.screenChanged.connect(onTabletScreenChanged);
        Window.domainChanged.connect(onDomainChanged);
        Window.geometryChanged.connect(resizeViewFinderOverlay);
        Controller.keyPressEvent.connect(keyPressEvent);
        HMD.displayModeChanged.connect(onHMDChanged);
        viewFinderOverlay = false;
        camera = false;
        registerButtonMappings();
    }

    // Function Name: wireEventBridge()
    //
    // Description:
    //   -Used to connect/disconnect the script's response to the tablet's "fromQml" signal. Set the "on" argument to enable or
    //    disable to event bridge.
    //
    // Relevant Variables:
    //   -hasEventBridge: true/false depending on whether we've already connected the event bridge.
    var hasEventBridge = false;
    function wireEventBridge(on) {
        if (!tablet) {
            print("Warning in wireEventBridge(): 'tablet' undefined!");
            return;
        }
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

    // Function Name: setDisplay()
    //
    // Description:
    //   -There are two bool variables that determine what the "url" argument to "setDisplayTexture(url)" should be:
    //     Camera on/off switch, and the "Monitor Shows" on/off switch.
    //     This results in four possible cases for the argument. Those four cases are:
    //     1. Camera is off; "Monitor Shows" is "HMD Preview": "url" is ""
    //     2. Camera is off; "Monitor Shows" is "Camera View": "url" is ""
    //     3. Camera is on; "Monitor Shows" is "HMD Preview":  "url" is ""
    //     4. Camera is on; "Monitor Shows" is "Camera View":  "url" is "resource://spectatorCameraFrame"
    function setDisplay(showCameraView) {

        var url = (camera) ? (showCameraView ? "resource://spectatorCameraFrame" : "resource://hmdPreviewFrame") : "";
        sendToQml({ method: 'showPreviewTextureNotInstructions', setting: !!url, url: url});

        // FIXME: temporary hack to avoid setting the display texture to hmdPreviewFrame
        // until it is the correct mono.
        if (url === "resource://hmdPreviewFrame") {
            Window.setDisplayTexture("");
        } else {
            Window.setDisplayTexture(url);
        }
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
    function updateOverlay() {
        // The only way I found to update the viewFinderOverlay without turning the spectator camera on and off is to delete and recreate the
        //     overlay, which is inefficient but resizing the window shouldn't be performed often
        if (viewFinderOverlay) {
            Overlays.deleteOverlay(viewFinderOverlay);
        }
        viewFinderOverlay = Overlays.addOverlay("image3d", {
            url: "resource://spectatorCameraFrame",
            emissive: true,
            parentID: camera,
            alpha: 1,
            localRotation: { w: 1, x: 0, y: 0, z: 0 },
            localPosition: { x: 0, y: 0.13, z: 0.126 },
            dimensions: viewFinderOverlayDim
        });
    }

    // Function Name: resizeViewFinderOverlay()
    //
    // Description:
    //   -A function called when the window is moved/resized, which changes the viewFinderOverlay's texture and dimensions to be
    //    appropriately altered to fit inside the glass pane while not distorting the texture. The "geometryChanged" argument gives information
    //    on how the window changed, including x, y, width, and height.
    //
    // Relevant Variables:
    //   -glassPaneRatio: The aspect ratio of the glass pane, currently set as a 16:9 aspect ratio (change if model changes).
    //   -verticalScale: The amount the viewFinderOverlay should be scaled if the window size is vertical.
    //   -squareScale: The amount the viewFinderOverlay should be scaled if the window size is not vertical but is more square than the
    //     glass pane's aspect ratio.
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
        updateOverlay();
        spectatorCameraConfig.resetSizeSpectatorCamera(geometryChanged.width, geometryChanged.height);
        setDisplay(monitorShowsCameraView);
    }

    const SWITCH_VIEW_FROM_CONTROLLER_DEFAULT = false;
    var switchViewFromController = !!Settings.getValue('spectatorCamera/switchViewFromController', SWITCH_VIEW_FROM_CONTROLLER_DEFAULT);
    function setControllerMappingStatus(status) {
        if (!controllerMapping) {
            return;
        }
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

    // Function Name: registerButtonMappings()
    //
    // Description:
    //   -Updates controller button mappings for Spectator Camera.
    //
    // Relevant Variables:
    //   -controllerMappingName: The name of the controller mapping.
    //   -controllerMapping: The controller mapping itself.
    //   -controllerType: "OculusTouch", "Vive", "Other".
    var controllerMappingName;
    var controllerMapping;
    var controllerType = "Other";
    function registerButtonMappings() {
        var VRDevices = Controller.getDeviceNames().toString();
        if (VRDevices) {
            if (VRDevices.indexOf("Vive") !== -1) {
                controllerType = "Vive";
            } else if (VRDevices.indexOf("OculusTouch") !== -1) {
                controllerType = "OculusTouch";
            } else {
                sendToQml({ method: 'updateControllerMappingCheckbox', setting: switchViewFromController, controller: controllerType });
                return; // Neither Vive nor Touch detected
            }
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

    // Function Name: onTabletButtonClicked()
    //
    // Description:
    //   -Fired when the Spectator Camera app button is pressed.
    //
    // Relevant Variables:
    //   -SPECTATOR_CAMERA_QML_SOURCE: The path to the SpectatorCamera QML
    //   -onSpectatorCameraScreen: true/false depending on whether we're looking at the spectator camera app.
    var SPECTATOR_CAMERA_QML_SOURCE = Script.resourcesPath() + "qml/hifi/SpectatorCamera.qml";
    var onSpectatorCameraScreen = false;
    function onTabletButtonClicked() {
        if (!tablet) {
            print("Warning in onTabletButtonClicked(): 'tablet' undefined!");
            return;
        }
        if (onSpectatorCameraScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            tablet.loadQMLSource(SPECTATOR_CAMERA_QML_SOURCE);
            sendToQml({ method: 'updateSpectatorCameraCheckbox', params: !!camera });
            sendToQml({ method: 'updateMonitorShowsSwitch', params: monitorShowsCameraView });
            if (!controllerMapping) {
                registerButtonMappings();
            } else {
                sendToQml({ method: 'updateControllerMappingCheckbox', setting: switchViewFromController, controller: controllerType });
            }
            Menu.setIsOptionChecked("Disable Preview", false);
            Menu.setIsOptionChecked("Mono Preview", true);
        }
    }

    // Function Name: onTabletScreenChanged()
    //
    // Description:
    //   -Called when the TabletScriptingInterface::screenChanged() signal is emitted. The "type" argument can be either the string
    //    value of "Home", "Web", "Menu", "QML", or "Closed". The "url" argument is only valid for Web and QML.
    function onTabletScreenChanged(type, url) {
        onSpectatorCameraScreen = (type === "QML" && url === SPECTATOR_CAMERA_QML_SOURCE);
        wireEventBridge(onSpectatorCameraScreen);
        // Change button to active when window is first openend OR if the camera is on, false otherwise.
        if (button) {
            button.editProperties({ isActive: onSpectatorCameraScreen || camera });
        }
    }

    // Function Name: sendToQml()
    //
    // Description:
    //   -Use this function to send a message to the QML (i.e. to change appearances). The "message" argument is what is sent to
    //    SpectatorCamera QML in the format "{method, params}", like json-rpc. See also fromQml().
    function sendToQml(message) {
        tablet.sendToQml(message);
    }

    // Function Name: fromQml()
    //
    // Description:
    //   -Called when a message is received from SpectatorCamera.qml. The "message" argument is what is sent from the SpectatorCamera QML
    //    in the format "{method, params}", like json-rpc. See also sendToQml().
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

    // Function Name: onHMDChanged()
    //
    // Description:
    //   -Called from C++ when HMD mode is changed. The argument "isHMDMode" is true if HMD is on; false otherwise.
    function onHMDChanged(isHMDMode) {
        if (!controllerMapping) {
            registerButtonMappings();
        }
        setDisplay(monitorShowsCameraView);
        addOrRemoveButton(false, isHMDMode);
        if (!isHMDMode && !showSpectatorInDesktop) {
            spectatorCameraOff();
        }
    }

    // Function Name: shutdown()
    //
    // Description:
    //   -shutdown() will be called when the script ends (i.e. is stopped).
    function shutdown() {
        spectatorCameraOff();
        Window.domainChanged.disconnect(onDomainChanged);
        Window.geometryChanged.disconnect(resizeViewFinderOverlay);
        addOrRemoveButton(true, HMD.active);
        if (tablet) {
            tablet.screenChanged.disconnect(onTabletScreenChanged);
            if (onSpectatorCameraScreen) {
                tablet.gotoHomeScreen();
            }
        }
        HMD.displayModeChanged.disconnect(onHMDChanged);
        Controller.keyPressEvent.disconnect(keyPressEvent);
        if (controllerMapping) {
            controllerMapping.disable();
        }
    }

    // Function Name: onDomainChanged()
    //
    // Description:
    //   -A small utility function used when the Window.domainChanged() signal is fired.
    function onDomainChanged() {
        spectatorCameraOff(true);
    }

    // These functions will be called when the script is loaded.
    var CAMERA_ON_SOUND = SoundCache.getSound(Script.resolvePath("cameraOn.wav"));
    startup();
    Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
