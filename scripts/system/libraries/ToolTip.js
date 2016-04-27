//
//  ToolTip.js
//  examples/libraries
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


function Tooltip() {
    this.x = 285;
    this.y = 115;
    this.width = 500;
    this.height = 180; // 145;
    this.margin = 5;
    this.decimals = 3;

    this.textOverlay = Overlays.addOverlay("text", {
        x: this.x,
        y: this.y,
        width: this.width,
        height: this.height,
        margin: this.margin,
        text: "",
        color: { red: 128, green: 128, blue: 128 },
        alpha: 0.2,
        backgroundAlpha: 0.2,
        visible: false
    });
    this.show = function (doShow) {
        Overlays.editOverlay(this.textOverlay, { visible: doShow });
    }
    this.updateText = function(properties) {
        var angles = Quat.safeEulerAngles(properties.rotation);
        var text = "Entity Properties:\n"
        text += "type: " + properties.type + "\n"
        text += "X: " + properties.position.x.toFixed(this.decimals) + "\n"
        text += "Y: " + properties.position.y.toFixed(this.decimals) + "\n"
        text += "Z: " + properties.position.z.toFixed(this.decimals) + "\n"
        text += "Pitch: " + angles.x.toFixed(this.decimals) + "\n"
        text += "Yaw:  " + angles.y.toFixed(this.decimals) + "\n"
        text += "Roll:    " + angles.z.toFixed(this.decimals) + "\n"
        text += "Dimensions: " + properties.dimensions.x.toFixed(this.decimals) + ", "
                               + properties.dimensions.y.toFixed(this.decimals) + ", "
                               + properties.dimensions.z.toFixed(this.decimals) + "\n";

        text += "Natural Dimensions: " + properties.naturalDimensions.x.toFixed(this.decimals) + ", "
                                       + properties.naturalDimensions.y.toFixed(this.decimals) + ", "
                                       + properties.naturalDimensions.z.toFixed(this.decimals) + "\n";

        text += "ID: " + properties.id + "\n"
        if (properties.type == "Model") {
            text += "Model URL: " + properties.modelURL + "\n"
            text += "Shape Type: " + properties.shapeType + "\n"
            text += "Compound Shape URL: " + properties.compoundShapeURL + "\n"
            text += "Animation URL: " + properties.animationURL + "\n"
            text += "Animation is playing: " + properties.animationIsPlaying + "\n"
            if (properties.sittingPoints && properties.sittingPoints.length > 0) {
                text += properties.sittingPoints.length + " Sitting points: "
                for (var i = 0; i < properties.sittingPoints.length; ++i) {
                    text += properties.sittingPoints[i].name + " "
                }
            } else {
                text += "No sitting points" + "\n"
            }
        }
        if (properties.lifetime > -1) {
            text += "Lifetime: " + properties.lifetime + "\n"
        }
        text += "Age: " + properties.ageAsText + "\n"
        text += "Density: " + properties.density + "\n"
        text += "Script: " + properties.script + "\n"


        Overlays.editOverlay(this.textOverlay, { text: text });
    }

    this.cleanup = function () {
        Overlays.deleteOverlay(this.textOverlay);
    }
}

tooltip = new Tooltip();
