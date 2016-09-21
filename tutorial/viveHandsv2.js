Script.include("controllerDisplay.js");
Script.include("viveControllerConfiguration.js");

function debug() {
    var args = Array.prototype.slice.call(arguments);
    args.unshift("CONTROLLER DEBUG:");
    print.apply(this, args);
}

var zeroPosition = { x: 0, y: 0, z: 0 };
var zeroRotation = { x: 0, y: 0, z: 0, w: 1 };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Management of controller display                                          //
///////////////////////////////////////////////////////////////////////////////
ControllerDisplayManager = function() {
    var controllerDisplay = null;
    var activeController = null;
    var controllerCheckerIntervalID = null;

    function updateControllers() {
        if (HMD.active) {
            if ("Vive" in Controller.Hardware) {
                if (!activeController) {
                    debug("Found vive!");
                    activeController = createControllerDisplay(VIVE_CONTROLLER_CONFIGURATION);
                }
                // We've found the controllers, we no longer need to look for active controllers
                if (controllerCheckerIntervalID) {
                    Script.clearInterval(controllerCheckerIntervalID);
                    controllerCheckerIntervalID = null;
                }
            } else {
                debug("HMD active, but no controllers found");
                if (activeController) {
                    deleteControllerDisplay(activeController);
                    activeController = null;
                }
                if (controllerCheckerIntervalID == null) {
                    controllerCheckerIntervalID = Script.setInterval(updateControllers, 1000);
                }
            }
        } else {
            debug("HMD inactive");
            // We aren't in HMD mode, we no longer need to look for active controllers
            if (controllerCheckerIntervalID) {
                debug("Clearing controller checker interval");
                Script.clearInterval(controllerCheckerIntervalID);
                controllerCheckerIntervalID = null;
            }
            if (activeController) {
                debug("Deleting controller");
                deleteControllerDisplay(activeController);
                activeController = null;
            }
        }
    }

    HMD.displayModeChanged.connect(updateControllers);

    updateControllers();

    Messages.subscribe('Controller-Display');
    var handleMessages = function(channel, message, sender) {
        if (!activeController) {
            return;
        }

        if (sender === MyAvatar.sessionUUID) {
            if (channel === 'Controller-Display') {
                debug('here');
                var data = JSON.parse(message);
                var name = data.name;
                var visible = data.visible;
                //c.setDisplayAnnotation(name, visible);
                if (name in activeController.annotations) {
                    debug("hiding");
                    for (var i = 0; i < activeController.annotations[name].length; ++i) {
                        debug("hiding", i);
                        Overlays.editOverlay(activeController.annotations[name][i], { visible: visible });
                    }
                }
            } else if (channel === 'Controller-Display-Parts') {
                debug('here part');
                var data = JSON.parse(message);
                for (var name in data) {
                    var visible = data[name];
                    activeController.setPartVisible(name, visible);
                }
            } else if (channel === 'Controller-Set-Part-Layer') {
                var data = JSON.parse(message);
                for (var name in data) {
                    var layer = data[name];
                    activeController.setLayerForPart(name, layer);
                }
            }
        }
    }

    Messages.messageReceived.connect(handleMessages);

    this.destroy = function() {
        print("Destroying controller display");
        Messages.messageReceived.disconnect(handleMessages);
        if (activeController) {
            deleteControllerDisplay(activeController);
        }
    };

}

//var c = setupController(TOUCH_CONTROLLER_CONFIGURATION);
//var c = createControllerDisplay(VIVE_CONTROLLER_CONFIGURATION);
//c.setPartVisible("touchpad", false);
//c.setPartVisible("touchpad_teleport", false);
//layers = ["blank", "teleport", 'arrows'];
//num = 0;
//Script.setInterval(function() {
//    print('num', num);
//    num = (num + 1) % layers.length;
//    c.setLayerForPart("touchpad", layers[num]);
//}, 2000);
//
