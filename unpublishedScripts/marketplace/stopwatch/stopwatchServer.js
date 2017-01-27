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
    this.equipped = false;
    this.isActive = false;

    this.secondHandID = null;
    this.minuteHandID = null;

    var tickSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/huffman/tick.wav");
    var tickInjector = null;
    var tickIntervalID = null;

    var chimeSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/huffman/chime.wav");

    this.preload = function(entityID) {
        print("Preloading stopwatch: ", entityID);
        this.entityID = entityID;
        this.messageChannel = "STOPWATCH-" + entityID;

        var userData = Entities.getEntityProperties(this.entityID, 'userData').userData;
        var data = JSON.parse(userData);
        this.secondHandID = data.secondHandID;
        this.minuteHandID = data.minuteHandID;

        this.resetTimer();

        Messages.subscribe(this.messageChannel);
        Messages.messageReceived.connect(this, this.messageReceived);
    };
    this.unload = function() {
        print("Unloading stopwatch:", this.entityID);
        this.resetTimer();
        Messages.unsubscribe(this.messageChannel);
        Messages.messageReceived.disconnect(this, this.messageReceived);
    };
    this.messageReceived = function(channel, message, sender) {
        print("Message received", channel, sender, message); 
        if (channel === this.messageChannel && message === 'click') {
            if (this.isActive) {
                this.resetTimer();
            } else {
                this.startTimer();
            }
        }
    };
    this.getStopwatchPosition = function() {
        return Entities.getEntityProperties(this.entityID, "position").position;
    };
    this.resetTimer = function() {
        print("Stopping stopwatch");
        if (tickInjector) {
            tickInjector.stop();
        }
        if (tickIntervalID !== null) {
            Script.clearInterval(tickIntervalID);
            tickIntervalID = null;
        }
        Entities.editEntity(this.secondHandID, {
            rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            angularVelocity: { x: 0, y: 0, z: 0 },
        });
        Entities.editEntity(this.minuteHandID, {
            rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            angularVelocity: { x: 0, y: 0, z: 0 },
        });
        this.isActive = false;
    };
    this.startTimer = function() {
        print("Starting stopwatch");
        if (!tickInjector) {
            tickInjector = Audio.playSound(tickSound, {
                position: this.getStopwatchPosition(),
                volume: 0.7,
                loop: true
            });
        } else {
            tickInjector.restart();
        }

        var self = this;
        var seconds = 0;
        tickIntervalID = Script.setInterval(function() {
            if (tickInjector) {
                tickInjector.setOptions({
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
                Audio.playSound(chimeSound, {
                    position: self.getStopwatchPosition(),
                    volume: 1.0,
                    loop: false
                });
            }
        }, 1000);

        this.isActive = true;
    };
});
