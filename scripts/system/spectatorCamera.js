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
    var sendToQml, onTabletScreenChanged, fromQml, onTabletButtonClicked, wireEventBridge, startup, shutdown;



    //
    // Function Name: inFrontOf(), flip()
    // 
    // Description:
    // Spectator camera utility functions and variables.
    //
    function inFrontOf(distance, position, orientation) {
        return Vec3.sum(position || Vec3.sum(MyAvatar.position, { x: 0, y: 0.3, z: 0 }),
                        Vec3.multiply(distance, Quat.getForward(orientation || MyAvatar.orientation)));
    }
    var aroundY = Quat.fromPitchYawRollDegrees(0, 180, 0);
    function flip(rotation) { return Quat.multiply(rotation, aroundY); }

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
        if (JSON.stringify(lastCameraPosition) !== JSON.stringify(cameraData.position)) {
            lastCameraPosition = cameraData.position;
            beginSpectatorFrameRenderConfig.position = lastCameraPosition;
        }
        if (JSON.stringify(lastCameraRotation) !== JSON.stringify(cameraData.rotation)) {
            lastCameraRotation = cameraData.rotation;
            beginSpectatorFrameRenderConfig.orientation = lastCameraRotation;
        }
        if (cameraIsDynamic) {
            // BUG: image3d overlays don't retain their locations properly when parented to a dynamic object
            Overlays.editOverlay(viewFinderOverlay, { orientation: flip(cameraData.rotation) });
        }
    }

    //
    // Function Name: spectatorCameraOn()
    //
    // Relevant Variables:
    // isUpdateRenderWired: Bool storing whether or not the camera's update
    //     function is wired.
    // 
    // Arguments:
    // None
    // 
    // Description:
    // Call this function to set up the spectator camera and
    //     spawn the camera entity.
    //
    var isUpdateRenderWired = false;
    function spectatorCameraOn() {
        // Set the special texture size based on the window in which it will eventually be displayed.
        var size = Controller.getViewportDimensions(); // FIXME: Need a signal to hook into when the dimensions change.
        spectatorFrameRenderConfig.resetSizeSpectatorCamera(size.x, size.y);
        spectatorFrameRenderConfig.enabled = beginSpectatorFrameRenderConfig.enabled = true;
        var cameraRotation = MyAvatar.orientation, cameraPosition = inFrontOf(2);
        Script.update.connect(updateRenderFromCamera);
        isUpdateRenderWired = true;
        camera = Entities.addEntity({
            "angularDamping": 0.98000001907348633,
            "collisionsWillMove": 1,
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
            "rotation": cameraRotation,
            "position": cameraPosition,
            "shapeType": "simple-compound",
            "type": "Model",
            "userData": "{\"grabbableKey\":{\"grabbable\":true}}"
        }, true);
        // Put an image3d overlay on the near face, as a viewFinder.
        viewFinderOverlay = Overlays.addOverlay("image3d", {
            url: "resource://spectatorCameraFrame",
            parentID: camera,
            alpha: 1,
            position: inFrontOf(0, Vec3.sum(cameraPosition, { x: 0, y: 0.15, z: 0 }), cameraRotation),
            // FIXME: We shouldn't need the flip and the negative scale.
            // e.g., This isn't necessary using an ordinary .jpg with lettering, above.
            // Must be something about the view frustum projection matrix?
            // But don't go changing that in (c++ code) without getting all the way to a desktop display!
            orientation: flip(cameraRotation),
            scale: -0.16,
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

    function onHMDChanged(isHMDMode) {
        // Will also eventually enable disable app, camera, etc.
        setDisplay(monitorShowsCameraView);
    }

    //
    // Function Name: startup()
    //
    // Relevant Variables:
    // button: The tablet button.
    // buttonName: The name of the button.
    // tablet: The tablet instance to be modified.
    // 
    // Arguments:
    // None
    // 
    // Description:
    // startup() will be called when the script is loaded.
    //
    var button;
    var buttonName = "SPECTATOR";
    var tablet = null;
    function startup() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        button = tablet.addButton({
            text: buttonName
        });
        button.clicked.connect(onTabletButtonClicked);
        tablet.screenChanged.connect(onTabletScreenChanged);
        Window.domainChanged.connect(spectatorCameraOff);
        Controller.keyPressEvent.connect(keyPressEvent);
        HMD.displayModeChanged.connect(onHMDChanged);
        viewFinderOverlay = false;
        camera = false;
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
        var url = (showCameraView && isUpdateRenderWired) ? "http://selfieFrame" : "";
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
        button.editProperties({ isActive: shouldActivateButton });
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
                print('FIXME: Preference is now: ' + message.params);
                break;
            default:
                print('Unrecognized message from SpectatorCamera.qml:', JSON.stringify(message));
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
        tablet.removeButton(button);
        button.clicked.disconnect(onTabletButtonClicked);
        tablet.screenChanged.disconnect(onTabletScreenChanged);
        HMD.displayModeChanged.disconnect(onHMDChanged);
        Controller.keyPressEvent.disconnect(keyPressEvent);
    }

    //
    // These functions will be called when the script is loaded.
    //
    startup();
    Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
