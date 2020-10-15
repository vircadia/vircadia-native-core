//
//  createHoop.js
//
//  Created by James B. Pollack on 9/29/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This is a script that creates a persistent basketball hoop with a working collision hull.  Feel free to move it.
//


var hoopURL ="https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball_hoop.fbx";
var hoopCollisionHullURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball_hoop_collision_hull.obj";

var hoopStartPosition =
    Vec3.sum(MyAvatar.position,
        Vec3.multiplyQbyV(MyAvatar.orientation, {
            x: 0,
            y: 0.0,
            z: -2
        }));

var hoop = Entities.addEntity({
    type: "Model",
    modelURL: hoopURL,
    position: hoopStartPosition,
    shapeType: 'compound',
    gravity: {
        x: 0,
        y: -9.8,
        z: 0
    },
    dimensions: {
        x: 1.89,
        y: 3.99,
        z: 3.79
    },
    userData: JSON.stringify({
        grabbableKey: {
            grabbable: false
        }
    }),
    compoundShapeURL: hoopCollisionHullURL
});