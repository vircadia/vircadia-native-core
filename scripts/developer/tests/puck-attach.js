//
//  Created by Anthony J. Thibault on 2017/06/20
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
    icon: "https://s3.amazonaws.com/hifi-public/tony/icons/tpose-i.svg",
    activeIcon: "https://s3.amazonaws.com/hifi-public/tony/icons/tpose-a.svg"
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
    var key = "TrackedObject" + pad(attachedObj.puckno, 2);
    attachedObj.key = key;

    print("AJT: attachedObj = " + JSON.stringify(attachedObj));

    Script.update.connect(update);
    update(0.001);
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
    var s = "000000000" + num;
    return s.substr(s.length-size);
}

function update(dt) {
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
                print("AJT: WARNING: invalid pose for " + attachedObj.key);
            } else {
                print("AJT: WARNING: could not find key " + attachedObj.key);
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
