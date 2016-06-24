//
//  createBatButton.js
//  examples/baseball/moreBatsButton.js
//
//  Created by Stephen Birarda on 10/28/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){
    this.clickReleaseOnEntity = function(entityID, mouseEvent) {
        if (!mouseEvent.isLeftButton) {
            return;
        }
        this.dropBats();
    };
    this.startNearTrigger = function() {
        this.dropBats();
    };
    this.startFarTrigger = function() {
        this.dropBats();
    };
    this.dropBats = function() {
        // if the bat box is near us, grab it's position
        var nearby = Entities.findEntities(this.position, 20);

        nearby.forEach(function(id) {
            var properties = Entities.getEntityProperties(id, ["name", "position"]);
            if (properties.name && properties.name == "Bat Box") {
                boxPosition = properties.position;
            }
        });

        var BAT_DROP_HEIGHT = 2.0;

        var dropPosition;

        if (!boxPosition) {
            // we got no bat box position, drop in front of the avatar instead
        } else {
            // drop the bat above the bat box
            dropPosition = Vec3.sum(boxPosition, { x: 0.0, y: BAT_DROP_HEIGHT, z: 0.0});
        }

        var BAT_MODEL = "atp:c47deaae09cca927f6bc9cca0e8bbe77fc618f8c3f2b49899406a63a59f885cb.fbx";
        var BAT_COLLISION_HULL = "atp:9eafceb7510c41d50661130090de7e0632aa4da236ebda84a0059a4be2130e0c.obj";
        var SCRIPT_RELATIVE_PATH = "bat.js"

        var batUserData = {
            grabbableKey: {
                spatialKey: {
                    leftRelativePosition: { x: 0.9, y: 0.05, z: -0.05 },
                    rightRelativePosition: { x: 0.9, y: 0.05, z: 0.05 },
                    relativeRotation: Quat.fromPitchYawRollDegrees(0, 0, 45)
                }
            }
        }

        // add the fresh bat at the drop position
        var bat = Entities.addEntity({
            name: 'Bat',
            type: "Model",
            modelURL: BAT_MODEL,
            position: dropPosition,
            compoundShapeURL: BAT_COLLISION_HULL,
            dynamic: true,
            velocity: { x: 0, y: 0.05, z: 0}, // workaround for gravity not taking effect on add
            gravity: { x: 0, y: -9.81, z: 0},
            rotation: Quat.fromPitchYawRollDegrees(0.0, 0.0, -90.0),
            script: Script.resolvePath(SCRIPT_RELATIVE_PATH),
            userData: JSON.stringify(batUserData)
        });
    };
});
