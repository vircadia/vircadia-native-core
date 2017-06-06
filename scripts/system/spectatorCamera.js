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
        return Vec3.sum(position || MyAvatar.position,
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
    // 
    // Arguments:
    // None
    // 
    // Description:
    // The update function for the spectator camera. Modifies the camera's position
    //     and orientation.
    //
    var spectatorFrameRenderConfig = Render.getConfig("SelfieFrame");
    var beginSpectatorFrameRenderConfig = Render.getConfig("BeginSelfie");
    var viewFinderOverlay = false;
    var camera = false;
    function updateRenderFromCamera() {
        var cameraData = Entities.getEntityProperties(camera, ['position', 'rotation']);
        // FIXME: don't muck with config if properties haven't changed.
        beginSpectatorFrameRenderConfig.position = cameraData.position;
        beginSpectatorFrameRenderConfig.orientation = cameraData.rotation;
        // BUG: image3d overlays don't retain their locations properly when parented to a dynamic object
        Overlays.editOverlay(viewFinderOverlay, { orientation: flip(cameraData.rotation) });
    }

    //
    // Function Name: enableSpectatorCamera()
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
    function enableSpectatorCamera() {
        // Set the special texture size based on the window in which it will eventually be displayed.
        var size = Controller.getViewportDimensions(); // FIXME: Need a signal to hook into when the dimensions change.
        spectatorFrameRenderConfig.resetSize(size.x, size.y);
        spectatorFrameRenderConfig.enabled = beginSpectatorFrameRenderConfig.enabled = true;
        var cameraRotation = MyAvatar.orientation, cameraPosition = inFrontOf(2);
        Script.update.connect(updateRenderFromCamera);
        isUpdateRenderWired = true;
        camera = Entities.addEntity({
            type: 'Box',
            dimensions: { x: 0.4, y: 0.2, z: 0.4 },
            userData: '{"grabbableKey":{"grabbable":true}}',
            dynamic: true,
            color: { red: 255, green: 0, blue: 0 },
            name: 'SpectatorCamera',
            position: cameraPosition, // Put the camera in front of me so that I can find it.
            rotation: cameraRotation
        }); // FIXME: avatar entity so that you don't need rez rights.;
        // Put an image3d overlay on the near face, as a viewFinder.
        viewFinderOverlay = Overlays.addOverlay("image3d", {
            url: "http://selfieFrame",
            //url: "http://1.bp.blogspot.com/-1GABEq__054/T03B00j_OII/AAAAAAAAAa8/jo55LcvEPHI/s1600/Winning.jpg",
            parentID: camera,
            alpha: 1,
            position: inFrontOf(-0.25, cameraPosition, cameraRotation),
            // FIXME: We shouldn't need the flip and the negative scale.
            // e.g., This isn't necessary using an ordinary .jpg with lettering, above.
            // Must be something about the view frustum projection matrix?
            // But don't go changing that in (c++ code) without getting all the way to a desktop display!
            orientation: flip(cameraRotation),
            scale: -0.35,
        });
    }

    //
    // Function Name: disableSpectatorCamera()
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
    function disableSpectatorCamera() {
        spectatorFrameRenderConfig.enabled = beginSpectatorFrameRenderConfig.enabled = false;
        if (isUpdateRenderWired) {
            Script.update.disconnect(updateRenderFromCamera);
            isUpdateRenderWired = false;
        }
        if (viewFinderOverlay) {
            Overlays.deleteOverlay(viewFinderOverlay);
            viewFinderOverlay = false;
        }
        if (camera) {
            Entities.deleteEntity(camera);
            camera = false;
        }
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
            case 'enableSpectatorCamera':
                enableSpectatorCamera();
                break;
            case 'disableSpectatorCamera':
                disableSpectatorCamera();
                break;
            case 'showHmdPreviewOnMonitor':
                print('FIXME: showHmdPreviewOnMonitor');
                Settings.setValue('spectatorCamera/monitorShowsCameraView', false);
                break;
            case 'showCameraViewOnMonitor':
                print('FIXME: showCameraViewOnMonitor');
                Settings.setValue('spectatorCamera/monitorShowsCameraView', true);
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
        disableSpectatorCamera();
        tablet.removeButton(button);
        button.clicked.disconnect(onTabletButtonClicked);
        tablet.screenChanged.disconnect(onTabletScreenChanged);
    }

    //
    // These functions will be called when the script is loaded.
    //
    startup();
    Script.scriptEnding.connect(shutdown);

}()); // END LOCAL_SCOPE
