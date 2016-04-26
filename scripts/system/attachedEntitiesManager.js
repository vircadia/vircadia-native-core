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
    var BUTTON_SIZE = 32;
    var PADDING = 3;
    Script.include(["libraries/toolBars.js"]);
    var toolBar = new ToolBar(0, 0, ToolBar.VERTICAL, "highfidelity.attachedEntities.toolbar", function(screenSize) {
        return {
            x: (BUTTON_SIZE + PADDING),
            y: (screenSize.y / 2 - BUTTON_SIZE * 2 + PADDING)
        };
    });
    var saveButton = toolBar.addOverlay("image", {
        width: BUTTON_SIZE,
        height: BUTTON_SIZE,
        imageURL: ".../save.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1
    });
    var loadButton = toolBar.addOverlay("image", {
        width: BUTTON_SIZE,
        height: BUTTON_SIZE,
        imageURL: ".../load.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1
    });
}


function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({
        x: event.x,
        y: event.y
    });

    if (clickedOverlay == saveButton) {
        manager.saveAttachedEntities();
    } else if (clickedOverlay == loadButton) {
        manager.loadAttachedEntities();
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
    this.subscribeToMessages = function() {
        Messages.subscribe('Hifi-Object-Manipulation');
        Messages.messageReceived.connect(this.handleWearableMessages);
    }

    this.handleWearableMessages = function(channel, message, sender) {
        if (channel !== 'Hifi-Object-Manipulation') {
            return;
        }
        // if (sender !== MyAvatar.sessionUUID) {
        //     print('wearablesManager got message from wrong sender');
        //     return;
        // }

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

    this.avatarIsInDressingRoom = function() {
        // return true if MyAvatar is near the dressing room
        var possibleDressingRoom = Entities.findEntities(MyAvatar.position, DRESSING_ROOM_DISTANCE);
        for (i = 0; i < possibleDressingRoom.length; i++) {
            var entityID = possibleDressingRoom[i];
            var props = Entities.getEntityProperties(entityID);
            if (props.name == 'Hifi-Dressing-Room-Base') {
                return true;
            }
        }
        return false;
    }

    this.handleEntityRelease = function(grabbedEntity, releasedFromJoint) {
        // if this is still equipped, just rewrite the position information.
        var grabData = getEntityCustomData('grabKey', grabbedEntity, {});
        if ("refCount" in grabData && grabData.refCount > 0) {
            manager.updateRelativeOffsets(grabbedEntity);
            return;
        }

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
                var wearProps = {
                    parentID: MyAvatar.sessionUUID,
                    parentJointIndex: bestJointIndex
                };

                if (bestJointOffset && bestJointOffset.constructor === Array) {
                    if (this.avatarIsInDressingRoom() || bestJointOffset.length < 2) {
                        this.updateRelativeOffsets(grabbedEntity);
                    } else {
                        // don't snap the entity to the preferred position if the avatar is in the dressing room.
                        wearProps.localPosition = bestJointOffset[0];
                        wearProps.localRotation = bestJointOffset[1];
                    }
                }
                Entities.editEntity(grabbedEntity, wearProps);
            } else if (props.parentID != NULL_UUID) {
                // drop the entity and set it to have no parent (not on the avatar), unless it's being equipped in a hand.
                if (props.parentID === MyAvatar.sessionUUID &&
                    (props.parentJointIndex == MyAvatar.getJointIndex("RightHand") ||
                     props.parentJointIndex == MyAvatar.getJointIndex("LeftHand"))) {
                    // this is equipped on a hand -- don't clear the parent.
                } else {
                    Entities.editEntity(grabbedEntity, { parentID: NULL_UUID });
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

    this.saveAttachedEntities = function() {
        print("--- saving attached entities ---");
        saveData = [];
        var nearbyEntities = Entities.findEntities(MyAvatar.position, ATTACHED_ENTITY_SEARCH_DISTANCE);
        for (i = 0; i < nearbyEntities.length; i++) {
            var entityID = nearbyEntities[i];
            if (this.updateRelativeOffsets(entityID)) {
                var props = Entities.getEntityProperties(entityID); // refresh, because updateRelativeOffsets changed them
                this.scrubProperties(props);
                saveData.push(props);
            }
        }
        Settings.setValue(ATTACHED_ENTITIES_SETTINGS_KEY, JSON.stringify(saveData));
    }

    this.scrubProperties = function(props) {
        var toScrub = ["queryAACube", "position", "rotation",
                       "created", "ageAsText", "naturalDimensions",
                       "naturalPosition", "velocity", "acceleration",
                       "angularVelocity", "boundingBox"];
        toScrub.forEach(function(propertyName) {
            delete props[propertyName];
        });
        // if the userData has a grabKey, clear old state
        if ("userData" in props) {
            try {
                parsedUserData = JSON.parse(props.userData);
                if ("grabKey" in parsedUserData) {
                    parsedUserData.grabKey.refCount = 0;
                    delete parsedUserData.grabKey["avatarId"];
                    props["userData"] = JSON.stringify(parsedUserData);
                }
            } catch (e) {
            }
        }
    }

    this.loadAttachedEntities = function(grabbedEntity) {
        print("--- loading attached entities ---");
        jsonAttachmentData = Settings.getValue(ATTACHED_ENTITIES_SETTINGS_KEY);
        var loadData = [];
        try {
            loadData = JSON.parse(jsonAttachmentData);
        } catch (e) {
            print('error parsing saved attachment data');
            return;
        }

        for (i = 0; i < loadData.length; i ++) {
            var savedProps = loadData[ i ];
            var currentProps = Entities.getEntityProperties(savedProps.id);
            if (currentProps.id == savedProps.id &&
                // TODO -- also check that parentJointIndex matches?
                currentProps.parentID == MyAvatar.sessionUUID) {
                // entity is already in-world.  TODO -- patch it up?
                continue;
            }
            this.scrubProperties(savedProps);
            delete savedProps["id"];
            savedProps.parentID = MyAvatar.sessionUUID; // this will change between sessions
            var loadedEntityID = Entities.addEntity(savedProps);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'loaded',
                grabbedEntity: loadedEntityID
            }));
        }
    }
}

var manager = new AttachedEntitiesManager();
manager.subscribeToMessages();
