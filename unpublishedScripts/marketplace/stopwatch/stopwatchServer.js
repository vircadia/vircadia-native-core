//
//  stopwatchServer.js
//
//  Created by Ryan Huffman on 1/20/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var self = this;

    self.equipped = false;
    self.isActive = false;

    self.secondHandID = null;
    self.minuteHandID = null;

    self.tickSound = SoundCache.getSound(Script.resolvePath("sounds/tick.wav"));
    self.tickInjector = null;
    self.tickIntervalID = null;

    self.chimeSound = SoundCache.getSound(Script.resolvePath("sounds/chime.wav"));

    self.preload = function(entityID) {
        print("Preloading stopwatch: ", entityID);
        self.entityID = entityID;
        self.messageChannel = "STOPWATCH-" + entityID;

        var userData = Entities.getEntityProperties(self.entityID, 'userData').userData;
        var data = JSON.parse(userData);
        self.secondHandID = data.secondHandID;
        self.minuteHandID = data.minuteHandID;

        self.resetTimer();

        Messages.subscribe(self.messageChannel);
        Messages.messageReceived.connect(this, self.messageReceived);
    };
    self.unload = function() {
        print("Unloading stopwatch:", self.entityID);
        self.resetTimer();
        Messages.unsubscribe(self.messageChannel);
        Messages.messageReceived.disconnect(this, self.messageReceived);
    };
    self.messageReceived = function(channel, message, sender) {
        print("Message received", channel, sender, message); 
        if (channel === self.messageChannel && message === 'click') {
            if (self.isActive) {
                self.resetTimer();
            } else {
                self.startTimer();
            }
        }
    };
    self.getStopwatchPosition = function() {
        return Entities.getEntityProperties(self.entityID, "position").position;
    };
    self.resetTimer = function() {
        print("Stopping stopwatch");
        if (self.tickInjector) {
            self.tickInjector.stop();
        }
        if (self.tickIntervalID !== null) {
            Script.clearInterval(self.tickIntervalID);
            self.tickIntervalID = null;
        }
        Entities.editEntity(self.secondHandID, {
            rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            angularVelocity: { x: 0, y: 0, z: 0 },
        });
        Entities.editEntity(self.minuteHandID, {
            rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            angularVelocity: { x: 0, y: 0, z: 0 },
        });
        self.isActive = false;
    };
    self.startTimer = function() {
        print("Starting stopwatch");
        if (!self.tickInjector) {
            self.tickInjector = Audio.playSound(self.tickSound, {
                position: self.getStopwatchPosition(),
                volume: 0.7,
                loop: true
            });
        } else {
            self.tickInjector.restart();
        }

        var seconds = 0;
        self.tickIntervalID = Script.setInterval(function() {
            if (self.tickInjector) {
                self.tickInjector.setOptions({
                    position: self.getStopwatchPosition(),
                    volume: 0.7,
                    loop: true
                });
            }
            seconds++;
            const degreesPerTick = -360 / 60;
            Entities.editEntity(self.secondHandID, {
                rotation: Quat.fromPitchYawRollDegrees(0, seconds * degreesPerTick, 0),
            });
            if (seconds % 60 == 0) {
                Entities.editEntity(self.minuteHandID, {
                    rotation: Quat.fromPitchYawRollDegrees(0, (seconds / 60) * degreesPerTick, 0),
                });
                Audio.playSound(self.chimeSound, {
                    position: self.getStopwatchPosition(),
                    volume: 1.0,
                    loop: false
                });
            }
        }, 1000);

        self.isActive = true;
    };
});
