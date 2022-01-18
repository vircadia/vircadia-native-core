//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var baseURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/CellScience/";
    var self = this;
    this.buttonImageURL = baseURL + "GUI/play_audio.svg?2";

    this.preload = function(entityId) {
        this.entityId = entityId;
        this.initialize(entityId)
        this.initTimeout = null;
    }

    this.initialize = function(entityId) {
        //print(' should initialize' + entityId)
        var properties = Entities.getEntityProperties(entityId);
        if (properties.userData.length === 0 || properties.hasOwnProperty('userData') === false) {
            self.initTimeout = Script.setTimeout(function() {
                // print(' no user data yet, try again in one second')
                self.initialize(entityId);
            }, 1000)

        } else {
            //print(' userdata before parse attempt' + properties.userData)
            self.userData = null;
            try {
                self.userData = JSON.parse(properties.userData);
            } catch (err) {
                // print(' error parsing json');
                // print(' properties are:' + properties.userData);
                return;
            }

            self.addButton();
            self.buttonShowing = false;
            self.showDistance = self.userData.showDistance;
            self.soundURL = baseURL + "Audio/" + self.userData.soundName + ".wav";
            // print("distance = " + self.userData.showDistance + ", sound = " + self.soundURL);
            self.soundOptions = {
                stereo: true,
                loop: false,
                localOnly: true,
                volume: 1
            };
            self.sound = SoundCache.getSound(this.soundURL);

        }
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
            //print("button was clicked");
            if (self.sound.downloaded) {
                //print("play sound");
                Audio.playSound(self.sound, self.soundOptions);
            } else {
                //print("not downloaded");
            }
        }
    }

    this.unload = function() {
        Overlays.deleteOverlay(self.button);
        Controller.mousePressEvent.disconnect(this.onClick);
        Script.update.disconnect(this.update);
        if (this.initTimeout !== null) {
            Script.clearTimeout(this.initTimeout);
        }
    }

    Controller.mousePressEvent.connect(this.onClick);
    Script.update.connect(this.update);

});
