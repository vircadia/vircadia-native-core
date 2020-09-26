//  ambientSound.js
//
//  This entity script will allow you to create an ambient sound that loops when a person is within a given
//  range of this entity.  Great way to add one or more ambisonic soundfields to your environment.
//
//  In the userData section for the entity, add/edit three values:
//      userData.soundURL should be a string giving the URL to the sound file.  Defaults to 100 meters if not set.
//      userData.range should be an integer for the max distance away from the entity where the sound will be audible.
//      userData.maxVolume is the max volume at which the clip should play.  Defaults to 1.0 full volume.
//      userData.disabled is an optionanl boolean flag which can be used to disable the ambient sound. Defaults to false.
//
//  The rotation of the entity is copied to the ambisonic field, so by rotating the entity you will rotate the
//  direction in-which a certain sound comes from.
//
//  Remember that the entity has to be visible to the user for the sound to play at all, so make sure the entity is
//  large enough to be loaded at the range you set, particularly for large ranges.
//
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){
    //  This sample clip and range will be used if you don't add userData to the entity (see above)
    var DEFAULT_RANGE = 100;
    var DEFAULT_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/ken/samples/forest_ambiX.wav");
    var DEFAULT_VOLUME = 1.0;

    var DEFAULT_USERDATA = {
        soundURL: DEFAULT_URL,
        range: DEFAULT_RANGE,
        maxVolume: DEFAULT_VOLUME,
        disabled: true,
        grab: { triggerable: true }
    };

    var soundURL = "";
    var soundOptions = {
        loop: true,
        localOnly: true,
    };
    var range = DEFAULT_RANGE;
    var maxVolume = DEFAULT_VOLUME;
    var disabled = false;
    var UPDATE_INTERVAL_MSECS = 100;
    var rotation;

    var entity;
    var ambientSound;
    var center;
    var soundPlaying = false;
    var checkTimer = false;
    var _this;

    var COLOR_OFF = { red: 128, green: 128, blue: 128 };
    var COLOR_ON = { red: 255, green: 0, blue: 0 };

    var WANT_DEBUG = false;
    function debugPrint(string) {
        if (WANT_DEBUG) {
            print("ambientSound | " + string);
        }
    }

    this.updateSettings = function() {
        //  Check user data on the entity for any changes
        var oldSoundURL = soundURL;
        var props = Entities.getEntityProperties(entity, [ "userData" ]);
        if (props.userData) {
            try {
                var data = JSON.parse(props.userData);
            } catch(e) {
                debugPrint("unable to parse userData JSON string");
                this.cleanup();
                return;
            }
            if (data.soundURL && !(soundURL === data.soundURL)) {
                soundURL = data.soundURL;
                debugPrint("Read ambient sound URL");
            }
            if (data.range && !(range === data.range)) {
                range = data.range;
                debugPrint("Read ambient sound range: " + range);
            }
            // Check known aliases for the "volume" setting (which allows for inplace upgrade of existing marketplace entities)
            data.maxVolume = data.maxVolume || data.soundVolume || data.volume;
            if (data.maxVolume && !(maxVolume === data.maxVolume)) {
                maxVolume = data.maxVolume;
                debugPrint("Read ambient sound volume: " + maxVolume);
            }
            if ("disabled" in data && !(disabled === data.disabled)) {
                disabled = data.disabled;
                debugPrint("Read ambient disabled state: " + disabled);
                this._updateColor(disabled);
            }
        }
        if (disabled) {
            this.cleanup();
            soundURL = "";
            return;
        } else if (!checkTimer) {
            checkTimer = Script.setInterval(_this.maybeUpdate, UPDATE_INTERVAL_MSECS);
        }
        if (!(soundURL === oldSoundURL) || (soundURL === "")) {
            if (soundURL) {
                debugPrint("Loading ambient sound into cache");
                // Use prefetch to detect URL loading errors
                var resource = SoundCache.prefetch(soundURL);
                function onStateChanged() {
                    if (resource.state === Resource.State.FINISHED) {
                        resource.stateChanged.disconnect(onStateChanged);
                        ambientSound = SoundCache.getSound(soundURL);
                    } else if (resource.state === Resource.State.FAILED) {
                        resource.stateChanged.disconnect(onStateChanged);
                        debugPrint("Failed to download ambient sound");
                    }
                }
                resource.stateChanged.connect(onStateChanged);
                onStateChanged(resource.state);
            }
            if (soundPlaying && soundPlaying.playing) {
                debugPrint("URL changed, stopping current ambient sound");
                soundPlaying.stop();
                soundPlaying = false;
            }
        }
    }

    this.clickDownOnEntity = function(entityID, mouseEvent) {
        if (mouseEvent.isPrimaryButton) {
            this._toggle("primary click");
        }
    };

    this.startFarTrigger = function() {
        this._toggle("far click");
    };

    this._toggle = function(hint) {
        // Toggle between ON/OFF state, but only if not in edit mode
        if (Settings.getValue("io.highfidelity.isEditing")) {
            return;
        }
        var props = Entities.getEntityProperties(entity, [ "userData" ]);
        if (!props.userData) {
            debugPrint("userData is empty; ignoring " + hint);
            this.cleanup();
            return;
        }
        var data = JSON.parse(props.userData);
        data.disabled = !data.disabled;

        debugPrint(hint + " -- triggering ambient sound " + (data.disabled ? "OFF" : "ON"));

        this.cleanup();

        // Save the userData and notify nearby listeners of the change
        Entities.editEntity(entity, {
            userData: JSON.stringify(data)
        });
        Messages.sendMessage(entity, "toggled");
    };

    this._updateColor = function(disabled) {
        // Update Shape or Text Entity color based on ON/OFF status
        var props = Entities.getEntityProperties(entity, [ "color", "textColor" ]);
        var targetColor = disabled ? COLOR_OFF : COLOR_ON;
        var currentColor = props.textColor || props.color;
        var newProps = props.textColor ? { textColor: targetColor } : { color: targetColor };

        if (currentColor.red !== targetColor.red ||
            currentColor.green !== targetColor.green ||
            currentColor.blue !== targetColor.blue) {
            Entities.editEntity(entity, newProps);
        }
    };

    this.preload = function(entityID) {
        //  Load the sound and range from the entity userData fields, and note the position of the entity.
        debugPrint("Ambient sound preload " + entityID);
        entity = entityID;
        _this = this;

        var props = Entities.getEntityProperties(entity, [ "userData", "grab.triggerable" ]);
        var data = {};
        if (props.userData) {
            data = JSON.parse(props.userData);
        }
        var changed = false;
        for(var p in DEFAULT_USERDATA) {
            if (!(p in data)) {
                data[p] = DEFAULT_USERDATA[p];
                changed = true;
            }
        }
        if (changed) {
            debugPrint("applying default values to userData");
            Entities.editEntity(entity, { userData: JSON.stringify(data) });
        }

        if (!props.grab.triggerable) {
            Entities.editEntity(entity, { grab: { triggerable: true } });
        }

        this._updateColor(data.disabled);
        this.updateSettings();

        // Subscribe to toggle notifications using entity ID as a channel name
        Messages.subscribe(entity);
        Messages.messageReceived.connect(this, "_onMessageReceived");
    };

    this._onMessageReceived = function(channel, message, sender, local) {
        // Handle incoming toggle notifications
        if (channel === entity && message === "toggled") {
            debugPrint("received " + message + " from " + sender);
            this.updateSettings();
        }
    };

    this.maybeUpdate = function() {
        // Every UPDATE_INTERVAL_MSECS, update the volume of the ambient sound based on distance from my avatar
        _this.updateSettings();
        var HYSTERESIS_FRACTION = 0.1;
        var props = Entities.getEntityProperties(entity, [ "position", "rotation" ]);
        if (disabled || !props.position) {
            _this.cleanup();
            return;
        }
        center = props.position;
        rotation = props.rotation;
        var distance = Vec3.length(Vec3.subtract(MyAvatar.position, center));
        if (distance <= range) {
            var volume = (1.0 - distance / range) * maxVolume;
            soundOptions.orientation = rotation;
            soundOptions.volume = volume;
            if (!soundPlaying && ambientSound && ambientSound.downloaded) {
                debugPrint("Starting ambient sound: (duration: " + ambientSound.duration + ")");
                soundPlaying = Audio.playSound(ambientSound, soundOptions);
            } else if (soundPlaying && soundPlaying.playing) {
                soundPlaying.setOptions(soundOptions);
            }
        } else if (soundPlaying && soundPlaying.playing && (distance > range * HYSTERESIS_FRACTION)) {
            soundPlaying.stop();
            soundPlaying = false;
            debugPrint("Out of range, stopping ambient sound");
        }
    };

    this.unload = function(entityID) {
        debugPrint("Ambient sound unload");
        this.cleanup();
        Messages.unsubscribe(entity);
        Messages.messageReceived.disconnect(this, "_onMessageReceived");
    };

    this.cleanup = function() {
        if (checkTimer) {
            Script.clearInterval(checkTimer);
            checkTimer = false;
        }
        if (soundPlaying && soundPlaying.playing) {
            soundPlaying.stop();
            soundPlaying = false;
        }
    };

})
