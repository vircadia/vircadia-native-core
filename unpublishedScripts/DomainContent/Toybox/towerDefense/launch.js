//
//  cylinderBlock.js
//
//  Created by David Rowe on 25 Oct 2016.
//  Copyright 2015 High Fidelity, Inc.
//
//  This script displays a progress download indicator when downloads are in progress.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var BLOCK_MODEL_URL = Script.resolvePath("assets/block.fbx");
    var BLOCK_DIMENSIONS = {
        x: 1,
        y: 1,
        z: 1
    };
    var BLOCK_LIFETIME = 120;

    var MUZZLE_SOUND_URL = Script.resolvePath("air_gun_1_converted.wav");
    var MUZZLE_SOUND_VOLUME = 0.5;
    var MUZZLE_SPEED = 6.0;  // m/s
    var MUZZLE_ANGLE = 5.0;  // Degrees off-center to discourage blocks falling back inside cylinder.
    var muzzleSound;

    var cylinderID;

    this.preload = function (entityID) {
        print("launch.js | preload");
        cylinderID = entityID;
        muzzleSound = SoundCache.getSound(MUZZLE_SOUND_URL);
    }

    this.launchBlock = function () {
        print("launch.js | Launching block");
        var cylinder = Entities.getEntityProperties(cylinderID, ["position", "rotation", "dimensions"]);
        var muzzlePosition = Vec3.sum(cylinder.position,
            Vec3.multiplyQbyV(cylinder.rotation, { x: 0.0, y: 0.5 * (cylinder.dimensions.y + BLOCK_DIMENSIONS.y), Z: 0.0 }));
        var muzzleVelocity = Vec3.multiply(MUZZLE_SPEED, Vec3.UNIT_Y);
        muzzleVelocity = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(MUZZLE_ANGLE, 0.0, 0.0), muzzleVelocity);
        muzzleVelocity = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0.0, Math.random() * 360.0, 0.0), muzzleVelocity);
        muzzleVelocity = Vec3.multiplyQbyV(cylinder.rotation, muzzleVelocity);

        Entities.addEntity({
            type: "Model",
            name: "TD.block",
            modelURL: BLOCK_MODEL_URL,
            shapeType: "compound",
            //compoundShapeURL: BLOCK_COMPOUND_SHAPE_URL,
            dimensions: BLOCK_DIMENSIONS,
            dynamic: 1,
            gravity: { x: 0.0, y: -9.8, z: 0.0 },
            collisionsWillMove: 1,
            position: muzzlePosition,
            rotation: Quat.multiply(cylinder.rotation, Quat.fromPitchYawRollDegrees(0.0, Math.random() * 360.0, 0.0)),
            velocity: muzzleVelocity,
            lifetime: BLOCK_LIFETIME,
            script: Script.resolvePath("destructibleEntity.js")
        });

        Audio.playSound(muzzleSound, {
            position: cylinder.muzzlePosition,
            orientation: cylinder.rotation,
            volume: MUZZLE_SOUND_VOLUME
        });
    }

    this.clickDownOnEntity = function () {
        print("launch.js | got click down");
        this.launchBlock();
    }
    
    this.startNearTrigger = function () {
        print("launch.js | got start near trigger");
        this.launchBlock();
    }

    this.startFarTrigger = function () {
        print("launch.js | got start far trigger");
        this.launchBlock();
    }
})
