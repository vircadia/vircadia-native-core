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
    var VERSION = "0.0.1";
    //  This sample clip and range will be used if you don't add userData to the entity (see above)
    var DEFAULT_RANGE = 100;
    var DEFAULT_URL = "http://hifi-content.s3.amazonaws.com/ken/samples/forest_ambiX.wav";
    var DEFAULT_VOLUME = 1.0;

    var soundURL = "";
    var soundName = "";
    var startTime;
    var soundOptions = {
        loop: true,
        localOnly: true,
        //ignorePenumbra: true,
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

    var WANT_DEBUG_OVERLAY = false;
    var LINEHEIGHT = 0.1;
    // Optionally enable debug overlays using a Settings value
    WANT_DEBUG_OVERLAY = WANT_DEBUG_OVERLAY || /ambientSound/.test(Settings.getValue("WANT_DEBUG_OVERLAY"));
    var WANT_DEBUG_BROADCASTS = WANT_DEBUG_OVERLAY && "ambientSound.js";

    this.updateSettings = function() {
        //  Check user data on the entity for any changes
        var oldSoundURL = soundURL;
        var props = Entities.getEntityProperties(entity, [ "userData" ]);
        if (props.userData) {
            var data = JSON.parse(props.userData);
            if (data.soundURL && !(soundURL === data.soundURL)) {
                soundURL = data.soundURL;
                soundName = (soundURL||"").split("/").pop(); // just filename part
                debugPrint("Read ambient sound URL: " + soundURL);
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
                if (disabled) {
                    this.cleanup();
                    debugState("disabled");
                    return;
                }
            }
        }
        if (!(soundURL === oldSoundURL) || (soundURL === "")) {
            if (soundURL) {
                debugState("downloading", "Loading ambient sound into cache");
                // Use prefetch to detect URL loading errors
                var resource = SoundCache.prefetch(soundURL);
                function onStateChanged() {
                    if (resource.state === Resource.State.FINISHED) {
                        resource.stateChanged.disconnect(onStateChanged);
                        ambientSound = SoundCache.getSound(soundURL);
                        debugState("idle");
                    } else if (resource.state === Resource.State.FAILED) {
                        resource.stateChanged.disconnect(onStateChanged);
                        debugPrint("Failed to download ambient sound: " + soundURL);
                        debugState("error");
                    }
                    debugPrint("onStateChanged: " + JSON.stringify({
                        sound: soundName,
                        state: resource.state,
                        stateName: Object.keys(Resource.State).filter(function(key) {
                            return Resource.State[key] === resource.state;
                        })
                    }));
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
        if (Settings.getValue("io.highfidelity.isEditting")) {
            return;
        }
        var props = Entities.getEntityProperties(entity, [ "userData", "age", "scriptTimestamp" ]);
        var data = JSON.parse(props.userData);
        data.disabled = !data.disabled;

        debugPrint(hint + " -- triggering ambient sound " + (data.disabled ? "OFF" : "ON") + " (" + soundName + ")");
        var oldState = _debugState;

        if (WANT_DEBUG_BROADCASTS) {
            Messages.sendMessage(WANT_DEBUG_BROADCASTS, JSON.stringify({ palName: MyAvatar.sessionDisplayName, soundName: soundName, hint: hint, scriptTimestamp: props.scriptTimestamp, oldState: oldState, newState: _debugState, age: props.age  }));
        }

        this.cleanup();

        // Save the userData and bump scriptTimestamp, which causes all nearby listeners to apply the state change
        Entities.editEntity(entity, {
            userData: JSON.stringify(data),
            scriptTimestamp: Math.round(props.age * 1000)
        });
        //this._updateColor(data.disabled);
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
        debugPrint("Ambient sound preload " + VERSION);
        entity = entityID;
        _this = this;

        if (WANT_DEBUG_OVERLAY) {
            _createDebugOverlays();
        }

        var props = Entities.getEntityProperties(entity, [ "userData" ]);
        if (props.userData) {
            var data = JSON.parse(props.userData);
            this._updateColor(data.disabled);
            if (data.disabled) {
                _this.maybeUpdate();
                return;
            }
        }

        checkTimer = Script.setInterval(_this.maybeUpdate, UPDATE_INTERVAL_MSECS);
    };

    this.maybeUpdate = function() {
        // Every UPDATE_INTERVAL_MSECS, update the volume of the ambient sound based on distance from my avatar
        _this.updateSettings();
        var HYSTERESIS_FRACTION = 0.1;
        var props = Entities.getEntityProperties(entity, [ "position", "rotation" ]);
        if (!props.position) {
            // FIXME: this mysterious state has been happening while testing
            //  and might indicate a bug where an Entity can become unreachable without `unload` having been called..
            print("FIXME: ambientSound.js -- expected Entity unavailable!")
            if (WANT_DEBUG_BROADCASTS) {
                Messages.sendMessage(WANT_DEBUG_BROADCASTS, JSON.stringify({ palName: MyAvatar.sessionDisplayName, soundName: soundName, hint: "FIXME: maybeUpdate", oldState: _debugState  }));
            }
            return _this.cleanup();
        }
        center = props.position;
        rotation = props.rotation;
        var distance = Vec3.length(Vec3.subtract(MyAvatar.position, center));
        if (distance <= range) {
            var volume = (1.0 - distance / range) * maxVolume;
            soundOptions.orientation = rotation;
            soundOptions.volume = volume;
            if (!soundPlaying && ambientSound && ambientSound.downloaded) {
                debugState("playing", "Starting ambient sound: " + soundName + " (duration: " + ambientSound.duration + ")");
                soundPlaying = Audio.playSound(ambientSound, soundOptions);
            } else if (soundPlaying && soundPlaying.playing) {
                soundPlaying.setOptions(soundOptions);
            }
        } else if (soundPlaying && soundPlaying.playing && (distance > range * HYSTERESIS_FRACTION)) {
            soundPlaying.stop();
            soundPlaying = false;
            debugState("idle", "Out of range, stopping ambient sound: " + soundName);
        }
        if (WANT_DEBUG_OVERLAY) {
            updateDebugOverlay(distance);
        }
    }

    this.unload = function(entityID) {
        debugPrint("Ambient sound unload ");
        if (WANT_DEBUG_BROADCASTS) {
            var offset = ambientSound && (new Date - startTime)/1000 % ambientSound.duration;
            Messages.sendMessage(WANT_DEBUG_BROADCASTS, JSON.stringify({ palName: MyAvatar.sessionDisplayName, soundName: soundName, hint: "unload", oldState: _debugState, offset: offset  }));
        }
        if (WANT_DEBUG_OVERLAY) {
            _removeDebugOverlays();
        }
        this.cleanup();
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

    // Visual debugging overlay (to see set WANT_DEBUG_OVERLAY = true)

    var DEBUG_COLORS = {
        //preload:     { red: 0,   green: 80,  blue: 80 },
        disabled:    { red: 0,   green: 0,   blue: 0, alpha: 0.0 },
        downloading: { red: 255, green: 255, blue: 0  },
        error:       { red: 255, green: 0,   blue: 0  },
        playing:     { red: 0,   green: 200, blue: 0  },
        idle:        { red: 0,   green: 100, blue: 0  }
    };
    var _debugOverlay;
    var _debugState = "";
    function debugState(state, message) {
        if (state === "playing") {
            startTime = new Date;
        }
        _debugState = state;
        if (message) {
            debugPrint(message);
        }
        updateDebugOverlay();
        if (WANT_DEBUG_BROADCASTS) {
            // Broadcast state changes to make multi-user scenarios easier to verify from a single console
            Messages.sendMessage(WANT_DEBUG_BROADCASTS, JSON.stringify({ palName: MyAvatar.sessionDisplayName, soundName: soundName, state: state }));
        }
    }

    function updateDebugOverlay(distance) {
        var props = Entities.getEntityProperties(entity, [ "name", "dimensions" ]);
        if (!props.dimensions) {
            return print("ambientSound.js: updateDebugOverlay -- entity no longer available " + entity);
        }
        var options = soundPlaying && soundPlaying.options;
        if (options) {
            var offset = soundPlaying.playing && ambientSound && (new Date - startTime)/1000 % ambientSound.duration;
            var deg = Quat.safeEulerAngles(options.orientation);
            var orientation = [ deg.x, deg.y, deg.z].map(Math.round).join(", ");
            var volume = options.volume;
        }
        var info = {
            //loudness: soundPlaying.loudness && soundPlaying.loudness.toFixed(4) || undefined,
            offset: offset && ("00"+offset.toFixed(1)).substr(-4)+"s" || undefined,
            orientation: orientation,
            injector: soundPlaying && soundPlaying.playing && "playing",
            resource: ambientSound && ambientSound.downloaded && "ready (" + ambientSound.duration.toFixed(1) + "s)",
            name: props.name || undefined,
            uuid: entity.split(/\W/)[1], // extracts just the first part of the UUID
            sound: soundName,
            volume: Math.max(0,volume||0).toFixed(2) + " / " + maxVolume.toFixed(2),
            distance: (distance||0).toFixed(1) + "m / " + range.toFixed(1) + "m",
            state: _debugState.toUpperCase(),
        };

        // Pretty print key/value pairs, excluding any undefined values
        var outputText = Object.keys(info).filter(function(key) {
            return info[key] !== undefined;
        }).map(function(key) {
            return key + ": " + info[key];
        }).join("\n");

        // Calculate a local position for displaying info just above the Entity
        var textSize = Overlays.textSize(_debugOverlay, outputText);
        var size = {
            x: textSize.width + LINEHEIGHT,
            y: textSize.height + LINEHEIGHT
        };
        var pos = { x: 0, y: props.dimensions.y + size.y/2, z: 0 };

        var backgroundColor = DEBUG_COLORS[_debugState];
        var backgroundAlpha = backgroundColor ? backgroundColor.alpha : 0.6;
        Overlays.editOverlay(_debugOverlay, {
            visible: true,
            backgroundColor: backgroundColor,
            backgroundAlpha: backgroundAlpha,
            text: outputText,
            localPosition: pos,
            size: size,
        });
    }

    function _removeDebugOverlays() {
        if (_debugOverlay) {
            Overlays.deleteOverlay(_debugOverlay);
            _debugOverlay = 0;
        }
    }

    function _createDebugOverlays() {
        _debugOverlay = Overlays.addOverlay("text3d", {
            visible: true,
            lineHeight: LINEHEIGHT,
            leftMargin: LINEHEIGHT/2,
            topMargin: LINEHEIGHT/2,
            localPosition: Vec3.ZERO,
            parentID: entity,
            ignoreRayIntersection: true,
            isFacingAvatar: true,
            textAlpha: 0.6,
            //drawInFront: true,
        });
    }
})
