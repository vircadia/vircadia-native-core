//
//  attachedEntitiesManager.js
//
//  Created by Seth Alves on 2016-1-20
//  Copyright 2016 High Fidelity, Inc.
//
//  This script handles messages from the grab script related to wearables, and interacts with a doppelganger.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/utils.js");

var NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
var DEFAULT_WEARABLE_DATA = {
    joints: {}
};


var MINIMUM_DROP_DISTANCE_FROM_JOINT = 0.8;
var ATTACHED_ENTITY_SEARCH_DISTANCE = 10.0;
var ATTACHED_ENTITIES_SETTINGS_KEY = "ATTACHED_ENTITIES";
var DRESSING_ROOM_DISTANCE = 2.0;
var SHOW_TOOL_BAR = false;

// tool bar

if (SHOW_TOOL_BAR) {
    var BUTTON_SIZE = 64;
    var PADDING = 6;
    Script.include(["libraries/toolBars.js"]);

    var toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL, "highfidelity.attachedEntities.toolbar");
    var lockButton = toolBar.addTool({
        width: BUTTON_SIZE,
        height: BUTTON_SIZE,
        imageURL: Script.resolvePath("assets/images/lock.svg"),
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        visible: true
    }, false);
}


function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({
        x: event.x,
        y: event.y
    });

    if (lockButton === toolBar.clicked(clickedOverlay)) {
        manager.toggleLocked();
    }
}

function scriptEnding() {
    if (SHOW_TOOL_BAR) {
        toolBar.cleanup();
    }
}

if (SHOW_TOOL_BAR) {
    Controller.mousePressEvent.connect(mousePressEvent);
}
Script.scriptEnding.connect(scriptEnding);



// attached entites


