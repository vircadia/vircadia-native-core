//
//  Created by Anthony J. Thibault on 2017/06/20
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  When this script is running, a new app button, named "PUCKTACH", will be added to the toolbar/tablet.
//  Click this app to bring up the puck attachment panel.  This panel contains the following fields.
//
//  * Tracked Object - A drop down list of all the available pucks found.  If no pucks are found this list will only have a single NONE entry.
//    Closing and re-opening the app will refresh this list.
//  * Model URL - A model url of the model you wish to be attached to the specified puck.
//  * Position X, Y, Z - used to apply an offset between the puck and the attached model.
//  * Rot X, Y, Z - used to apply euler angle offsets, in degrees, between the puck and the attached model.
//  * Create Attachment - when this button is pressed a new Entity will be created at the specified puck's location.
//    If a puck atttachment already exists, it will be destroyed before the new entity is created.
//  * Destroy Attachmetn - destroies the entity attached to the puck.
//

/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
/* global Xform */
Script.include("/~/system/libraries/Xform.js");

(function() { // BEGIN LOCAL_SCOPE

var TABLET_BUTTON_NAME = "PUCKTACH";
var HTML_URL = "https://s3.amazonaws.com/hifi-public/tony/html/puck-attach.html";

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var tabletButton = tablet.addButton({
    text: TABLET_BUTTON_NAME,
    icon: "https://s3.amazonaws.com/hifi-public/tony/icons/puck-i.svg",
    activeIcon: "https://s3.amazonaws.com/hifi-public/tony/icons/puck-a.svg"
});

tabletButton.clicked.connect(function () {
    if (shown) {
        tablet.gotoHomeScreen();
    } else {
        tablet.gotoWebScreen(HTML_URL);
    }
});

var shown = false;
var attachedEntity;
var attachedObj;

function onScreenChanged(type, url) {
    if (type === "Web" && url === HTML_URL) {
        tabletButton.editProperties({isActive: true});
        if (!shown) {
            // hook up to event bridge
            tablet.webEventReceived.connect(onWebEventReceived);

            Script.setTimeout(function () {
                // send available tracked objects to the html running in the tablet.
                var availableTrackedObjects = getAvailableTrackedObjects();
                tablet.emitScriptEvent(JSON.stringify(availableTrackedObjects));

                // print("PUCK-ATTACH: availableTrackedObjects = " + JSON.stringify(availableTrackedObjects));
            }, 1000);  // wait 1 sec before sending..
        }
        shown = true;
    } else {
        tabletButton.editProperties({isActive: false});
        if (shown) {
            // disconnect from event bridge
            tablet.webEventReceived.disconnect(onWebEventReceived);
        }
        shown = false;
    }
}

tablet.screenChanged.connect(onScreenChanged);

function indexToTrackedObjectName(index) {
    return "TrackedObject" + pad(index, 2);
}

function getAvailableTrackedObjects() {
    var available = [];
    var NUM_TRACKED_OBJECTS = 16;
    var i;
    for (i = 0; i < NUM_TRACKED_OBJECTS; i++) {
        var key = indexToTrackedObjectName(i);
        var pose = Controller.getPoseValue(Controller.Hardware.Vive[key]);
        if (pose && pose.valid) {
            available.push(i);
        }
    }
    return available;
}

function attach(obj) {
    attachedEntity = Entities.addEntity({
        type: "Model",
        name: "puck-attach-entity",
        modelURL: obj.modelurl
    });
    attachedObj = obj;
    var localPos = {x: Number(obj.posx), y: Number(obj.posy), z: Number(obj.posz)};
    var localRot = Quat.fromVec3Degrees({x: Number(obj.rotx), y: Number(obj.roty), z: Number(obj.rotz)});
    attachedObj.localXform = new Xform(localRot, localPos);
    var key = indexToTrackedObjectName(Number(attachedObj.puckno));
    attachedObj.key = key;

    // print("PUCK-ATTACH: attachedObj = " + JSON.stringify(attachedObj));

    Script.update.connect(update);
    update();
}

function remove() {
    if (attachedEntity) {
        Script.update.disconnect(update);
        Entities.deleteEntity(attachedEntity);
        attachedEntity = undefined;
    }
    attachedObj = undefined;
}

function pad(num, size) {
    var tempString = "000000000" + num;
    return tempString.substr(tempString.length - size);
}

function update() {
    if (attachedEntity && attachedObj && Controller.Hardware.Vive) {
        var pose = Controller.getPoseValue(Controller.Hardware.Vive[attachedObj.key]);
        var avatarXform = new Xform(MyAvatar.orientation, MyAvatar.position);
        var puckXform = new Xform(pose.rotation, pose.translation);
        var finalXform = Xform.mul(avatarXform, Xform.mul(puckXform, attachedObj.localXform));
        if (pose && pose.valid) {
            Entities.editEntity(attachedEntity, {
                position: finalXform.pos,
                rotation: finalXform.rot
            });
        } else {
            if (pose) {
                print("PUCK-ATTACH: WARNING: invalid pose for " + attachedObj.key);
            } else {
                print("PUCK-ATTACH: WARNING: could not find key " + attachedObj.key);
            }
        }
    }
}

function onWebEventReceived(msg) {
    var obj = {};
    try {
        obj = JSON.parse(msg);
    } catch (err) {
        return;
    }
    if (obj.cmd === "attach") {
        remove();
        attach(obj);
    } else if (obj.cmd === "detach") {
        remove();
    }
}

Script.scriptEnding.connect(function () {
    remove();
    tablet.removeButton(tabletButton);
    if (shown) {
        tablet.webEventReceived.disconnect(onWebEventReceived);
        tablet.gotoHomeScreen();
    }
    tablet.screenChanged.disconnect(onScreenChanged);
});

}()); // END LOCAL_SCOPE
