//
//  mouseHighlightEntities.js
//
//  scripts/system/controllers/controllerModules/
//
//  Created by Dante Ruiz 2018-4-11
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* jslint bitwise: true */

/* global Script, print, Entities, Messages, Picks, HMD, MyAvatar, isInEditMode, DISPATCHER_PROPERTIES */


(function() {
    Script.include("/~/system/libraries/utils.js");
    var dispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");

    function MouseHighlightEntities() {
        this.highlightedEntity = null;
        this.grabbedEntity = null;

        this.parameters = dispatcherUtils.makeDispatcherModuleParameters(
            5,
            ["mouse"],
            [],
            100);

        this.setGrabbedEntity = function(entity) {
            this.grabbedEntity = entity;
            this.highlightedEntity = null;
        };

        this.isReady = function(controllerData) {
            if (HMD.active) {
                if (this.highlightedEntity) {
                    dispatcherUtils.unhighlightTargetEntity(this.highlightedEntity);
                    this.highlightedEntity = null;
                }
            } else if (!this.grabbedEntity && !isInEditMode()) {
                var pickResult = controllerData.mouseRayPick;
                if (pickResult.type === Picks.INTERSECTED_ENTITY) {
                    var targetEntityID = pickResult.objectID;

                    if (this.highlightedEntity !== targetEntityID) {
                        var targetProps = Entities.getEntityProperties(targetEntityID, DISPATCHER_PROPERTIES);

                        if (this.highlightedEntity) {
                            dispatcherUtils.unhighlightTargetEntity(this.highlightedEntity);
                            this.highlightedEntity = null;
                        }

                        if (dispatcherUtils.entityIsGrabbable(targetProps)) {
                            // highlight entity
                            dispatcherUtils.highlightTargetEntity(targetEntityID);
                            this.highlightedEntity = targetEntityID;
                        }
                    }
                } else if (this.highlightedEntity) {
                    dispatcherUtils.unhighlightTargetEntity(this.highlightedEntity);
                    this.highlightedEntity = null;
                }
            }

            return dispatcherUtils.makeRunningValues(false, [], []);
        };

        this.run = function(controllerData) {
            return this.isReady(controllerData);
        };
    }

    var mouseHighlightEntities = new MouseHighlightEntities();
    dispatcherUtils.enableDispatcherModule("MouseHighlightEntities", mouseHighlightEntities);

    var handleMessage = function(channel, message, sender) {
        var data;
        if (sender === MyAvatar.sessionUUID) {
            if (channel === 'Hifi-Object-Manipulation') {
                try {
                    data = JSON.parse(message);
                    if (data.action === 'grab') {
                        var grabbedEntity = data.grabbedEntity;
                        mouseHighlightEntities.setGrabbedEntity(grabbedEntity);
                    } else if (data.action === 'release') {
                        mouseHighlightEntities.setGrabbedEntity(null);
                    }
                } catch (e) {
                    print("Warning: mouseHighlightEntities -- error parsing Hifi-Object-Manipulation: " + message);
                }
            }
        }
    };

    function cleanup() {
        dispatcherUtils.disableDispatcherModule("MouseHighlightEntities");
    }
    Messages.subscribe('Hifi-Object-Manipulation');
    Messages.messageReceived.connect(handleMessage);
    Script.scriptEnding.connect(cleanup);
})();
