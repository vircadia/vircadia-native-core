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
    self.seconds = 0;

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
        if (channel === self.messageChannel) {
            switch (message) {
                case "startStop":
                    if (self.isActive) {
                        self.stopTimer();
                    } else {
                        self.startTimer();
                    }
                    break;
                case "reset":
                    self.stopTimer();
                    self.resetTimer();
                    break;
            }
        }
    };
    self.getStopwatchPosition = function() {
        return Entities.getEntityProperties(self.entityID, "position").position;
    };
    self.resetTimer = function() {
        print("Resetting stopwatch");
        Entities.editEntity(self.secondHandID, {
            localRotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            angularVelocity: { x: 0, y: 0, z: 0 },
        });
        Entities.editEntity(self.minuteHandID, {
            localRotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            angularVelocity: { x: 0, y: 0, z: 0 },
        });
        self.seconds = 0;
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

        self.tickIntervalID = Script.setInterval(function() {
            if (self.tickInjector) {
                self.tickInjector.setOptions({
                    position: self.getStopwatchPosition(),
                    volume: 0.7,
                    loop: true
                });
            }
            self.seconds++;
            const degreesPerTick = -360 / 60;
            Entities.editEntity(self.secondHandID, {
                localRotation: Quat.fromPitchYawRollDegrees(0, self.seconds * degreesPerTick, 0),
            });

            if (self.seconds % 60 == 0) {
                Entities.editEntity(self.minuteHandID, {
                    localRotation: Quat.fromPitchYawRollDegrees(0, (self.seconds / 60) * degreesPerTick, 0),
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
    self.stopTimer = function () {
        print("Stopping stopwatch");
        if (self.tickInjector) {
            self.tickInjector.stop();
        }
        if (self.tickIntervalID !== null) {
            Script.clearInterval(self.tickIntervalID);
            self.tickIntervalID = null;
        }
        self.isActive = false;
    };
});
