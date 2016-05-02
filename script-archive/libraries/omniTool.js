//
//  Created by Bradley Austin Davis on 2015/09/01
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("constants.js");
Script.include("utils.js");
Script.include("highlighter.js");
Script.include("omniTool/models/modelBase.js");
Script.include("omniTool/models/wand.js");
Script.include("omniTool/models/invisibleWand.js");

OmniToolModules = {};
OmniToolModuleType = null;
LOG_DEBUG = 1;

OmniTool = function(left) {
    this.OMNI_KEY = "OmniTool";
    this.MAX_FRAMERATE = 60;
    this.UPDATE_INTERVAL = 1.0 / this.MAX_FRAMERATE
    this.left = left;
    this.triggered = false;
    var actions = Controller.Actions;
    var standard = Controller.Standard;
    this.palmControl = left ? actions.LeftHand : actions.RightHand;
    logDebug("Init OmniTool " + (left ? "left" : "right"));
    this.highlighter = new Highlighter();
    this.ignoreEntities = {};
    this.nearestOmniEntity = {
        id: null,
        
        inside: false,
        position: null,
        distance: Infinity,
        radius: 0,
        omniProperties: {},
        boundingBox: null,
    };
    
    this.activeOmniEntityId = null;
    this.lastUpdateInterval = 0;
    this.active = false;
    this.module = null;
    this.moduleEntityId = null;
    this.lastScanPosition = ZERO_VECTOR;
    this.showWand(false);

    // Connect to desired events
    var that = this;

    Script.update.connect(function(deltaTime) {
        that.lastUpdateInterval += deltaTime;
        if (that.lastUpdateInterval >= that.UPDATE_INTERVAL) {
            that.onUpdate(that.lastUpdateInterval);
            that.lastUpdateInterval = 0;
        }
    });

    Script.scriptEnding.connect(function() {
        that.onCleanup();
    });

    this.mapping = Controller.newMapping();
    this.mapping.from(left ? standard.LeftPrimaryThumb : standard.RightPrimaryThumb).to(function(value){
        that.onUpdateTrigger(value);
    })
    this.mapping.enable();
}

OmniTool.prototype.showWand = function(show) {
    if (this.model && this.model.onCleanup) {
        this.model.onCleanup();
    }
    logDebug("Showing wand: " + show);
    if (show) {
        this.model = new Wand();
        this.model.setLength(0.4);
        this.model.setVisible(true);
    } else {
        this.model = new InvisibleWand();
        this.model.setLength(0.1);
        this.model.setVisible(true);
    }
}

OmniTool.prototype.onCleanup = function(action) {
    this.mapping.disable();
    this.unloadModule();
}


OmniTool.prototype.onUpdateTrigger = function (value) {
    //logDebug("Trigger update value " + value);
    var triggered = value != 0;
    if (triggered != this.triggered) {
        this.triggered = triggered;
        if (this.triggered) {
            this.onClick();
        } else {
            this.onRelease();
        }
    }
}

OmniTool.prototype.getOmniToolData = function(entityId) {
    return getEntityCustomData(this.OMNI_KEY, entityId, null);
}

OmniTool.prototype.setOmniToolData = function(entityId, data) {
    setEntityCustomData(this.OMNI_KEY, entityId, data);
}

OmniTool.prototype.updateOmniToolData = function(entityId, data) {
    var currentData = this.getOmniToolData(entityId) || {};
    for (var key in data) {
        currentData[key] = data[key];
    }
    setEntityCustomData(this.OMNI_KEY, entityId, currentData);
}

OmniTool.prototype.setActive = function(active) {
    if (active === this.active) {
        return;
    }
    logDebug("OmniTool " + this.left  + " changing active state: " + active);
    this.active = active;
    this.model.setVisible(this.active);
    if (this.module && this.module.onActiveChanged) {
        this.module.onActiveChanged(this.side);
    }
}


OmniTool.prototype.onUpdate = function(deltaTime) {
    // FIXME this returns data if either the left or right controller is not on the base
    this.pose = Controller.getPoseValue(this.palmControl);
    this.position = this.left ? MyAvatar.leftHandTipPosition : MyAvatar.rightHandTipPosition;
    // When on the base, hydras report a position of 0
    this.setActive(Vec3.length(this.position) > 0.001);
    if (!this.active) {
        return;
    }
    
    if (this.model) {
        // Update the wand
        var rawRotation = this.pose.rotation;
        this.rotation = Quat.multiply(MyAvatar.orientation, rawRotation);
        this.model.setTransform({
            rotation: this.rotation,
            position: this.position,
        });
        
        if (this.model.onUpdate) {
            this.model.onUpdate(deltaTime);
        }
    }

    
    this.scan();
    
    if (this.module && this.module.onUpdate) {
        this.module.onUpdate(deltaTime);
    }
}

OmniTool.prototype.onClick = function() {
    // First check to see if the user is switching to a new omni module
    if (this.nearestOmniEntity.inside && this.nearestOmniEntity.omniProperties.script) {

        // If this is already the active entity, turn it off
        // FIXME add a flag to allow omni modules to cause this entity to be
        // ignored in order to support items that will be picked up.
        if (this.moduleEntityId && this.moduleEntityId == this.nearestOmniEntity.id) {
            this.showWand(false);
            this.unloadModule();
            this.highlighter.setColor("White");
            return;
        }

        this.showWand(true);
        this.highlighter.setColor("Red");
        this.activateNewOmniModule();
        return;
    }
    
    // Next check if there is an active module and if so propagate the click
    // FIXME how to I switch to a new module?
    if (this.module && this.module.onClick) {
        this.module.onClick();
        return;
    }
}

