//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var version = 12;
    var added = false;
    var self = this;
    var baseURL = "https://hifi-production.s3.amazonaws.com/DomainContent/CellScience/";

    this.preload = function(entityId) {
        this.entityId = entityId;
        self.buttonImageURL = baseURL + "GUI/GUI_Ribosome.png?" + version;
        // print(' BUTTON IMAGE URL:' + self.buttonImageURL)
        if (self.button === undefined) {
            //    print(' NO BUTTON ADDING ONE!!')
            self.button = true;
            self.addButton();

        } else {
            //print(' SELF ALREADY HAS A BUTTON!!')
        }
    }

    this.addButton = function() {

        this.windowDimensions = Controller.getViewportDimensions();
        this.buttonWidth = 150;
        this.buttonHeight = 50;
        this.buttonPadding = 10;
        this.offset = 3;
        this.buttonPositionX = (this.offset + 1) * (this.buttonWidth + this.buttonPadding) + (self.windowDimensions.x / 2) - (this.buttonWidth * 3 + this.buttonPadding * 2.5);


        this.buttonPositionY = (self.windowDimensions.y - self.buttonHeight) - 50;
        this.button = Overlays.addOverlay("image", {
            x: self.buttonPositionX,
            y: self.buttonPositionY,
            width: self.buttonWidth,
            height: self.buttonHeight,
            imageURL: self.buttonImageURL,
            visible: true,
            alpha: 1.0
        });

    }


    this.onClick = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({
            x: event.x,
            y: event.y
        });


        if (clickedOverlay == self.button) {
            self.lookAtTarget();
        }

    }

    this.lookAtTarget = function() {

        var entryPoint = {
            x: 13500,
            y: 3000,
            z: 3000
        };

        var target = {
            x: 2755,
            y: 3121,
            z: 13501
        };

        var direction = Vec3.normalize(Vec3.subtract(entryPoint, target));
        var pitch = Quat.angleAxis(Math.asin(-direction.y) * 180.0 / Math.PI, {
            x: 1,
            y: 0,
            z: 0
        });
        var yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * 180.0 / Math.PI, {
            x: 0,
            y: 1,
            z: 0
        });

        MyAvatar.goToLocation(entryPoint, true, yaw);

        MyAvatar.headYaw = 0;

    }

    this.unload = function() {
        Overlays.deleteOverlay(self.button);
        Controller.mousePressEvent.disconnect(this.onClick);

    }

    Controller.mousePressEvent.connect(this.onClick);

});