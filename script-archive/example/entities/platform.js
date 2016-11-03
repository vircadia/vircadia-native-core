//
// platform.js
//
// Created by Seiji Emery on 8/19/15
// Copyright 2015 High Fidelity, Inc.
//
// Entity stress test / procedural demo.
// Spawns a platform under your avatar made up of randomly sized and colored boxes or spheres. The platform follows your avatar
// around, and comes with a UI to update the platform's properties (radius, entity density, color distribution, etc) in real time.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// UI and debug console implemented using uiwidgets / 2d overlays
Script.include("../../libraries/uiwidgets.js");
if (typeof(UI) === 'undefined') {   // backup link in case the user downloaded this somewhere
    print("Missing library script -- loading from public.highfidelity.io");
    Script.include('http://public.highfidelity.io/scripts/libraries/uiwidgets.js');
    if (typeof(UI) === 'undefined') {
        print("Cannot load UIWidgets library -- check your internet connection", COLORS.RED);
        throw new Error("Could not load uiwidgets.js");
    }
}

// Platform script
(function () {
var SCRIPT_NAME = "platform.js";
var USE_DEBUG_LOG = true;   // Turns on the 2dOverlay-based debug log. If false, just redirects to print.
var NUM_DEBUG_LOG_LINES = 10;
var LOG_ENTITY_CREATION_MESSAGES = false;   // detailed debugging (init)
var LOG_UPDATE_STATUS_MESSAGES = false;     // detailed debugging (startup)

var MAX_UPDATE_INTERVAL = 0.2;  // restrict to 5 updates / sec
var AVATAR_HEIGHT_OFFSET = 1.5; // offset to make the platform spawn under your feet. Might need to be adjusted for unusually proportioned avatars.

var USE_ENTITY_TIMEOUTS = true;
var ENTITY_TIMEOUT_DURATION = 30.0; // kill entities in 30 secs if they don't get any updates
var ENTITY_REFRESH_INTERVAL = 10.0; // poke the entities every 10s so they don't die until we're done with them

// Initial state
var NUM_PLATFORM_ENTITIES = 400;
var RADIUS = 5.0;

// Defines min/max for onscreen platform radius, density, and entity width/height/depth sliders.
// Color limits are hardcoded at [0, 255].
var PLATFORM_RADIUS_RANGE  = [ 1.0, 15.0 ];
var PLATFORM_DENSITY_RANGE = [ 0.0, 35.0 ]; // do NOT increase this above 40! (~20k limit). Entity count = Math.PI * radius * radius * density.
var PLATFORM_SHAPE_DIMENSIONS_RANGE = [ 0.001, 2.0 ]; // axis-aligned entity dimension limits

// Utils
(function () {
    if (typeof(Math.randRange) === 'undefined') {
        Math.randRange = function (min, max) {
            return Math.random() * (max - min) + min;
        }
    }
    if (typeof(Math.randInt) === 'undefined') {
        Math.randInt = function (n) {
            return Math.floor(Math.random() * n) | 0;
        }
    }
    function fromComponents (r, g, b, a) {
        this.red   = r;
        this.green = g;
        this.blue  = b;
        this.alpha = a || 1.0;
    }
    function fromHex (c) {
        this.red   = parseInt(c[1] + c[2], 16);
        this.green = parseInt(c[3] + c[4], 16);
        this.blue  = parseInt(c[5] + c[6], 16);
    }
    var Color = this.Color = function () {
        if (arguments.length >= 3) {
            fromComponents.apply(this, arguments);
        } else if (arguments.length == 1 && arguments[0].length == 7 && arguments[0][0] == '#') {
            fromHex.apply(this, arguments);
        } else {
            throw new Error("Invalid arguments to new Color(): " + JSON.stringify(arguments));
        }
    }
    Color.prototype.toString = function () {
        return "[Color: " + JSON.stringify(this) + "]";
    }
})();

// RNG models
(function () {
    /// Encapsulates a simple color model that generates colors using a linear, pseudo-random color distribution.
    var RandomColorModel = this.RandomColorModel = function () {
        this.shadeRange = 0;    // = 200;
        this.minColor = 55;     // = 100;
        this.redRange = 255;    // = 200;
        this.greenRange = 0;    // = 10;
        this.blueRange = 0;     // = 0;
    };
    /// Generates 4 numbers in [0, 1] corresponding to each color attribute (uniform shade and additive red, green, blue).
    /// This is done in a separate step from actually generating the colors, since it allows us to either A) completely
    /// rebuild / re-randomize the color values, or B) reuse the RNG values but with different color parameters, which
    /// enables us to do realtime color editing on the same visuals (awesome!).
    RandomColorModel.prototype.generateSeed = function () {
        return [ Math.random(), Math.random(), Math.random(), Math.random() ];
    };
    /// Takes a random 'seed' (4 floats from this.generateSeed()) and calculates a pseudo-random
    /// color by combining that with the color model's current parameters.
    RandomColorModel.prototype.getRandom = function (r) {
        // logMessage("color seed values " + JSON.stringify(r));
        var shade = Math.min(255, this.minColor + r[0] * this.shadeRange);

        // No clamping on the color components, so they may overflow. 
        // However, this creates some pretty interesting visuals, so we're not "fixing" this.
        var color = {
            red:   shade + r[1] * this.redRange,
            green: shade + r[2] * this.greenRange,
            blue:  shade + r[3] * this.blueRange
        };
        // logMessage("this: " + JSON.stringify(this));
        // logMessage("color: " + JSON.stringify(color), COLORS.RED);
        return color;
    };
    /// Custom property iterator used to setup UI (sliders, etc)
    RandomColorModel.prototype.setupUI = function (callback) {
        var _this = this;
        [ 
            ['shadeRange', 'shade range'], 
            ['minColor', 'shade min'], 
            ['redRange', 'red (additive)'],
            ['greenRange', 'green (additive)'], 
            ['blueRange', 'blue (additive)'] 
        ].forEach(function (v) {
            //       name, value, min, max, onValueChanged
            callback(v[1], _this[v[0]], 0, 255, function (value) { _this[v[0]] = value });
        });
    }

    /// Generates pseudo-random dimensions for our cubes / shapes.
    var RandomShapeModel = this.RandomShapeModel = function () {
        this.widthRange = [ 0.3, 0.7 ];
        this.depthRange = [ 0.5, 0.8 ];
        this.heightRange = [ 0.01, 0.08 ];
    };
    /// Generates 3 seed numbers in [0, 1]
    RandomShapeModel.prototype.generateSeed = function () {
        return [ Math.random(), Math.random(), Math.random() ];
    }
    /// Combines seed values with width/height/depth ranges to produce vec3 dimensions for a cube / sphere.
    RandomShapeModel.prototype.getRandom = function (r) {
        return {
            x: r[0] * (this.widthRange[1] - this.widthRange[0]) + this.widthRange[0],
            y: r[1] * (this.heightRange[1] - this.heightRange[0]) + this.heightRange[0],
            z: r[2] * (this.depthRange[1] - this.depthRange[0]) + this.depthRange[0]
        };
    }
    /// Custom property iterator used to setup UI (sliders, etc)
    RandomShapeModel.prototype.setupUI = function (callback) {
        var _this = this;
        var dimensionsMin = PLATFORM_SHAPE_DIMENSIONS_RANGE[0];
        var dimensionsMax = PLATFORM_SHAPE_DIMENSIONS_RANGE[1];
        [
            ['widthRange', 'width'], 
            ['depthRange', 'depth'], 
            ['heightRange', 'height'] 
        ].forEach(function (v) {
            //       name, value, min, max, onValueChanged
            callback(v[1], _this[v[0]], dimensionsMin, dimensionsMax, function (value) { _this[v[0]] = value });
        });
    }

    /// Combines color + shape PRNG models and hides their implementation details.
    var RandomAttribModel = this.RandomAttribModel = function () {
        this.colorModel = new RandomColorModel();
        this.shapeModel = new RandomShapeModel();
    }
    /// Completely re-randomizes obj's `color` and `dimensions` parameters based on the current model params.
    RandomAttribModel.prototype.randomizeShapeAndColor = function (obj) {
        // logMessage("randomizing " + JSON.stringify(obj));
        obj._colorSeed = this.colorModel.generateSeed();
        obj._shapeSeed = this.shapeModel.generateSeed();
        this.updateShapeAndColor(obj);
        // logMessage("color seed: " + JSON.stringify(obj._colorSeed), COLORS.RED);
        // logMessage("randomized color: " + JSON.stringify(obj.color), COLORS.RED);
        // logMessage("randomized: " + JSON.stringify(obj));
        return obj;
    }
    /// Updates obj's `color` and `dimensions` params to use the current model params.
    /// Reuses hidden seed attribs; _must_ have called randomizeShapeAndColor(obj) at some point before
    /// calling this.
    RandomAttribModel.prototype.updateShapeAndColor = function (obj) {
        try {
            // logMessage("update shape and color: " + this.colorModel);
            obj.color      = this.colorModel.getRandom(obj._colorSeed);
            obj.dimensions = this.shapeModel.getRandom(obj._shapeSeed); 
        } catch (e) {
            logMessage("update shape / color failed", COLORS.RED);
            logMessage('' + e, COLORS.RED);
            logMessage("obj._colorSeed = " + JSON.stringify(obj._colorSeed));
            logMessage("obj._shapeSeed = " + JSON.stringify(obj._shapeSeed));
            // logMessage("obj = " + JSON.stringify(obj));
            throw e;
        }
        return obj;
    }
})();

// Status / logging UI (ignore this)
(function () {
    var COLORS = this.COLORS = {
        'GREEN': new Color("#2D870C"),
        'RED': new Color("#AF1E07"),
        'LIGHT_GRAY': new Color("#CCCCCC"),
        'DARK_GRAY': new Color("#4E4E4E")
    };
    function buildDebugLog () {
        var LINE_WIDTH = 400;
        var LINE_HEIGHT = 20;
    
        var lines = [];
        var lineIndex = 0;
        for (var i = 0; i < NUM_DEBUG_LOG_LINES; ++i) {
            lines.push(new UI.Label({ 
                text: " ", visible: false, 
                width: LINE_WIDTH, height: LINE_HEIGHT,
            }));
        }
        var title = new UI.Label({
            text: SCRIPT_NAME, visible: true,
            width: LINE_WIDTH, height: LINE_HEIGHT,
        });
    
        var overlay = new UI.Box({
            visible: true,
            width: LINE_WIDTH, height: 0,
            backgroundColor: COLORS.DARK_GRAY,
            backgroundAlpha: 0.3
        });
        overlay.setPosition(280, 10);
        relayoutFrom(0);
        UI.updateLayout();
    
        function relayoutFrom (n) {
            var layoutPos = {
                x: overlay.position.x,
                y: overlay.position.y
            };
    
            title.setPosition(layoutPos.x, layoutPos.y);
            layoutPos.y += LINE_HEIGHT;
    
            // for (var i = n; i >= 0; --i) {
            for (var i = n + 1; i < lines.length; ++i) {
                if (lines[i].visible) {
                    lines[i].setPosition(layoutPos.x, layoutPos.y);
                    layoutPos.y += LINE_HEIGHT;
                }
            }
            // for (var i = lines.length - 1; i > n; --i) {
            for (var i = 0; i <= n; ++i) {
                if (lines[i].visible) {
                    lines[i].setPosition(layoutPos.x, layoutPos.y);
                    layoutPos.y += LINE_HEIGHT;
                }
            }
            overlay.height = (layoutPos.y - overlay.position.y + 10);
            overlay.getOverlay().update({
                height: overlay.height
            });
        }
        this.logMessage = function (text, color, alpha) {
            lines[lineIndex].setVisible(true);
            relayoutFrom(lineIndex);
    
            lines[lineIndex].getOverlay().update({
                text: text,
                visible: true,
                color: color || COLORS.LIGHT_GRAY,
                alpha: alpha !== undefined ? alpha : 1.0,
                x: lines[lineIndex].position.x,
                y: lines[lineIndex].position.y
            });
            lineIndex = (lineIndex + 1) % lines.length;
            UI.updateLayout();
        }
    }
    if (USE_DEBUG_LOG) {
        buildDebugLog();
    } else {
        this.logMessage = function (msg) {
            print(SCRIPT_NAME + ": " + msg);
        }
    }
})();

// Utils (ignore)
(function () {
    // Utility function
    var withDefaults = this.withDefaults = function (properties, defaults) {
        // logMessage("withDefaults: " + JSON.stringify(properties) + JSON.stringify(defaults));
        properties = properties || {};
        if (defaults) {
            for (var k in defaults) {
                properties[k] = defaults[k];
            }
        }
        return properties;
    }
    var withReadonlyProp = this.withReadonlyProp = function (propname, value, obj) {
        Object.defineProperty(obj, propname, {
            value: value,
            writable: false
        });
        return obj;
    }

    // Math utils
    if (typeof(Math.randRange) === 'undefined') {
        Math.randRange = function (min, max) {
            return Math.random() * (max - min) + min;
        }
    }
    if (typeof(Math.randInt) === 'undefined') {
        Math.randInt = function (n) {
            return Math.floor(Math.random() * n) | 0;
        }
    }

    /// Random distrib: Get a random point within a circle on the xz plane with radius r, center p.
    this.randomCirclePoint = function (r, pos) {
        var a = Math.random(), b = Math.random();
        if (b < a) {
            var tmp = b;
            b = a;
            a = tmp;
        }
        var point = {
            x: pos.x + b * r * Math.cos(2 * Math.PI * a / b),
            y: pos.y,
            z: pos.z + b * r * Math.sin(2 * Math.PI * a / b)
        };
        if (LOG_ENTITY_CREATION_MESSAGES) {
            // logMessage("input params: " + JSON.stringify({ radius: r, position: pos }), COLORS.GREEN);
            // logMessage("a = " + a + ", b = " + b);
            logMessage("generated point: " + JSON.stringify(point), COLORS.RED);
        }
        return point;
    }

    // Entity utils. NOT using overlayManager for... reasons >.>
    var makeEntity = this.makeEntity = function (properties) {
        if (LOG_ENTITY_CREATION_MESSAGES) {
            logMessage("Creating entity: " + JSON.stringify(properties));
        }
        var entity = Entities.addEntity(properties);
        return withReadonlyProp("type", properties.type, {
            update: function (properties) {
                Entities.editEntity(entity, properties);
            },
            destroy: function () {
                Entities.deleteEntity(entity)
            },
            getId: function () {
                return entity;
            }
        });
    }
    // this.makeLight = function (properties) {
    //  return makeEntity(withDefaults(properties, {
    //      type: "Light",
    //      isSpotlight: false,
    //      diffuseColor: { red: 255, green: 100, blue: 100 },
    //      ambientColor: { red: 200, green: 80, blue: 80 }
    //  }));
    // }
    this.makeBox = function (properties) {
        // logMessage("Creating box: " + JSON.stringify(properties));
        return makeEntity(withDefaults(properties, {
            type: "Box"
        }));
    }
})();

// Platform
(function () {
    /// Encapsulates a platform 'piece'. Owns an entity (`box`), and handles destruction and some other state.
    var PlatformComponent = this.PlatformComponent = function (properties) {
        // logMessage("Platform component initialized with " + Object.keys(properties), COLORS.GREEN);
        this.position   = properties.position || null;
        this.color      = properties.color    || null;
        this.dimensions = properties.dimensions || null;
        this.entityType = properties.type || "Box";

        // logMessage("Spawning with type: '" + this.entityType + "' (properties.type = '" + properties.type + "')", COLORS.GREEN);

        if (properties._colorSeed)
            this._colorSeed = properties._colorSeed;
        if (properties._shapeSeed)
            this._shapeSeed = properties._shapeSeed;

        // logMessage("dimensions: " + JSON.stringify(this.dimensions));
        // logMessage("color: " + JSON.stringify(this.color));

        this.cachedEntity = null;
        this.activeEntity = this.spawnEntity(this.entityType);
    };
    PlatformComponent.prototype.spawnEntity = function (type) {
        return makeEntity({
            type: type,
            position: this.position,
            dimensions: this.dimensions,
            color: this.color,
            lifetime: USE_ENTITY_TIMEOUTS ? ENTITY_TIMEOUT_DURATION : -1.0,
            alpha: 0.5
        });
    }
    if (USE_ENTITY_TIMEOUTS) {
        PlatformComponent.prototype.pokeEntity = function () {
            // Kinda inefficient, but there's no way to get around this :/
            var age = Entities.getEntityProperties(this.activeEntity.getId()).age;
            this.activeEntity.update({ lifetime: ENTITY_TIMEOUT_DURATION + age });
        }
    } else {
        PlatformComponent.prototype.pokeEntity = function () {}
    }
    /// Updates platform to be at position p, and calls <entity>.update() with the current
    /// position, color, and dimensions parameters.
    PlatformComponent.prototype.update = function (position) {
        if (position)
            this.position = position;
        // logMessage("updating with " + JSON.stringify(this));
        this.activeEntity.update(this);
    }
    function swap (a, b) {
        var tmp = a;
        a = b;
        b = tmp;
    }   
    PlatformComponent.prototype.swapEntityType = function (newType) {
        if (this.entityType !== newType) {
            this.entityType = newType;
            // logMessage("Destroying active entity and rebuilding it (newtype = '" + newType + "')");
            if (this.activeEntity) {
                this.activeEntity.destroy();
            }
            this.activeEntity = this.spawnEntity(newType);
            // if (this.cachedEntity && this.cachedEntity.type == newType) {
            //  this.cachedEntity.update({ visible: true });
            //  this.activeEntity.update({ visible: false });
            //  swap(this.cachedEntity, this.activeEntity);
            //  this.update(this.position);
            // } else {
            //  this.activeEntity.update({ visible: false });
            //  this.cachedEntity = this.activeEntity;
            //  this.activeEntity = spawnEntity(newType);
            // }
        }
    }
    /// Swap state with another component
    PlatformComponent.prototype.swap = function (other) {
        swap(this.position, other.position);
        swap(this.dimensions, other.dimensions);
        swap(this.color, other.color);
        swap(this.entityType, other.entityType);
        swap(this.activeEntity, other.activeEntity);
        swap(this._colorSeed, other._colorSeed);
        swap(this._shapeSeed, other._shapeSeed);
    }
    PlatformComponent.prototype.destroy = function () {
        if (this.activeEntity) {
            this.activeEntity.destroy();
            this.activeEntity = null;
        }
        if (this.cachedEntity) {
            this.cachedEntity.destroy();
            this.cachedEntity = null;
        }
    }

    // util
    function inRange (p1, p2, radius) {
        return Vec3.distance(p1, p2) < Math.abs(radius);
    }

    /// Encapsulates a moving platform that follows the avatar around (mostly).
    var DynamicPlatform = this.DynamicPlatform = function (n, position, radius) {
        this.position = position;
        this.radius   = radius;
        this.randomizer = new RandomAttribModel();
        this.boxType = "Box";
        this.boxTypes = [ "Box", "Sphere" ];

        logMessage("Spawning " + n + " entities", COLORS.GREEN);
        var boxes = this.boxes = [];
        while (n > 0) {
            boxes.push(this.spawnEntity());
            --n;
        }
        this.targetDensity = this.getEntityDensity();
        this.pendingUpdates = {};
        this.updateTimer = 0.0;

        this.platformHeight = position.y;
        this.oldPos    = { x: position.x, y: position.y, z: position.z };
        this.oldRadius = radius;

        // this.sendPokes();
    }
    DynamicPlatform.prototype.toString = function () {
        return "[DynamicPlatform (" + this.boxes.length + " entities)]";
    }
    DynamicPlatform.prototype.spawnEntity = function () {
        // logMessage("Called spawn entity. this.boxType = '" + this.boxType + "'")
        var properties = { position: this.randomPoint(), type: this.boxType };
        this.randomizer.randomizeShapeAndColor(properties);
        return new PlatformComponent(properties);
    }
    DynamicPlatform.prototype.updateEntityAttribs = function () {
        var _this = this;
        this.setPendingUpdate('updateEntityAttribs', function () {
            // logMessage("updating model", COLORS.GREEN);
            _this.boxes.forEach(function (box) {
                this.randomizer.updateShapeAndColor(box);
                box.update();
            }, _this);
        });
    }
    DynamicPlatform.prototype.toggleBoxType = function () {
        var _this = this;
        this.setPendingUpdate('toggleBoxType', function () {
            // Swap / cycle through types: find index of current type and set next type to idx+1
            for (var idx = 0; idx < _this.boxTypes.length; ++idx) {
                if (_this.boxTypes[idx] === _this.boxType) {
                    var nextIndex = (idx + 1) % _this.boxTypes.length;
                    logMessage("swapping box type from '" + _this.boxType + "' to '" + _this.boxTypes[nextIndex] + "'", COLORS.GREEN);
                    _this.boxType = _this.boxTypes[nextIndex];
                    break;
                }
            }
            _this.boxes.forEach(function (box) {
                box.swapEntityType(_this.boxType);
            }, _this);
        });
    }
    DynamicPlatform.prototype.getBoxType = function () {
        return this.boxType;
    }

    // if (USE_ENTITY_TIMEOUTS) {
    //  DynamicPlatform.prototype.sendPokes = function () {
    //      var _this = this;
    //      function poke () {
    //          logMessage("Poking entities so they don't die", COLORS.GREEN);
    //          _this.boxes.forEach(function (box) {
    //              box.pokeEntity();
    //          }, _this);


    //          if (_this.pendingUpdates['keepalive']) {
    //              logMessage("previous timer: " + _this.pendingUpdates['keepalive'].timer + "; new timer: " + ENTITY_REFRESH_INTERVAL)
    //          }
    //          _this.pendingUpdates['keepalive'] = {
    //              callback: poke,
    //              timer: ENTITY_REFRESH_INTERVAL,
    //              skippedUpdates: 0
    //          };
    //          // _this.setPendingUpdate('keepalive', poke);
    //          // _this.pendingUpdates['keepalive'].timer = ENTITY_REFRESH_INTERVAL;
    //      }
    //      poke();
    //  }
    // } else {
    //  DynamicPlatform.prototype.sendPokes = function () {};
    // }

    /// Queue impl that uses the update loop to limit potentially expensive updates to only execute every x seconds (default: 200 ms).
    /// This is to prevent UI code from running full entity updates every 10 ms (or whatever).
    DynamicPlatform.prototype.setPendingUpdate = function (name, callback) {
        if (!this.pendingUpdates[name]) {
            // logMessage("Queued update for " + name, COLORS.GREEN);
            this.pendingUpdates[name] = {
                callback: callback,
                timer: 0.0,
                skippedUpdates: 0
            }
        } else {
            // logMessage("Deferred update for " + name, COLORS.GREEN);
            this.pendingUpdates[name].callback = callback;
            this.pendingUpdates[name].skippedUpdates++;
            // logMessage("scheduling update for \"" + name + "\" to run in " + this.pendingUpdates[name].timer + " seconds");
        }
    }
    /// Runs all queued updates as soon as they can execute (each one has a cooldown timer).
    DynamicPlatform.prototype.processPendingUpdates = function (dt) {
        for (var k in this.pendingUpdates) {
            if (this.pendingUpdates[k].timer >= 0.0)
                this.pendingUpdates[k].timer -= dt;

            if (this.pendingUpdates[k].callback && this.pendingUpdates[k].timer < 0.0) {
                // logMessage("Dispatching update for " + k);
                try {
                    this.pendingUpdates[k].callback();
                } catch (e) {
                    logMessage("update for \"" + k + "\" failed: " + e, COLORS.RED);
                }
                this.pendingUpdates[k].timer = MAX_UPDATE_INTERVAL;
                this.pendingUpdates[k].skippedUpdates = 0;
                this.pendingUpdates[k].callback = null;
            } else {
                // logMessage("Deferred update for " + k + " for " + this.pendingUpdates[k].timer + " seconds");
            }
        }
    }

    /// Updates the platform based on the avatar's current position (spawning / despawning entities as needed),
    /// and calls processPendingUpdates() once this is done.
    /// Does NOT have any update interval limits (it just updates every time it gets run), but these are not full
    /// updates (they're incremental), so the network will not get flooded so long as the avatar is moving at a
    /// normal walking / flying speed.
    DynamicPlatform.prototype.updatePosition = function (dt, position) {
        // logMessage("updating " + this);
        position.y = this.platformHeight;
        this.position = position;

        var toUpdate = [];
        this.boxes.forEach(function (box, i) {
            // if (Math.abs(box.position.y - position.y) > HEIGHT_TOLERANCE || !inRange(box, position, radius)) {
            if (!inRange(box.position, this.position, this.radius)) {
                toUpdate.push(i);
            }
        }, this);

        var MAX_TRIES = toUpdate.length * 8;
        var tries = MAX_TRIES;
        var moved = 0;
        var recalcs = 0;
        toUpdate.forEach(function (index) {
            if ((index % 2 == 0) || tries > 0) {
                do {
                    var randomPoint = this.randomPoint(this.position, this.radius);
                    ++recalcs
                } while (--tries > 0 && inRange(randomPoint, this.oldPos, this.oldRadiuss));
    
                if (LOG_UPDATE_STATUS_MESSAGES && tries <= 0) {
                    logMessage("updatePlatform() gave up after " + MAX_TRIES + " iterations (" + moved + " / " + toUpdate.length + " successful updates)", COLORS.RED);
                    logMessage("old pos: " + JSON.stringify(this.oldPos) + ", old radius: " + this.oldRadius);
                    logMessage("new pos: " + JSON.stringify(this.position) + ", new radius: " + this.radius);
                }
            } else {
                var randomPoint = this.randomPoint(position, this.radius);
            }

            this.randomizer.randomizeShapeAndColor(this.boxes[index]);
            this.boxes[index].update(randomPoint);
            // this.boxes[index].setValues({
            //  position: randomPoint,
            //  // dimensions: this.randomDimensions(),
            //  // color: this.randomColor()
            // });
            ++moved;
        }, this);
        recalcs = recalcs - toUpdate.length;

        this.oldPos    = position;
        this.oldRadius = this.radius;
        if (LOG_UPDATE_STATUS_MESSAGES && toUpdate.length > 0) {
            logMessage("updated " + toUpdate.length + " entities w/ " + recalcs + " recalcs");
        }
    }

    DynamicPlatform.prototype.update = function (dt, position) {
        this.updatePosition(dt, position);
        this.processPendingUpdates(dt);
        this.sendPokes(dt);
    }

    if (USE_ENTITY_TIMEOUTS) {
        DynamicPlatform.prototype.sendPokes = function (dt) {
            logMessage("starting keepalive", COLORS.GREEN);
            // logMessage("dt = " + dt, COLORS.RED);
            // var original = this.sendPokes;
            var pokeTimer = 0.0;
            this.sendPokes = function (dt) {
                // logMessage("dt = " + dt);
                if ((pokeTimer -= dt) < 0.0) {
                    // logMessage("Poking entities so they don't die", COLORS.GREEN);
                    this.boxes.forEach(function (box) {
                        box.pokeEntity();
                    }, this);
                    pokeTimer = ENTITY_REFRESH_INTERVAL;
                } else {
                    // logMessage("Poking entities in " + pokeTimer + " seconds");
                }
            }
            // logMessage("this.sendPokes === past this.sendPokes? " + (this.sendPokes === original), COLORS.GREEN);
            this.sendPokes(dt);
        }
    } else {
        DynamicPlatform.prototype.sendPokes = function () {};
    }
    DynamicPlatform.prototype.getEntityCount = function () {
        return this.boxes.length;
    }
    DynamicPlatform.prototype.getEntityCountWithRadius = function (radius) {
        var est = Math.floor((radius * radius) / (this.radius * this.radius) * this.getEntityCount());
        var actual = Math.floor(Math.PI * radius * radius * this.getEntityDensity());

        if (est != actual) {
            logMessage("assert failed: getEntityCountWithRadius() -- est " + est + " != actual " + actual);
        }
        return est;
    }
    DynamicPlatform.prototype.getEntityCountWithDensity = function (density) {
        return Math.floor(Math.PI * this.radius * this.radius * density);
    }

    /// Sets the entity count to n. Don't call this directly -- use setRadius / density instead.
    DynamicPlatform.prototype.setEntityCount = function (n) {
        if (n > this.boxes.length) {
            // logMessage("Setting entity count to " + n + " (adding " + (n - this.boxes.length) + " entities)", COLORS.GREEN);

            // Spawn new boxes
            n = n - this.boxes.length;
            for (; n > 0; --n) {
                // var properties = { position: this.randomPoint() };
                // this.randomizer.randomizeShapeAndColor(properties);
                // this.boxes.push(new PlatformComponent(properties));
                this.boxes.push(this.spawnEntity());
            }
        } else if (n < this.boxes.length) {
            // logMessage("Setting entity count to " + n + " (removing " + (this.boxes.length - n) + " entities)", COLORS.GREEN);

            // Destroy random boxes (technically, the most recent ones, but it should be sorta random)
            n = this.boxes.length - n;
            for (; n > 0; --n) {
                this.boxes.pop().destroy();
            }
        }
    }
    /// Calculate the entity density based on radial surface area.
    DynamicPlatform.prototype.getEntityDensity = function () {
        return (this.boxes.length * 1.0) / (Math.PI * this.radius * this.radius);
    }
    /// Queues a setDensity update. This is expensive, so we don't call it directly from UI.
    DynamicPlatform.prototype.setDensityOnNextUpdate = function (density) {
        var _this = this;
        this.targetDensity = density;
        this.setPendingUpdate('density', function () {
            _this.updateEntityDensity(density);
        });
    }
    DynamicPlatform.prototype.updateEntityDensity = function (density) {
        this.setEntityCount(Math.floor(density * Math.PI * this.radius * this.radius));
    }
    DynamicPlatform.prototype.getRadius = function () {
        return this.radius;
    }
    /// Queues a setRadius update. This is expensive, so we don't call it directly from UI.
    DynamicPlatform.prototype.setRadiusOnNextUpdate = function (radius) {
        var _this = this;
        this.setPendingUpdate('radius', function () {
            _this.setRadius(radius);
        });
    }
    var DEBUG_RADIUS_RECALC = false;
    DynamicPlatform.prototype.setRadius = function (radius) {
        if (radius < this.radius) { // Reduce case
            // logMessage("Setting radius to " + radius + " (shrink by " + (this.radius - radius) + ")", COLORS.GREEN );
            this.radius = radius;

            // Remove all entities outside of current bounds. Requires swapping, since we want to maintain a contiguous array.
            // Algorithm: two pointers at front and back. We traverse fwd and back, swapping elems so that all entities in bounds
            // are at the front of the array, and all entities out of bounds are at the back. We then pop + destroy all entities
            // at the back to reduce the entity count.
            var count = this.boxes.length;
            var toDelete = 0;
            var swapList = [];
            if (DEBUG_RADIUS_RECALC) {
                logMessage("starting at i = 0, j = " + (count - 1));
            }
            for (var i = 0, j = count - 1; i < j; ) {
                // Find first elem outside of bounds that we can move to the back
                while (inRange(this.boxes[i].position, this.position, this.radius) && i < j) {
                    ++i;
                }
                // Find first elem in bounds that we can move to the front
                while (!inRange(this.boxes[j].position, this.position, this.radius) && i < j) {
                    --j; ++toDelete;
                }
                if (i < j) {
                    // swapList.push([i, j]);
                    if (DEBUG_RADIUS_RECALC) {
                        logMessage("swapping " + i + ", " + j); 
                    }
                    this.boxes[i].swap(this.boxes[j]);
                    ++i, --j; ++toDelete;
                } else {
                    if (DEBUG_RADIUS_RECALC) {
                        logMessage("terminated at i = " + i + ", j = " + j, COLORS.RED);
                    }
                }
            }
            if (DEBUG_RADIUS_RECALC) {
                logMessage("toDelete = " + toDelete, COLORS.RED);
            }
            // Sanity check
            if (toDelete > this.boxes.length) {
                logMessage("Error: toDelete " + toDelete + " > entity count " + this.boxes.length + " (setRadius algorithm)", COLORS.RED);
                toDelete = this.boxes.length;
            }
            if (toDelete > 0) {
                // logMessage("Deleting " + toDelete + " entities as part of radius resize", COLORS.GREEN);
            }
            // Delete cleared boxes
            for (; toDelete > 0; --toDelete) {
                this.boxes.pop().destroy();
            }
            // fix entity density (just in case -- we may have uneven entity distribution)
            this.updateEntityDensity(this.targetDensity);
        } else if (radius > this.radius) {
            // Grow case (much simpler)
            // logMessage("Setting radius to " + radius + " (grow by " + (radius - this.radius) + ")", COLORS.GREEN);

            // Add entities based on entity density
            // var density = this.getEntityDensity();
            var density = this.targetDensity;
            var oldArea = Math.PI * this.radius * this.radius;
            var n = Math.floor(density * Math.PI * (radius * radius - this.radius * this.radius));

            if (n > 0) {
                // logMessage("Adding " + n + " entities", COLORS.GREEN);

                // Add entities (we use a slightly different algorithm to place them in the area between two concentric circles.
                // This is *slightly* less uniform (the reason we're not using this everywhere is entities would be tightly clustered
                // at the platform center and become spread out as the radius increases), but the use-case here is just incremental 
                // radius resizes and the user's not likely to notice the difference).
                for (; n > 0; --n) {
                    var theta = Math.randRange(0.0, Math.PI * 2.0);
                    var r = Math.randRange(this.radius, radius);
                    // logMessage("theta = " + theta + ", r = " + r);
                    var pos = {
                        x: Math.cos(theta) * r + this.position.x,
                        y: this.position.y,
                        z: Math.sin(theta) * r + this.position.y
                    };

                    // var properties = { position: pos };
                    // this.randomizer.randomizeShapeAndColor(properties);
                    // this.boxes.push(new PlatformComponent(properties));
                    this.boxes.push(this.spawnEntity());
                }
            }
            this.radius = radius;
        }
    }
    DynamicPlatform.prototype.updateHeight = function (height) {
        logMessage("Setting platform height to " + height);
        this.platformHeight = height;

        // Invalidate current boxes to trigger a rebuild
        this.boxes.forEach(function (box) {
            box.position.x += this.oldRadius * 100;
        });
        // this.update(dt, position, radius);
    }
    /// Gets a random point within the platform bounds.
    /// Should maybe get moved to the RandomAttribModel (would be much cleaner), but this works for now.
    DynamicPlatform.prototype.randomPoint = function (position, radius) {
        position = position || this.position;
        radius = radius !== undefined ? radius : this.radius;
        return randomCirclePoint(radius, position);
    }
    /// Old. The RandomAttribModel replaces this and enables realtime editing of the *****_RANGE params.
    // DynamicPlatform.prototype.randomDimensions = function () {
    //  return {
    //      x: Math.randRange(WIDTH_RANGE[0], WIDTH_RANGE[1]),
    //      y: Math.randRange(HEIGHT_RANGE[0], HEIGHT_RANGE[1]),
    //      z: Math.randRange(DEPTH_RANGE[0], DEPTH_RANGE[1])
    //  };
    // }
    // DynamicPlatform.prototype.randomColor = function () {
    //  var shade = Math.randRange(SHADE_RANGE[0], SHADE_RANGE[1]);
    //  // var h = HUE_RANGE;
    //  return {
    //      red:    shade + Math.randRange(RED_RANGE[0], RED_RANGE[1]) | 0,
    //      green:  shade + Math.randRange(GREEN_RANGE[0], GREEN_RANGE[1]) | 0,
    //      blue:   shade + Math.randRange(BLUE_RANGE[0], BLUE_RANGE[1]) | 0
    //  }
    //  // return COLORS[Math.randInt(COLORS.length)]
    // }

    /// Cleanup.
    DynamicPlatform.prototype.destroy = function () {
        this.boxes.forEach(function (box) {
            box.destroy();
        });
        this.boxes = [];
    }
})();

// UI
(function () {
    var CATCH_SETUP_ERRORS = true;

    // Util functions for setting up widgets (the widget library is intended to be used like this)
    function makePanel (dir, properties) {
        return new UI.WidgetStack(withDefaults(properties, {
            dir: dir
        }));
    }
    function addSpacing (parent, width, height) {
        parent.add(new UI.Box({
            backgroundAlpha: 0.0,
            width: width, height: height
        }));
    }
    function addLabel (parent, text) {
        return parent.add(new UI.Label({
            text: text,
            width: 200,
            height: 20
        }));
    }
    function addSlider (parent, label, min, max, getValue, onValueChanged) {
        try {
            var layout = parent.add(new UI.WidgetStack({ dir: "+x" }));
            var textLabel = layout.add(new UI.Label({
                text: label,
                width: 130,
                height: 20
            }));
            var valueLabel = layout.add(new UI.Label({
                text: "" + (+getValue().toFixed(1)),
                width: 60,
                height: 20
            }));
            var slider = layout.add(new UI.Slider({
                value: getValue(), minValue: min, maxValue: max,
                width: 300, height: 20,
                slider: {
                    width: 30,
                    height: 18
                },
                onValueChanged: function (value) {
                    valueLabel.setText("" + (+value.toFixed(1)));
                    onValueChanged(value, slider);
                    UI.updateLayout();
                }
            }));
            return slider;
        } catch (e) {
            logMessage("" + e, COLORS.RED);
            logMessage("parent: " + parent, COLORS.RED);
            logMessage("label:  " + label, COLORS.RED);
            logMessage("min:    " + min, COLORS.RED);
            logMessage("max:    " + max, COLORS.RED);
            logMessage("getValue: " + getValue, COLORS.RED);
            logMessage("onValueChanged: " + onValueChanged, COLORS.RED);
            throw e;
        }
    }
    function addButton (parent, label, onClicked) {
        var button = parent.add(new UI.Box({
            text: label,
            width: 160,
            height: 26,
            leftMargin: 8,
            topMargin: 3
        }));
        button.addAction('onClick', onClicked);
        return button;
    }
    function moveToBottomLeftScreenCorner (widget) {
        var border = 5;
        var pos = {
            x: border,
            y: Controller.getViewportDimensions().y - widget.getHeight() - border
        };
        if (widget.position.x != pos.x || widget.position.y != pos.y) {
            widget.setPosition(pos.x, pos.y);
            UI.updateLayout();
        }
    }
    var _export = this;

    /// Setup the UI. Creates a bunch of sliders for setting the platform radius, density, and entity color / shape properties.
    /// The entityCount slider is readonly.
    function _setupUI (platform) {
        var layoutContainer = makePanel("+y", { visible: false });
        // layoutContainer.setPosition(10, 280);
        // makeDraggable(layoutContainer);
        _export.onScreenResize = function () {
            moveToBottomLeftScreenCorner(layoutContainer);
        }
        var topSection = layoutContainer.add(makePanel("+x")); addSpacing(layoutContainer, 1, 5);
        var btmSection = layoutContainer.add(makePanel("+x"));

        var controls = topSection.add(makePanel("+y")); addSpacing(topSection, 20, 1);
        var buttons  = topSection.add(makePanel("+y")); addSpacing(topSection, 20, 1);

        var colorControls = btmSection.add(makePanel("+y")); addSpacing(btmSection, 20, 1);
        var shapeControls = btmSection.add(makePanel("+y")); addSpacing(btmSection, 20, 1);

        // Top controls
        addLabel(controls, "Platform (platform.js)");
        controls.radiusSlider = addSlider(controls, "radius", PLATFORM_RADIUS_RANGE[0], PLATFORM_RADIUS_RANGE[1], function () { return platform.getRadius() },
            function (value) { 
                platform.setRadiusOnNextUpdate(value);
                controls.entityCountSlider.setValue(platform.getEntityCountWithRadius(value));
            });
        addSpacing(controls, 1, 2);
        controls.densitySlider = addSlider(controls, "entity density", PLATFORM_DENSITY_RANGE[0], PLATFORM_DENSITY_RANGE[1], function () { return platform.getEntityDensity() }, 
            function (value) {
                platform.setDensityOnNextUpdate(value);
                controls.entityCountSlider.setValue(platform.getEntityCountWithDensity(value));
            });
        addSpacing(controls, 1, 2);

        var minEntities = Math.PI * PLATFORM_RADIUS_RANGE[0] * PLATFORM_RADIUS_RANGE[0] * PLATFORM_DENSITY_RANGE[0];
        var maxEntities = Math.PI * PLATFORM_RADIUS_RANGE[1] * PLATFORM_RADIUS_RANGE[1] * PLATFORM_DENSITY_RANGE[1];
        controls.entityCountSlider = addSlider(controls, "entity count", minEntities, maxEntities, function () { return platform.getEntityCount() },
            function (value) {});
        controls.entityCountSlider.actions = {}; // hack: make this slider readonly (clears all attached actions)
        controls.entityCountSlider.slider.actions = {};

        // Buttons
        addSpacing(buttons, 1, 22);
        addButton(buttons, 'rebuild', function () {
            platform.updateHeight(MyAvatar.position.y - AVATAR_HEIGHT_OFFSET);
        });
        addSpacing(buttons, 1, 2);
        addButton(buttons, 'toggle entity type', function () {
            platform.toggleBoxType();
        });
        
        // Bottom controls

        // Iterate over controls (making sliders) for the RNG shape / dimensions model
        platform.randomizer.shapeModel.setupUI(function (name, value, min, max, setValue) {
            // logMessage("platform.randomizer.shapeModel." + name + " = " + value);
            var internal = {
                avg: (value[0] + value[1]) * 0.5,
                range: Math.abs(value[0] - value[1])
            };
            // logMessage(JSON.stringify(internal), COLORS.GREEN);
            addSlider(shapeControls, name + ' avg', min, max, function () { return internal.avg; }, function (value) {
                internal.avg = value;
                setValue([ internal.avg - internal.range * 0.5, internal.avg + internal.range * 0.5 ]);
                platform.updateEntityAttribs();
            });
            addSpacing(shapeControls, 1, 2);
            addSlider(shapeControls, name + ' range', min, max, function () { return internal.range }, function (value) {
                internal.range = value;
                setValue([ internal.avg - internal.range * 0.5, internal.avg + internal.range * 0.5 ]);
                platform.updateEntityAttribs();
            });
            addSpacing(shapeControls, 1, 2);
        });
        // Do the same for the color model
        platform.randomizer.colorModel.setupUI(function (name, value, min, max, setValue) {
            // logMessage("platform.randomizer.colorModel." + name + " = " + value);
            addSlider(colorControls, name, min, max, function () { return value; }, function (value) {
                setValue(value);
                platform.updateEntityAttribs();
            });
            addSpacing(colorControls, 1, 2);
        });
        
        moveToBottomLeftScreenCorner(layoutContainer);
        layoutContainer.setVisible(true);
    }
    this.setupUI = function (platform) {
        if (CATCH_SETUP_ERRORS) {
            try {
                _setupUI(platform);
            } catch (e) {
                logMessage("Error setting up ui: " + e, COLORS.RED);
            }
        } else {
            _setupUI(platform);
        }
    }
})();

// Error handling w/ explicit try / catch blocks. Good for catching unexpected errors with the onscreen debugLog 
// (if it's enabled); bad for detailed debugging since you lose the file and line num even if the error gets rethrown.

// Catch errors from init
var CATCH_INIT_ERRORS = true;

// Catch errors from everything (technically, Script and Controller signals that runs platform / ui code)
var CATCH_ERRORS_FROM_EVENT_UPDATES = false;

// Setup everything
(function () {
    var doLater = null;
    if (CATCH_ERRORS_FROM_EVENT_UPDATES) {
        // Decorates a function w/ explicit error catching + printing to the debug log.
        function catchErrors (fcn) {
            return function () {
                try {
                    fcn.apply(this, arguments);
                } catch (e) {
                    logMessage('' + e, COLORS.RED);
                    logMessage("while calling " + fcn);
                    logMessage("Called by: " + arguments.callee.caller);
                }
            }
        }
        // We need to do this after the functions are registered...
        doLater = function () {
            // Intercept errors from functions called by Script.update and Script.ScriptEnding.
            [ 'teardown', 'startup', 'update', 'initPlatform', 'setupUI' ].forEach(function (fcn) {
                this[fcn] = catchErrors(this[fcn]);
            });
        };
        // These need to be wrapped first though:

        // Intercept errors from UI functions called by Controller.****Event.
        [ 'handleMousePress', 'handleMouseMove', 'handleMouseRelease' ].forEach(function (fcn) {
            UI[fcn] = catchErrors(UI[fcn]);
        });
    }

    function getTargetPlatformPosition () {
        var pos = MyAvatar.position;
        pos.y -= AVATAR_HEIGHT_OFFSET;
        return pos;
    }

    // Program state
    var platform = this.platform = null;
    var lastHeight = null;

    // Init
    this.initPlatform = function () {
        platform = new DynamicPlatform(NUM_PLATFORM_ENTITIES, getTargetPlatformPosition(), RADIUS);
        lastHeight = getTargetPlatformPosition().y;
    }

    // Handle relative screen positioning (UI)
    var lastDimensions = Controller.getViewportDimensions();
    function checkScreenDimensions () {
        var dimensions = Controller.getViewportDimensions();
        if (dimensions.x != lastDimensions.x || dimensions.y != lastDimensions.y) {
            onScreenResize(dimensions.x, dimensions.y);
        }
        lastDimensions = dimensions;
    }

    // Update
    this.update = function (dt) {
        checkScreenDimensions();
        var pos = getTargetPlatformPosition();
        platform.update(dt, getTargetPlatformPosition(), platform.getRadius());
    }

    // Teardown
    this.teardown = function () {
        try {
            platform.destroy();
            UI.teardown();

            Controller.mousePressEvent.disconnect(UI.handleMousePress);
            Controller.mouseMoveEvent.disconnect(UI.handleMouseMove);
            Controller.mouseReleaseEvent.disconnect(UI.handleMouseRelease);
        } catch (e) {
            logMessage("" + e, COLORS.RED);
        }
    }

    if (doLater) {
        doLater();
    }

    // Delays startup until / if entities can be spawned.
    this.startup = function (dt) {
        if (Entities.canAdjustLocks() && Entities.canRez()) {
            Script.update.disconnect(this.startup);

            function init () {
                logMessage("initializing...");
                
                this.initPlatform();

                Script.update.connect(this.update);
                Script.scriptEnding.connect(this.teardown);
    
                this.setupUI(platform);
    
                logMessage("finished initializing.", COLORS.GREEN);
            }
            if (CATCH_INIT_ERRORS) {
                try {
                    init();
                } catch (error) {
                    logMessage("" + error, COLORS.RED);
                }
            } else {
                init();
            }

            Controller.mousePressEvent.connect(UI.handleMousePress);
            Controller.mouseMoveEvent.connect(UI.handleMouseMove);
            Controller.mouseReleaseEvent.connect(UI.handleMouseRelease);
        } else {
            if (!startup.printedWarnMsg) {
                startup.timer = startup.timer || startup.ENTITY_SERVER_WAIT_TIME;
                if ((startup.timer -= dt) < 0.0) {
                    logMessage("Waiting for entity server");
                    startup.printedWarnMsg = true;
                }

            }
        }
    }
    startup.ENTITY_SERVER_WAIT_TIME = 0.2;   // print "waiting for entity server" if more than this time has elapsed in startup()

    Script.update.connect(this.startup);
})();

})();
