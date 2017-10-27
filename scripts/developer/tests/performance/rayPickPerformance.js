//
//  rayPickingPerformance.js
//  examples
//
//  Created by Brad Hefta-Gaub on 5/13/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



var MIN_RANGE = -3;
var MAX_RANGE = 3;
var RANGE_DELTA = 0.5;
var OUTER_LOOPS = 10;

// NOTE: These expected results depend completely on the model, and the range settings above
var EXPECTED_TESTS = 1385 * OUTER_LOOPS;
var EXPECTED_INTERSECTIONS = 1286 * OUTER_LOOPS;


var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var model_url = "http://hifi-content.s3.amazonaws.com/caitlyn/production/Scansite/buddhaReduced.fbx";

var rayPickOverlays = Array();

var modelEntity = Entities.addEntity({
    type: "Model",
    modelURL: model_url,
    dimensions: {
        x: 0.671,
        y: 1.21,
        z: 0.938
    },
    position: center
});

function rayCastTest() {
    var tests = 0;
    var intersections = 0;

    var testStart = Date.now();
    for (var t = 0; t < OUTER_LOOPS; t++) {
        print("beginning loop:" + t);
        for (var x = MIN_RANGE; x < MAX_RANGE; x += RANGE_DELTA) {
            for (var y = MIN_RANGE; y < MAX_RANGE; y += RANGE_DELTA) {
                for (var z = MIN_RANGE; z < MAX_RANGE; z += RANGE_DELTA) {
                    if ((x <= -2 || x >= 2) || 
                        (y <= -2 || y >= 2) || 
                        (z <= -2 || z >= 2)) {

                        tests++;

                        var origin = { x: center.x + x,
                                       y: center.y + y,
                                       z: center.z + z };
                        var direction = Vec3.subtract(center, origin);

                        var pickRay = { 
                            origin: origin,
                            direction: direction
                        };

                        var pickResults = Entities.findRayIntersection(pickRay, true);

                        var color;
                        var visible;

                        if (pickResults.intersects && pickResults.entityID == modelEntity) {
                            intersections++;
                            color = {
                                red: 0,
                                green: 255,
                                blue: 0
                            };
                            visible = false;

                        } else {
                            /*
                            print("NO INTERSECTION?");
                            Vec3.print("origin:", origin);
                            Vec3.print("direction:", direction);
                            */

                            color = {
                                red: 255,
                                green: 0,
                                blue: 0
                            };
                            visible = true;
                        }

                        var overlayID = Overlays.addOverlay("line3d", {
                            color: color,
                            alpha: 1,
                            visible: visible,
                            lineWidth: 2,
                            start: origin,
                            end: Vec3.sum(origin,Vec3.multiply(5,direction))
                        });

                        rayPickOverlays.push(overlayID);

                    }
                }
            }
        }
        print("ending loop:" + t);
    }
    var testEnd = Date.now();
    var testElapsed = testEnd - testStart;


    print("EXPECTED tests:" + EXPECTED_TESTS + " intersections:" + EXPECTED_INTERSECTIONS);
    print("ACTUAL   tests:" + tests + " intersections:" + intersections);
    print("ELAPSED TIME:" + testElapsed + " ms");

}

function cleanup() {
    Entities.deleteEntity(modelEntity);
    rayPickOverlays.forEach(function(item){
        Overlays.deleteOverlay(item);
    });
}

Script.scriptEnding.connect(cleanup);

rayCastTest(); // run ray cast test immediately