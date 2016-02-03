//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var self = this;
    var baseURL = "https://hifi-content.s3.amazonaws.com/DomainContent/CellScience/";
    var version = 9;
    this.preload = function(entityId) {
        self.soundPlaying = false;
        self.entityId = entityId;
        self.getUserData();
        self.soundURL = baseURL + "Audio/" + self.userData.name + ".wav?" + version;
        print("Script.clearTimeout creating WAV name location is " + baseURL + "Audio/" + self.userData.name + ".wav");

        self.soundOptions = {
            stereo: true,
            loop: true,
            localOnly: true,
            volume: 0.035
        };

        this.sound = SoundCache.getSound(self.soundURL);

    }

    this.getUserData = function() {
        self.properties = Entities.getEntityProperties(self.entityId);
        if (self.properties.userData) {
            self.userData = JSON.parse(this.properties.userData);
        } else {
            self.userData = {};
        }
    }

    this.enterEntity = function(entityID) {
        print("entering audio zone");
        if (self.sound.downloaded) {
            print("playing background audio named " + self.userData.name + "which has been downloaded");
            this.soundPlaying = Audio.playSound(self.sound, self.soundOptions);

        } else {
            print("sound is not downloaded");
        }
    }


    this.leaveEntity = function(entityID) {
        print("leaving audio area " + self.userData.name);
        if (self.soundPlaying !== false) {
            print("not null");
            print("Stopped sound " + self.userData.name);
            self.soundPlaying.stop();
        } else {
            print("Sound not playing");
        }
    }



});