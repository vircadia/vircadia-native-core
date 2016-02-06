//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var self = this;
    var baseURL = "https://hifi-content.s3.amazonaws.com/DomainContent/CellScience/";
    var version = 10;
    this.preload = function(entityId) {
        self.soundPlaying = false;
        self.entityId = entityId;
        this.initTimeout = null;
        this.initialize(entityId);
    }

    this.initialize = function(entityID) {
        //print(' should initialize' + entityID)
        var properties = Entities.getEntityProperties(entityID);
        if (properties.userData.length === 0 || properties.hasOwnProperty('userData') === false) {
            self.initTimeout = Script.setTimeout(function() {
                //print(' no user data yet, try again in one second')
                self.initialize(entityID);
            }, 1000)

        } else {
            //print(' userdata before parse attempt' + properties.userData)
            self.userData = null;
            try {
                self.userData = JSON.parse(properties.userData);
            } catch (err) {
                //print(' error parsing json');
                //print(' properties are:' + properties.userData);
                return;
            }


            //print(' USERDATA NAME ' + self.userData.name)
            self.soundURL = baseURL + "Audio/" + self.userData.name + ".wav?" + version;
            //print(" creating WAV name location is " + baseURL + "Audio/" + self.userData.name + ".wav");
            //print(' self soundURL' + self.soundURL)

            self.soundOptions = {
                stereo: true,
                loop: true,
                localOnly: true,
                volume: 0.035
            };

            self.sound = SoundCache.getSound(self.soundURL);
        }
    }

    this.enterEntity = function(entityID) {
        //print("entering audio zone");
        if (self.sound.downloaded) {
            //print("playing background audio named " + self.userData.name + "which has been downloaded");
            this.soundPlaying = Audio.playSound(self.sound, self.soundOptions);

        } else {
            //print("sound is not downloaded");
        }
    }

    this.leaveEntity = function(entityID) {
        //print("leaving audio area " + self.userData.name);
        if (self.soundPlaying !== false) {
            //print("not null");
            //print("Stopped sound " + self.userData.name);
            self.soundPlaying.stop();
        } else {
            //print("Sound not playing");
        }
    }

    this.unload = function() {
        if (this.initTimeout !== null) {
            Script.clearTimeout(this.initTimeout);
        }
    }



});