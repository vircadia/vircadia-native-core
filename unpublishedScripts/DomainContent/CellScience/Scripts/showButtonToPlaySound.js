//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var baseURL = "https://hifi-content.s3.amazonaws.com/hifi-content/DomainContent/CellScience/";
    var self = this;
    this.buttonImageURL = baseURL + "GUI/play_audio.svg?2";



    this.preload = function(entityId) {
        this.entityId = entityId;
        self.addButton();
        this.buttonShowing = false;
        self.getUserData();
        this.showDistance = self.userData.showDistance;
        this.soundURL = baseURL + "Audio/" + self.userData.soundName + ".wav";
        print("distance = " + self.userData.showDistance + ", sound = " + this.soundURL);
        this.soundOptions = {
            stereo: true,
            loop: false,
            localOnly: true,
            volume: 0.035
        };
        this.sound = SoundCache.getSound(this.soundURL);
    }

    this.addButton = function() {
        this.windowDimensions = Controller.getViewportDimensions();
        this.buttonWidth = 100;
        this.buttonHeight = 100;
        this.buttonPadding = 0;

        this.buttonPositionX = (self.windowDimensions.x - self.buttonPadding) / 2 - self.buttonWidth;
        this.buttonPositionY = (self.windowDimensions.y - self.buttonHeight) - (self.buttonHeight + self.buttonPadding);
        this.button = Overlays.addOverlay("image", {
            x: self.buttonPositionX,
            y: self.buttonPositionY,
            width: self.buttonWidth,
            height: self.buttonHeight,
            imageURL: self.buttonImageURL,
            visible: false,
            alpha: 1.0
        });
    }

    this.getUserData = function() {
        this.properties = Entities.getEntityProperties(this.entityId);
        if (self.properties.userData) {
            this.userData = JSON.parse(this.properties.userData);
        } else {
            this.userData = {};
        }
    }

    this.update = function(deltaTime) {

        self.distance = Vec3.distance(MyAvatar.position, Entities.getEntityProperties(self.entityId).position);
        //print(self.distance);
        if (!self.buttonShowing && self.distance < self.userData.showDistance) {
            self.buttonShowing = true;
            Overlays.editOverlay(self.button, {
                visible: true
            });
        } else if (self.buttonShowing && self.distance > self.userData.showDistance) {
            self.buttonShowing = false;
            Overlays.editOverlay(self.button, {
                visible: false
            });
        }
    }

    this.onClick = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });
        if (clickedOverlay === self.button) {
            print("button was clicked");
            if (self.sound.downloaded) {
                print("play sound");
                Audio.playSound(self.sound, self.soundOptions);
            } else {
                print("not downloaded");
            }
        }
    }

    this.unload = function() {
        Overlays.deleteOverlay(self.button);
        Controller.mousePressEvent.disconnect(this.onClick);
        Script.update.disconnect(this.update);
    }

    Controller.mousePressEvent.connect(this.onClick);
    Script.update.connect(this.update);

});