function AttachedEntitiesManager() {
    var clothingLocked = false;

    this.subscribeToMessages = function() {
        Messages.subscribe('Hifi-Object-Manipulation');
        Messages.messageReceived.connect(this.handleWearableMessages);
    }

    this.handleWearableMessages = function(channel, message, sender) {
        if (channel !== 'Hifi-Object-Manipulation') {
            return;
        }

        var parsedMessage = null;

        try {
            parsedMessage = JSON.parse(message);
        } catch (e) {
            print('error parsing wearable message');
            return;
        }

        if (parsedMessage.action === 'update' ||
            parsedMessage.action === 'loaded') {
            // ignore
        } else if (parsedMessage.action === 'release') {
            manager.handleEntityRelease(parsedMessage.grabbedEntity, parsedMessage.joint)
                // manager.saveAttachedEntities();
        } else if (parsedMessage.action === 'equip') {
            // manager.saveAttachedEntities();
        } else {
            print('attachedEntitiesManager -- unknown actions: ' + parsedMessage.action);
        }
    }

    this.handleEntityRelease = function(grabbedEntity, releasedFromJoint) {
        // if this is still equipped, just rewrite the position information.
        var grabData = getEntityCustomData('grabKey', grabbedEntity, {});

        var allowedJoints = getEntityCustomData('wearable', grabbedEntity, DEFAULT_WEARABLE_DATA).joints;

        var props = Entities.getEntityProperties(grabbedEntity, ["position", "parentID", "parentJointIndex"]);
        if (props.parentID === NULL_UUID || props.parentID === MyAvatar.sessionUUID) {
            var bestJointName = "";
            var bestJointIndex = -1;
            var bestJointDistance = 0;
            var bestJointOffset = null;
            for (var jointName in allowedJoints) {
                if ((releasedFromJoint == "LeftHand" || releasedFromJoint == "RightHand") &&
                    (jointName == "LeftHand" || jointName == "RightHand")) {
                    // don't auto-attach to a hand if a hand just dropped something
                    continue;
                }
                var jointIndex = MyAvatar.getJointIndex(jointName);
                if (jointIndex >= 0) {
                    var jointPosition = MyAvatar.getJointPosition(jointIndex);
                    var distanceFromJoint = Vec3.distance(jointPosition, props.position);
                    if (distanceFromJoint <= MINIMUM_DROP_DISTANCE_FROM_JOINT) {
                        if (bestJointIndex == -1 || distanceFromJoint < bestJointDistance) {
                            bestJointName = jointName;
                            bestJointIndex = jointIndex;
                            bestJointDistance = distanceFromJoint;
                            bestJointOffset = allowedJoints[jointName];
                        }
                    }
                }
            }

            if (bestJointIndex != -1) {
                var wearProps = Entities.getEntityProperties(grabbedEntity);
                wearProps.parentID = MyAvatar.sessionUUID;
                wearProps.parentJointIndex = bestJointIndex;
                delete wearProps.localPosition;
                delete wearProps.localRotation;
                var updatePresets = false;
                if (bestJointOffset && bestJointOffset.constructor === Array) {
                    if (!clothingLocked || bestJointOffset.length < 2) {
                        // we're unlocked or this thing didn't have a preset position, so update it
                        updatePresets = true;
                    } else {
                        // don't snap the entity to the preferred position if unlocked
                        wearProps.localPosition = bestJointOffset[0];
                        wearProps.localRotation = bestJointOffset[1];
                    }
                }

                Entities.deleteEntity(grabbedEntity);
                //the true boolean here after add entity adds it as an 'avatar entity', which can travel with you from server to server.

                var newEntity = Entities.addEntity(wearProps, true);

                if (updatePresets) {
                    this.updateRelativeOffsets(newEntity);
                }
            } else if (props.parentID != NULL_UUID) {
                // drop the entity and set it to have no parent (not on the avatar), unless it's being equipped in a hand.
                if (props.parentID === MyAvatar.sessionUUID &&
                    (props.parentJointIndex == MyAvatar.getJointIndex("RightHand") ||
                        props.parentJointIndex == MyAvatar.getJointIndex("LeftHand"))) {
                    // this is equipped on a hand -- don't clear the parent.
                } else {
                    var wearProps = Entities.getEntityProperties(grabbedEntity);
                    wearProps.parentID = NULL_UUID;
                    wearProps.parentJointIndex = -1;
                    delete wearProps.id;
                    delete wearProps.created;
                    delete wearProps.age;
                    delete wearProps.ageAsText;
                    delete wearProps.naturalDimensions;
                    delete wearProps.naturalPosition;
                    delete wearProps.actionData;
                    delete wearProps.sittingPoints;
                    delete wearProps.boundingBox;
                    delete wearProps.clientOnly;
                    delete wearProps.owningAvatarID;
                    delete wearProps.localPosition;
                    delete wearProps.localRotation;
                    Entities.deleteEntity(grabbedEntity);
                    Entities.addEntity(wearProps);
                }
            }
        }
    }

    this.updateRelativeOffsets = function(entityID) {
        // save the preferred (current) relative position and rotation into the user-data of the entity
        var props = Entities.getEntityProperties(entityID);
        if (props.parentID == MyAvatar.sessionUUID) {
            grabData = getEntityCustomData('grabKey', entityID, {});
            var wearableData = getEntityCustomData('wearable', entityID, DEFAULT_WEARABLE_DATA);
            var currentJointName = MyAvatar.getJointNames()[props.parentJointIndex];
            wearableData.joints[currentJointName] = [props.localPosition, props.localRotation];
            setEntityCustomData('wearable', entityID, wearableData);
            return true;
        }
        return false;
    }

    // this.saveAttachedEntities = function() {
    //     print("--- saving attached entities ---");
    //     saveData = [];
    //     var nearbyEntities = Entities.findEntities(MyAvatar.position, ATTACHED_ENTITY_SEARCH_DISTANCE);
    //     for (i = 0; i < nearbyEntities.length; i++) {
    //         var entityID = nearbyEntities[i];
    //         if (this.updateRelativeOffsets(entityID)) {
    //             var props = Entities.getEntityProperties(entityID); // refresh, because updateRelativeOffsets changed them
    //             this.scrubProperties(props);
    //             saveData.push(props);
    //         }
    //     }
    //     Settings.setValue(ATTACHED_ENTITIES_SETTINGS_KEY, JSON.stringify(saveData));
    // }

    // this.scrubProperties = function(props) {
    //     var toScrub = ["queryAACube", "position", "rotation",
    //                    "created", "ageAsText", "naturalDimensions",
    //                    "naturalPosition", "velocity", "acceleration",
    //                    "angularVelocity", "boundingBox"];
    //     toScrub.forEach(function(propertyName) {
    //         delete props[propertyName];
    //     });
    //     // if the userData has a grabKey, clear old state
    //     if ("userData" in props) {
    //         try {
    //             parsedUserData = JSON.parse(props.userData);
    //             if ("grabKey" in parsedUserData) {
    //                 parsedUserData.grabKey.refCount = 0;
    //                 delete parsedUserData.grabKey["avatarId"];
    //                 props["userData"] = JSON.stringify(parsedUserData);
    //             }
    //         } catch (e) {
    //         }
    //     }
    // }

    // this.loadAttachedEntities = function(grabbedEntity) {
    //     print("--- loading attached entities ---");
    //     jsonAttachmentData = Settings.getValue(ATTACHED_ENTITIES_SETTINGS_KEY);
    //     var loadData = [];
    //     try {
    //         loadData = JSON.parse(jsonAttachmentData);
    //     } catch (e) {
    //         print('error parsing saved attachment data');
    //         return;
    //     }

    //     for (i = 0; i < loadData.length; i ++) {
    //         var savedProps = loadData[ i ];
    //         var currentProps = Entities.getEntityProperties(savedProps.id);
    //         if (currentProps.id == savedProps.id &&
    //             // TODO -- also check that parentJointIndex matches?
    //             currentProps.parentID == MyAvatar.sessionUUID) {
    //             // entity is already in-world.  TODO -- patch it up?
    //             continue;
    //         }
    //         this.scrubProperties(savedProps);
    //         delete savedProps["id"];
    //         savedProps.parentID = MyAvatar.sessionUUID; // this will change between sessions
    //         var loadedEntityID = Entities.addEntity(savedProps, true);

    //         Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
    //             action: 'loaded',
    //             grabbedEntity: loadedEntityID
    //         }));
    //     }
    // }
}

var manager = new AttachedEntitiesManager();
manager.subscribeToMessages();