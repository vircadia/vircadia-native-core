//
//  clonedOverlaysExample.js
//  examples
//
//  Created by Thijs Wenker on 11/13/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Demonstrates the use of the overlay cloning function.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const NUM_OF_TREES = 40;
const NUM_OF_SANTAS = 20;

// Image source: https://openclipart.org/detail/447/christmas-tree-by-theresaknott (heavily edited by Maximillian Merlin)
const CHRISTMAS_TREE_SPRITES_URL = "https://s3.amazonaws.com/hifi-public/models/props/xmas/christmas-tree.svg";

// Image source: http://opengameart.org/content/santa-claus (CC-BY 3.0)
const SANTA_SPRITES_URL = "https://s3.amazonaws.com/hifi-public/models/props/xmas/Santa.png";

Array.prototype.contains = function(obj) {
    var i = this.length;
    while (i--) {
        if (this[i] === obj) {
            return true;
        }
    }
    return false;
};

function getRandomPosAroundMyAvatar(radius, height_offset) {
    if (height_offset == undefined) {
        height_offset = 0;
    }
    return {x: MyAvatar.position.x + Math.floor(Math.random() * radius * 2) - radius, y: MyAvatar.position.y + height_offset, z: MyAvatar.position.z + Math.floor(Math.random() * radius * 2) - radius};
}

function OverlayPreloader(overlay_type, overlay_properties, loaded_callback) {
    var _this = this;
    this.loaded_callback = loaded_callback;
    this.overlay = Overlays.addOverlay(overlay_type, overlay_properties);
    this.timer = null;
    this.timer = Script.setInterval(function() { 
        if (Overlays.isLoaded(_this.overlay)) {
            Script.clearInterval(_this.timer);
            _this.loaded_callback();
        }
    }, 100);
}

function SpriteBillboard(sprite_properties, overlay) {
    var _this = this;
    
    // set properties
    this.overlay = overlay;
    this.overlay_properties = {};
    this.sprite_properties = sprite_properties;
    this.edited_overlay_properties = [];
    this.defaultFPS = 30;
    this.currentSequence = "";
    this.sequenceIndex = 0;
    this.sequenceFPS = 0;
    this.sequenceStepToNext = false;
    this.sequenceTimer = null;
    this.prevSequenceIndex = -1;
    this.sequenceX = 0;
    this.sequenceY = 0;
    
    this.spriteSize = {x: 0, y: 0, width: 32, height: 64};
    
    this.editedProperty = function(prop_name) {
        if (!_this.edited_overlay_properties.contains(prop_name)) {
            _this.edited_overlay_properties.push(prop_name);
        }
    };

    // function definitions
    this.getPosition = function() {
        return _this.overlay_properties.position ?
            _this.overlay_properties.position : {x: 0, y: 0, z: 0};
    };

    this.setPosition = function(newPosition) {
        this.overlay_properties.position = newPosition;
        this.editedProperty("position");
        return _this;
    };

    this.setAlpha = function(alpha) {
        this.overlay_properties.alpha = alpha;
        this.editedProperty("alpha");
        return _this;
    }

    this.setScale = function(scale) {
        this.overlay_properties.scale = scale;
        this.editedProperty("scale");
        return _this;
    }

    this.setSpriteSize = function(spriteSize) {
        this.overlay_properties.subImage = spriteSize;
        this.editedProperty("subImage");
        return _this;
    };
    
    this.setSpriteXIndex = function(x_index) {
        _this.sequenceX = x_index;
        _this.overlay_properties.subImage.x = x_index * _this.overlay_properties.subImage.width;
        _this.editedProperty("subImage");
        return _this;
    }
    
    this.setSpriteYIndex = function(y_index) {
        _this.sequenceY = y_index;
        _this.overlay_properties.subImage.y = y_index * _this.overlay_properties.subImage.height;
        _this.editedProperty("subImage");
        return _this;
    }

    this.editOverlay = function(properties) {
        for (var key in properties) {
            _this.overlay_properties[attrname] = properties[key];
            _this.editedProperty(key);
        }
        return _this;
    };

    this.commitChanges = function() {
        var changed_properties = {};
        for (var i = 0; i < _this.edited_overlay_properties.length; i++) {
             var key = _this.edited_overlay_properties[i];
             changed_properties[key] = _this.overlay_properties[key];
        }
        if (Object.keys(changed_properties).length === 0) {
            return;
        }
        Overlays.editOverlay(_this.overlay, changed_properties);
        _this.edited_overlay_properties = [];
    };

    this._renderFrame = function() {
        var currentSequence = _this.sprite_properties.sequences[_this.currentSequence];
        var currentItem = currentSequence[_this.sequenceIndex];
        var indexChanged = _this.sequenceIndex != _this.prevSequenceIndex;
        var canMoveToNext = true;
        _this.prevSequenceIndex = _this.sequenceIndex;
        if (indexChanged) {
            if (currentItem.loop != undefined) {
                _this.loopSequence = currentItem.loop;
            }
            if (currentItem.fps !== undefined && currentItem.fps != _this.sequenceFPS) {
                _this.startSequenceTimerFPS(currentItem.fps);
            }
            if (currentItem.step_to_next !== undefined) {
                _this.sequenceStepToNext = currentItem.step_to_next;
            }
            if (currentItem.x !== undefined) {
                _this.setSpriteXIndex(currentItem.x);
            }
            if (currentItem.y !== undefined) {
                _this.setSpriteYIndex(currentItem.y);
            }
            
            if (_this.sequenceStepToNext) {
                canMoveToNext = false;
            }
        }
        _this.prevSequenceIndex = _this.sequenceIndex;
        var nextIndex = (_this.sequenceIndex + 1) % currentSequence.length;
        var nextItem = currentSequence[nextIndex];
        var nextX = nextItem.x != undefined ? nextItem.x : _this.sequenceX;
        var nextY = nextItem.Y != undefined ? nextItem.Y : _this.sequenceY;
        
        if (_this.sequenceStepToNext && !indexChanged) {
            var XMoveNext = true;
            var YMoveNext = true;
            if (Math.abs(nextX - _this.sequenceX) > 1) {
                _this.setSpriteXIndex(_this.sequenceX + (nextX > _this.sequenceX ? 1 : -1));
                XMoveNext = Math.abs(nextX - _this.sequenceX) == 1;
            }
            if (Math.abs(nextY - _this.sequenceY) > 1) {
                _this.setSpriteYIndex(_this.sequenceY + (nextY > _this.sequenceY ? 1 : -1));
                YMoveNext = Math.abs(nextY - _this.sequenceY) == 1;
            }
            canMoveToNext = XMoveNext && YMoveNext;
        }
        if (canMoveToNext) {
            _this.sequenceIndex = nextIndex;
        }
        _this.commitChanges();
        
    };

    this.clone = function() {
        var clone = {};
        clone.prototype = this.prototype;
        for (property in this) {
            clone[property] = this[property];
        }
        return clone;
    };
    
    this.startSequenceTimerFPS = function(fps) {
        _this.sequenceFPS = fps;
        if (_this.sequenceTimer != null) {
            Script.clearInterval(_this.sequenceTimer);
        }
        _this.sequenceTimer = Script.setInterval(_this._renderFrame, 1000 / fps);
    }

    this.start = function(sequenceName) {
        this.currentSequence = sequenceName;
        this.sequenceFPS = this.defaultFPS;
        this.startSequenceTimerFPS(this.defaultFPS);
    };

    if (this.sprite_properties.url !== undefined) {
        this.setURL(this.sprite_properties.url);
    }

    if (this.sprite_properties.sprite_size !== undefined) {
        this.setSpriteSize(this.sprite_properties.sprite_size);
    }

    if (this.sprite_properties.scale !== undefined) {
        this.setScale(this.sprite_properties.scale);
    }

    if (this.sprite_properties.alpha !== undefined) {
        this.setAlpha(this.sprite_properties.alpha);
    }

    if (this.sprite_properties.position !== undefined) {
        this.setPosition(this.sprite_properties.position);
    }

    _this.commitChanges();

    if (this.sprite_properties.startup_sequence != undefined) {
        this.start(this.sprite_properties.startup_sequence);
    }
    
    Script.scriptEnding.connect(function () {
        if (_this.sequenceTimer != null) {
            Script.clearInterval(_this.sequenceTimer);
        }
        Overlays.deleteOverlay(_this.overlay);
    });
}

