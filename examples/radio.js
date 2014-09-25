//
//  Radio.js
//  examples
//
//  Created by Clément Brisset on 8/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var modelURL = "https://s3-us-west-1.amazonaws.com/highfidelity-public/models/entities/radio/Speakers2Finished.fbx";
var soundURL = "https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/FamilyStereo.raw";

var AudioRotationOffset = Quat.fromPitchYawRollDegrees(0, -90, 0);
var audioOptions = new AudioInjectionOptions();
audioOptions.volume = 0.5;
audioOptions.loop = true;
audioOptions.isStereo = true;
var injector = null;

var sound = new Sound(soundURL);

var entity = null;
var properties = null;

function update() {
    if (entity === null) {
        if (sound.downloaded) {
            print("Sound file downloaded");
            var position = Vec3.sum(MyAvatar.position,
                                    Vec3.multiplyQbyV(MyAvatar.orientation,
                                                      { x: 0, y: 0.3, z: -1 }));
            var rotation = Quat.multiply(MyAvatar.orientation,
                                         Quat.fromPitchYawRollDegrees(0, -90, 0));
            entity = Entities.addEntity({
                                        type: "Model",
                                        position: position,
                                        rotation: rotation,
                                        dimensions: { x: 0.5, y: 0.5, z: 0.5 },
                                        modelURL: modelURL
                                      });
            properties = Entities.getEntityProperties(entity);
            
            audioOptions.position = position;
            audioOptions.orientation = rotation;
            injector = Audio.playSound(sound, audioOptions);
        }
    } else {
        var newProperties = Entities.getEntityProperties(entity);
        if (newProperties.type === "Model") {
            if (newProperties.position != properties.position) {
                audioOptions.position = newProperties.position;
            }
            if (newProperties.orientation != properties.orientation) {
                audioOptions.orientation = newProperties.orientation;
            }
            
            properties = newProperties;
        } else {
            entity = null;
            Script.update.disconnect(update);
            Script.scriptEnding.connect(scriptEnding);
            scriptEnding();
            Script.stop();
        }
    }
}

function scriptEnding() {
    if (entity != null) {
        Entities.deleteEntity(entity);
    }
    if (injector != null) {
        injector.stop();
    }
}

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

