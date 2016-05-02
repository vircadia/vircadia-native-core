"use strict";
/*jslint nomen: true, plusplus: true, vars: true*/
var Entities, Script, print, Vec3, MyAvatar;
//
//  Created by Howard Stearns
//  Copyright 2016 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Creates a rectangular matrix of objects overhed, and edits them at EDIT_FREQUENCY_TARGET.
//  Reports ms since last edit, ms to edit all the created entities, and average ms to edit one entity,
//  so that you can measure how many edits can be made.
//
var LIFETIME = 15;
var ROWS_X = 10;
var ROWS_Y = 2;
var ROWS_Z = 10;
var SEPARATION = 10.0;
var SIZE = 1.0;
//  Note that when creating things quickly, the entity server will ignore data if we send updates too quickly.
//  like Internet MTU, these rates are set by th domain operator, so in this script there is a RATE_PER_SECOND 
//  variable letting you set this speed.  If entities are missing from the grid after a relog, this number 
//  being too high may be the reason. 
var RATE_PER_SECOND = 600;    //    The entity server will drop data if we create things too fast.
var SCRIPT_INTERVAL = 100;

var x = 0;
var y = 0;
var z = 0;
var o = Vec3.sum(MyAvatar.position, {x: ROWS_X * SEPARATION / -2, y: SEPARATION, z: ROWS_Z * SEPARATION / -2});
var totalCreated = 0;
var startTime = new Date();
var totalToCreate = ROWS_X * ROWS_Y * ROWS_Z;
print("Creating " + totalToCreate + " entities starting at " + startTime);

var ids = [], colors = [], flasher, lastService = Date.now();
function doFlash() {  // One could call this in an interval timer, an update, or even a separate entity script.
    var i, oldColor,  newColor;
    var start = Date.now();
    for (i = 0; i < ids.length; i++) {
        oldColor = colors[i];
        newColor = {red: oldColor.green, green: oldColor.blue, blue: oldColor.red};
        colors[i] = newColor;
        Entities.editEntity(ids[i], {color: newColor});
    }
    var elapsed = Date.now() - start, serviceTime = start - lastService;
    lastService = start;
    print(serviceTime, elapsed, elapsed / totalCreated); // ms since last flash, ms to edit all entities, average ms to edit one entity
}
function stopFlash() {
    Script.clearTimeout(flasher);
    Script.stop();
}
var creator = Script.setInterval(function () {
    if (!Entities.serversExist() || !Entities.canRez()) {
        return;
    }

    var numToCreate = Math.min(RATE_PER_SECOND * (SCRIPT_INTERVAL / 1000.0), totalToCreate - totalCreated);
    var i, properties;
    for (i = 0; i < numToCreate; i++) {
        properties = {
            position: { x: o.x + SIZE + (x * SEPARATION), y: o.y + SIZE + (y * SEPARATION), z: o.z + SIZE + (z * SEPARATION) },
            name: "gridTest",
            type: 'Box',
            dimensions: {x: SIZE, y: SIZE, z: SIZE},
            ignoreCollisions: true,
            collisionsWillMove: false,
            lifetime: LIFETIME,
            color: {red: x / ROWS_X * 255, green: y / ROWS_Y * 255, blue: z / ROWS_Z * 255}
        };
        colors.push(properties.color);
        ids.push(Entities.addEntity(properties));
        totalCreated++;

        x++;
        if (x === ROWS_X) {
            x = 0;
            y++;
            if (y === ROWS_Y) {
                y = 0;
                z++;
                print("Created: " + totalCreated);
            }
        }
        if (z === ROWS_Z) {
            print("Total: " + totalCreated + " entities in " + ((new Date() - startTime) / 1000.0) + " seconds.");
            Script.clearTimeout(creator);
            flasher = Script.setInterval(doFlash, 1000 / 60); // I.e., spin as fast as we have time for.
            Script.setTimeout(stopFlash, LIFETIME * 1000);
        }
    }
}, SCRIPT_INTERVAL);
Script.scriptEnding.connect(function () { Script.clearTimeout(flasher); });

