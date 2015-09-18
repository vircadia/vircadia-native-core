
Wand = function() {
    // Max updates fps
    this.DEFAULT_TIP_COLORS = [ {
        red: 128,
        green: 128,
        blue: 128,
    }, {
        red: 0,
        green: 0,
        blue: 0,
    }];
    this.POINTER_ROTATION = Quat.fromPitchYawRollDegrees(45, 0, 45);

    
    this.pointers = [
        Overlays.addOverlay("cube", {
            position: ZERO_VECTOR,
            color: this.DEFAULT_TIP_COLORS[0],
            alpha: 1.0,
            solid: true,
            visible: false,
        }),
        Overlays.addOverlay("cube", {
            position: ZERO_VECTOR,
            color: this.DEFAULT_TIP_COLORS[1],
            alpha: 1.0,
            solid: true,
            visible: false,
        }) 
    ];
    
    this.wand = Overlays.addOverlay("cube", {
        position: ZERO_VECTOR,
        color: COLORS.WHITE,
        dimensions: { x: 0.01, y: 0.01, z: 0.2 },
        alpha: 1.0,
        solid: true,
        visible: false
    });
    
    var _this = this;
    Script.scriptEnding.connect(function() {
        _this.onCleanup();
    });
} 

Wand.prototype = Object.create( ModelBase.prototype );

Wand.prototype.setVisible = function(visible) {
    ModelBase.prototype.setVisible.call(this, visible);
    Overlays.editOverlay(this.pointers[0], {
        visible: this.visible
    });
    Overlays.editOverlay(this.pointers[1], {
        visible: this.visible
    });
    Overlays.editOverlay(this.wand, {
        visible: this.visible
    });
}

Wand.prototype.setTransform = function(transform) {
    ModelBase.prototype.setTransform.call(this, transform);
    var wandPosition = Vec3.sum(this.position, Vec3.multiply(0.5, this.tipVector));
    Overlays.editOverlay(this.pointers[0], {
        position: this.tipPosition,
        rotation: this.rotation,
        visible: true,
    });
    Overlays.editOverlay(this.pointers[1], {
        position: this.tipPosition,
        rotation: Quat.multiply(this.POINTER_ROTATION, this.rotation),
        visible: true,
    });
    Overlays.editOverlay(this.wand, {
        dimensions: { x: 0.01, y: this.length * 0.9, z: 0.01 },            
        position: wandPosition,
        rotation: this.rotation,
        visible: true,
    });
}

Wand.prototype.setTipColors = function(color1, color2) {
    Overlays.editOverlay(this.pointers[0], {
        color: color1 || this.DEFAULT_TIP_COLORS[0],
    });
    Overlays.editOverlay(this.pointers[1], {
        color: color2 || this.DEFAULT_TIP_COLORS[1],
    });
}

Wand.prototype.onUpdate = function(deltaTime) {
    if (this.visible) {
        var time = new Date().getTime() / 250;
        var scale1 = Math.abs(Math.sin(time));
        var scale2 = Math.abs(Math.cos(time));
        Overlays.editOverlay(this.pointers[0], {
            scale: scale1 * 0.01,
        });
        Overlays.editOverlay(this.pointers[1], {
            scale: scale2 * 0.01,
        });
    }
}

Wand.prototype.onCleanup = function() {
    Overlays.deleteOverlay(this.pointers[0]);
    Overlays.deleteOverlay(this.pointers[1]);
    Overlays.deleteOverlay(this.wand);
}