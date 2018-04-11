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

/* global Script, print, Entities, Picks, HMD */


(function() {
    var dispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");

    function MouseHighlightEntities() {
        this.highlightedEntity = null;

        this.parameters = dispatcherUtils.makeDispatcherModuleParameters(
            5,
            ["mouse"],
            [],
            100);

        this.isReady = function(controllerData) {
            if (HMD.active) {
                if (this.highlightedEntity) {
                    dispatcherUtils.unhighlightTargetEntity(this.highlightedEntity);
                    this.highlightedEntity = null;
                }
            } else {
                var pickResult = controllerData.mouseRayPick;
                if (pickResult.type === Picks.INTERSECTED_ENTITY) {
                    var targetEntityID = pickResult.objectID;

                    if (this.highlightedEntity !== targetEntityID) {
                        var targetProps = Entities.getEntityProperties(targetEntityID, [
                            "dynamic", "shapeType", "position",
                            "rotation", "dimensions", "density",
                            "userData", "locked", "type", "href"
                        ]);

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

    function cleanup() {
        dispatcherUtils.disableDispatcherModule("MouseHighlightEntities");
    }
    Script.scriptEnding.connect(cleanup);
})();
