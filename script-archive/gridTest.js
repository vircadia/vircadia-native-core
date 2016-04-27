//
//  Created by Philip Rosedale on July 28, 2015
//  Copyright 2015 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Creates a rectangular grid of objects, starting at the origin and proceeding along the X/Z plane.
//  Useful for testing the rendering, LOD, and octree storage aspects of the system.  
//
//  Note that when creating things quickly, the entity server will ignore data if we send updates too quickly.
//  like Internet MTU, these rates are set by th domain operator, so in this script there is a RATE_PER_SECOND 
//  variable letting you set this speed.  If entities are missing from the grid after a relog, this number 
//  being too high may be the reason. 

var SIZE = 10.0; 
var SEPARATION = 20.0;
var ROWS_X = 30; 
var ROWS_Z = 30;
var TYPE = "Model";            //   Right now this can be "Box" or "Model" or "Sphere"
var MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/CellScience/Instances/vesicle.fbx";
var MODEL_DIMENSION = { x: 33, y: 16, z: 49 };
var RATE_PER_SECOND = 1000;    //    The entity server will drop data if we create things too fast.
var SCRIPT_INTERVAL = 100;
var LIFETIME = 600;            //     By default, these entities will live in the server for 10 minutes 

var addRandom = false; 

var x = 0;
var z = 0;
var totalCreated = 0;

Script.setInterval(function () {
    if (!Entities.serversExist() || !Entities.canRez()) {
        return;
    } 
    
    var numToCreate = RATE_PER_SECOND * (SCRIPT_INTERVAL / 1000.0);
    for (var i = 0; i < numToCreate; i++) {
        var position = { x: SIZE + (x * SEPARATION), y: SIZE, z: SIZE + (z * SEPARATION) };
        print('position:'+JSON.stringify(position))
        if (TYPE == "Model") {
            Entities.addEntity({ 
            type: TYPE,
            name: "gridTest",
            modelURL: MODEL_URL,
            position: position,  
            dimensions: MODEL_DIMENSION,        
            ignoreCollisions: true,
            dynamic: false, 
            lifetime: LIFETIME
            });
        } else {
            Entities.addEntity({ 
            type: TYPE,
            name: "gridTest",
            position: position,
            dimensions: { x: SIZE, y: SIZE, z: SIZE },       
            color: { red: x / ROWS_X * 255, green: 50, blue: z / ROWS_Z * 255 },
            ignoreCollisions: true,
            dynamic: false,
            lifetime: LIFETIME
            });
        }

        totalCreated++;

        x++;
        if (x == ROWS_X) {
            x = 0;
            z++;
            print("Created: " + totalCreated);
        }
        if (z == ROWS_Z) {
            Script.stop();
        }
    } 
}, SCRIPT_INTERVAL);

