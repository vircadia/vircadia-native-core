//
//  metavoxels.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.setInterval(function() {
    var spanner;
    if (Math.random() < 0.5) {
        spanner = new Sphere();
    } else {
        spanner = new Cuboid();
        spanner.aspectX = 0.1 + Math.random() * 1.9;
        spanner.aspectY = 0.1 + Math.random() * 1.9;
    }
    spanner.scale = 0.1 + Math.random() * 1.9;
    spanner.rotation = Quat.fromPitchYawRollDegrees(Math.random() * 360.0, Math.random() * 360.0, Math.random() * 360.0);
    spanner.translation = { x: 10.0 + Math.random() * 10.0, y: 10.0 + Math.random() * 10.0, z: 10.0 + Math.random() * 10.0 };
    
    if (Math.random() < 0.5) {
        var material = new MaterialObject();
        if (Math.random() < 0.5) {
            material.diffuse = "http://www.fungibleinsight.com/faces/grass.jpg";
        } else {
            material.diffuse = "http://www.fungibleinsight.com/faces/soil.jpg";
        }
        Metavoxels.setVoxelMaterial(spanner, material);
    
    } else if (Math.random() < 0.5) {
        Metavoxels.setVoxelColor(spanner, { red: Math.random() * 255.0, green: Math.random() * 255.0,
            blue: Math.random() * 255.0 });
    
    } else {
        Metavoxels.setVoxelColor(spanner, { red: 0, green: 0, blue: 0, alpha: 0 });
    }
}, 1000);

