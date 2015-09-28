//
//  rayPickExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Camera class
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function mouseMoveEvent(event) {
    print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
    var pickRay = Camera.computePickRay(event.x, event.y);
    print("called Camera.computePickRay()");
    print("computePickRay origin=" + pickRay.origin.x + ", " + pickRay.origin.y + ", " + pickRay.origin.z);
    print("computePickRay direction=" + pickRay.direction.x + ", " + pickRay.direction.y + ", " + pickRay.direction.z);
    var pickRay = Camera.computePickRay(event.x, event.y);
    intersection = Entities.findRayIntersection(pickRay, true); // to get precise picking
    if (!intersection.accurate) {
        print(">>> NOTE: intersection not accurate. will try calling Entities.findRayIntersectionBlocking()");
        intersection = Entities.findRayIntersectionBlocking(pickRay, true); // to get precise picking
        print(">>> AFTER BLOCKING CALL intersection.accurate=" + intersection.accurate);
    }
    
    if (intersection.intersects) {
        print("intersection entityID=" + intersection.entityID);
        print("intersection properties.modelURL=" + intersection.properties.modelURL);
        print("intersection face=" + intersection.face);
        print("intersection distance=" + intersection.distance);
        print("intersection intersection.x/y/z=" + intersection.intersection.x + ", "
                    + intersection.intersection.y + ", " + intersection.intersection.z);
        print("intersection surfaceNormal.x/y/z=" + intersection.surfaceNormal.x + ", "
                    + intersection.surfaceNormal.y + ", " + intersection.surfaceNormal.z);
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);

function scriptEnding() {
}
Script.scriptEnding.connect(scriptEnding);