var christmastree_loader = null;
christmastree_loader = new OverlayPreloader("image3d",
    {url: CHRISTMAS_TREE_SPRITES_URL, alpha: 0}, function() {
        for (var i = 0; i < NUM_OF_TREES; i++) {
            var clonedOverlay = Overlays.cloneOverlay(christmastree_loader.overlay);
            new SpriteBillboard({
                position: getRandomPosAroundMyAvatar(20),
                sprite_size: {x: 0, y: 0, width: 250.000, height: 357.626},
                scale: 6,
                alpha: 1,
                sequences: {"idle": [{x: 0, y: 0, step_to_next: true, fps: 3}, {x: 3, step_to_next: false}]},
                startup_sequence: "idle"
            }, clonedOverlay);
        }
    }
);

var santa_loader = null;
santa_loader = new OverlayPreloader("image3d",
    {url: SANTA_SPRITES_URL, alpha: 0}, function() {
        for (var i = 0; i < NUM_OF_SANTAS; i++) {
            var clonedOverlay = Overlays.cloneOverlay(santa_loader.overlay);
            new SpriteBillboard({
                position: getRandomPosAroundMyAvatar(18, -1),
                sprite_size: {x: 0, y: 0, width: 64, height:  72},
                scale: 4,
                alpha: 1,
                sequences: {
                    "walk_left": [{x: 2, y: 0, step_to_next: true, fps: 4}, {x: 10, step_to_next: false}],
                    "walk_right": [{x: 10, y: 1, step_to_next: true, fps: 4}, {x: 2, step_to_next: false}],
                },
                startup_sequence: (i % 2 == 0) ? "walk_left" : "walk_right"
            }, clonedOverlay);
        }
    }
);

Script.scriptEnding.connect(function () {
    Overlays.deleteOverlay(christmastree_loader.overlay);
    Overlays.deleteOverlay(santa_loader.overlay);
});
