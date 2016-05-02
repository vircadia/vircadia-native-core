//
//  musicPlayer.js
//
//  Created by Brad Hefta-Gaub on 3/3/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

(function() {

    var imageShader = Script.resolvePath("./imageShader.fs");
    var defaultImage = Script.resolvePath("./defaultImage.jpg");

    var MAPPING_NAME = "com.highfidelity.musicPlayerEntity";

    var PLAYLIST_URL = "https://spreadsheets.google.com/feeds/cells/1x-ceGPGHldkHadARABFWfujLPTOWzXJPhrf2bTwg2cQ/od6/public/basic?alt=json";
    var SONG_VOLUME = 0.1;
    var HEADPHONES_ATTACHMENT = {
        modelURL: "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/musicPlayer/headphones2-v2.fbx",
        jointName: "Head",
        translation: {"x": 0, "y": 0.19, "z": 0.06},
        rotation: {"x":0,"y":0.7071067690849304,"z":0.7071067690849304,"w":0},
        scale: 0.435,
        isSoft: false
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Various helper functions...
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Helper function for returning either a value, or the default value if the value is undefined. This is
    // is handing in parsing JSON where you don't know if the values have been set or not.
    function valueOrDefault(value, defaultValue) {
        if (value !== undefined) {
            return value;
        }
        return defaultValue;
    }

    // return a random float between high and low
    function randFloat(low, high) {
        return low + Math.random() * (high - low);
    }

    // wears an attachment on MyAvatar
    function wearAttachment(attachment) {
        MyAvatar.attach(attachment.modelURL,
                        attachment.jointName,
                        attachment.translation,
                        attachment.rotation,
                        attachment.scale,
                        attachment.isSoft);
    }

    // un-wears an attachment from MyAvatar
    function removeAttachment(attachment) {
        var attachments = MyAvatar.attachmentData;
        var i, l = attachments.length;
        for (i = 0; i < l; i++) {
            if (attachments[i].modelURL === attachment.modelURL) {
                attachments.splice(i, 1);
                MyAvatar.attachmentData = attachments;
                break;
            }
        }
    }

    var _this;
    MusicPlayer = function() {
        _this = this;
        this.equipped = false;
    };

    MusicPlayer.prototype = {
        preload: function(entityID) {
            print("preload");
            //print("rotation:" + JSON.stringify(Quat.fromPitchYawRollDegrees(-90,180,0)));
            this.entityID = entityID;

            // Get the entities userData property, to see if someone has overridden any of our default settings
            var userDataText = Entities.getEntityProperties(entityID, ["userData"]).userData;
            //print(userDataText);
            var userData = {};
            if (userDataText !== "") {
                //print("calling JSON.parse");
                userData = JSON.parse(userDataText);
                //print("userData:" + JSON.stringify(userData));
            }
            var musicPlayerUserData = valueOrDefault(userData.musicPlayer, {});
            this.headphonesAttachment = valueOrDefault(musicPlayerUserData.headphonesAttachment, HEADPHONES_ATTACHMENT);

            this.track = 0; // start at the first track
            this.playlistURL = valueOrDefault(musicPlayerUserData.playlistURL, PLAYLIST_URL);
            this.songVolume = valueOrDefault(musicPlayerUserData.songVolume, SONG_VOLUME);
            this.songPlaying = false;

            this.loadPlayList();

            // Find my screen and any controlls
            var children = Entities.getChildrenIDsOfJoint(entityID, 65535);
            for (var child in children) {
                var childID = children[child];
                var childProperties = Entities.getEntityProperties(childID,["type", "name"]);
                if (childProperties.type == "Text" && childProperties.name == "now playing") {
                    this.nowPlayingID = childID;
                }
                if (childProperties.type == "Box" && childProperties.name == "album art") {
                    this.albumArt = childID;
                }
            }
        },

        unload: function() {
            print("unload");
            if (_this.songInjector !== undefined) {
                _this.songInjector.stop();
            }
        },

        loadPlayList: function() {
            print("loadPlayList");
            var req = new XMLHttpRequest();
            req.open("GET", _this.playlistURL, false);
            req.send();

            var entries = JSON.parse(req.responseText).feed.entry;

            for (entry in entries) {
                var cellDetails = entries[entry];
                var cellName = cellDetails.title.$t;
                var column = Number(cellName.charCodeAt(0)) - Number("A".charCodeAt(0));
                var row = Number(cellName.slice(1)) - 1;
                var cellContent = cellDetails.content.$t;
                //print(JSON.stringify(cellDetails));
                //print("["+column +"/"+ row +":"+cellContent+"]");
                if (_this.playList === undefined) {
                    _this.playList = new Array();
                }
                if (_this.playList[row] === undefined) {
                    _this.playList[row] = { };
                }
                switch (column) {
                    case 0:
                        _this.playList[row].title = cellContent;
                        break;
                    case 1:
                        _this.playList[row].artist = cellContent;
                        break;
                    case 2:
                        _this.playList[row].album = cellContent;
                        break;
                    case 3:
                        _this.playList[row].url = cellContent;
                        _this.playList[row].sound = SoundCache.getSound(cellContent);
                        break;
                    case 4:
                        _this.playList[row].albumArtURL = cellContent;
                        break;
                }
            }
            //print(req.responseText);
            print(JSON.stringify(_this.playList));
        },

        startEquip: function(id, params) {
            var whichHand = params[0]; // "left" or "right"
            print("I am equipped in the " + whichHand + " hand....");
            this.equipped = true;
            this.hand = whichHand;

            this.loadPlayList(); // reload the playlist in case...

            this.mapHandButtons(whichHand);
            wearAttachment(HEADPHONES_ATTACHMENT);
        },

        continueEquip: function(id, params) {
            if (!this.equipped) {
                return;
            }
        },
        releaseEquip: function(id, params) {
            print("I am NO LONGER equipped....");
            this.hand = null;
            this.equipped = false;
            Controller.disableMapping(MAPPING_NAME);
            removeAttachment(HEADPHONES_ATTACHMENT);
            this.pause();
        },

        mapHandButtons: function(hand) {
            var mapping = Controller.newMapping(MAPPING_NAME);
            if (hand === "left") {
                mapping.from(Controller.Standard.LS).peek().to(this.playOrPause);
                mapping.from(Controller.Standard.LX).peek().to(this.seek);
                mapping.from(Controller.Standard.LY).peek().to(this.volume);
            } else {
                mapping.from(Controller.Standard.RS).peek().to(this.playOrPause);
                mapping.from(Controller.Standard.RX).peek().to(this.seek);
                mapping.from(Controller.Standard.RY).peek().to(this.volume);
            }
            Controller.enableMapping(MAPPING_NAME);
        },

        playOrPause: function(value) {
            print("[playOrPause: "+value+"]");
            if (value === 1) {
                if (!_this.songPlaying) {
                    _this.play();
                } else {
                    _this.pause();
                }
            }
        },

        play: function() {
            print("play current track:" + _this.track);
            if (!_this.playList[_this.track].sound.downloaded) {
                print("still waiting on track to download....");
                return; // not yet ready
            }

            var statusText = "Song:" + _this.playList[_this.track].title + "\n" +
                             "Artist:" + _this.playList[_this.track].artist + "\n" +
                             "Album:" + _this.playList[_this.track].album;

            Entities.editEntity(_this.nowPlayingID, { text: statusText });

            var newAlbumArt = JSON.stringify(
                    {
                        "ProceduralEntity": {
                            "version":2,
                            "shaderUrl":imageShader,
                            "uniforms":{"iSpeed":0,"iShell":1},
                            "channels":[_this.playList[_this.track].albumArtURL]
                        }
                    });

            Entities.editEntity(_this.albumArt, { userData: newAlbumArt });
             

            _this.songInjector = Audio.playSound(_this.playList[_this.track].sound, {
                position: MyAvatar.position,
                volume: _this.songVolume,
                loop: false,
                localOnly: true
            });
            _this.songPlaying = true;
        },

        pause: function() {
            print("pause");
            Entities.editEntity(_this.nowPlayingID, { text: "Paused" });
            if (_this.songInjector !== undefined) {
                _this.songInjector.stop();
            }
            var newAlbumArt = JSON.stringify(
                    {
                        "ProceduralEntity": {
                            "version":2,
                            "shaderUrl":imageShader,
                            "uniforms":{"iSpeed":0,"iShell":1},
                            "channels":[defaultImage]
                        }
                    });

            Entities.editEntity(_this.albumArt, { userData: newAlbumArt });


            _this.songPlaying = false;
        },

        seek: function(value) {
            print("[seek: " + value + "]");
            if (value > 0.9) {
                _this.next();
            } else if (value < -0.9) {
                _this.previous();
            }
        },

        volume: function(value) {
            print("adjusting volume disabled because of a bug in audio injectors....");
            /*
            var scaledValue = value / 20; 
            _this.songVolume += scaledValue;
            print("[volume: " + value + "] new volume:" + _this.songVolume);
            if (_this.songInjector !== undefined) {
                print("[volume: attempting to set options....");
                var newOptions = {
                    position: MyAvatar.position,
                    volume: _this.songVolume,
                    loop: false,
                    localOnly: true
                };
                
                _this.songInjector.options = newOptions;
                print("[volume: attempting to set options.... RESULT:" + JSON.stringify(_this.songInjector.options));
            }
            */
        },

        previous: function() {
            print("[previous]");
            _this.pause();
            _this.track--;
            if (_this.track < 0) {
                _this.track = (_this.playList.length - 1);
            }
            _this.play();
        },

        next: function() {
            print("[next]");
            _this.pause();
            _this.track++;
            if (_this.track >= _this.playList.length) {
                _this.track = 0;
            }
            _this.play();
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new MusicPlayer();
});