OmniTool.prototype.onRelease = function() {
    if (this.module && this.module.onRelease) {
        this.module.onRelease();
        return;
    }
}

// FIXME resturn a structure of all nearby entities to distances
OmniTool.prototype.findNearestOmniEntity = function(maxDistance, selector)  {
    if (!maxDistance) {
        maxDistance = 2.0;
    }
    var resultDistance = Infinity;
    var resultId = null;
    var resultProperties = null;
    var resultOmniData = null;
    var ids = Entities.findEntities(this.model.tipPosition, maxDistance);
    for (var i in ids) {
        var entityId = ids[i];
        if (this.ignoreEntities[entityId]) {
            continue;
        }
        var omniData = this.getOmniToolData(entityId);
        if (!omniData) {
            // FIXME find a place to flush this information
            this.ignoreEntities[entityId] = true;
            continue;
        }
        
        // Let searchers query specifically
        if (selector && !selector(entityId, omniData)) {
            continue;
        }
        
        var properties = Entities.getEntityProperties(entityId);
        var distance = Vec3.distance(this.model.tipPosition, properties.position);
        if (distance < resultDistance) {
            resultDistance = distance;
            resultId = entityId;
        }
    }

    return resultId;
}

OmniTool.prototype.getPosition = function() {
    return this.model.tipPosition;
}

OmniTool.prototype.onEnterNearestOmniEntity = function() {
    this.nearestOmniEntity.inside = true;
    this.highlighter.highlight(this.nearestOmniEntity.id);
    if (this.moduleEntityId && this.moduleEntityId == this.nearestOmniEntity.id) {
        this.highlighter.setColor("Red");
    } else {
        this.highlighter.setColor("White");
    }
    logDebug("On enter omniEntity " + this.nearestOmniEntity.id);
}

OmniTool.prototype.onLeaveNearestOmniEntity = function() {
    this.nearestOmniEntity.inside = false;
    this.highlighter.highlight(null);
    logDebug("On leave omniEntity " + this.nearestOmniEntity.id);
}

OmniTool.prototype.setNearestOmniEntity = function(entityId) {
    if (entityId && entityId !== this.nearestOmniEntity.id) {
        if (this.nearestOmniEntity.id && this.nearestOmniEntity.inside) {
            this.onLeaveNearestOmniEntity();
        }
        this.nearestOmniEntity.id = entityId;
        this.nearestOmniEntity.omniProperties = this.getOmniToolData(entityId);
        var properties = Entities.getEntityProperties(entityId);
        this.nearestOmniEntity.position = properties.position;
        // FIXME use a real bounding box, not a sphere
        var bbox = properties.boundingBox;
        this.nearestOmniEntity.radius = Vec3.length(Vec3.subtract(bbox.center, bbox.brn));
        this.highlighter.setRotation(properties.rotation);
        this.highlighter.setSize(Vec3.multiply(1.05, bbox.dimensions));
    }
}

OmniTool.prototype.scan = function() {
    var scanDistance = Vec3.distance(this.model.tipPosition, this.lastScanPosition);
    
    if (scanDistance < 0.005) {
        return;
    }
    
    this.lastScanPosition = this.model.tipPosition;
    
    this.setNearestOmniEntity(this.findNearestOmniEntity());
    if (this.nearestOmniEntity.id) {
        var distance = Vec3.distance(this.model.tipPosition, this.nearestOmniEntity.position);
        // track distance on a half centimeter basis
        if (Math.abs(this.nearestOmniEntity.distance - distance) > 0.005) {
            this.nearestOmniEntity.distance = distance;
            if (!this.nearestOmniEntity.inside && distance < this.nearestOmniEntity.radius) {
                this.onEnterNearestOmniEntity();
            }

            if (this.nearestOmniEntity.inside && distance > this.nearestOmniEntity.radius + 0.01) {
                this.onLeaveNearestOmniEntity();
            }
        }
    }
}

OmniTool.prototype.unloadModule = function() {
    logDebug("Unloading omniTool module")
    if (this.module && this.module.onUnload) {
        this.module.onUnload();
    }
    this.module = null;
    this.moduleEntityId = null;
}

OmniTool.prototype.activateNewOmniModule = function() {
    // Support the ability for scripts to just run without replacing the current module
    var script = this.nearestOmniEntity.omniProperties.script;
    if (script.indexOf("/") < 0) {
        script = "omniTool/modules/" + script;
    }

    // Reset the tool type
    OmniToolModuleType = null;
    logDebug("Including script path: " + script);
    try {
        Script.include(script);
    } catch(err) {
        logWarn("Failed to include script: " + script + "\n" + err);
        return;
    }

    // If we're building a new module, unload the old one
    if (OmniToolModuleType) {
        logDebug("New OmniToolModule: " + OmniToolModuleType);
        this.unloadModule();

        try {
            this.module = new OmniToolModules[OmniToolModuleType](this, this.nearestOmniEntity.id);
            this.moduleEntityId = this.nearestOmniEntity.id;
            if (this.module.onLoad) {
                this.module.onLoad();
            }
        } catch(err) {
            logWarn("Failed to instantiate new module: " + err);
        }
    }
}

// FIXME find a good way to sync the two omni tools
OMNI_TOOLS = [ new OmniTool(true), new OmniTool(false) ];
