//
//    planky.js
//    examples
//
//    Created by Thijs Wenker on 5/2/14.
//    Copyright 2015 High Fidelity, Inc.
//
//    Pull blocks off the bottom and put them on the top using the grab.js script.
//
//    Distributed under the Apache License, Version 2.0.
//    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
HIFI_PUBLIC_BUCKET = 'http://s3.amazonaws.com/hifi-public/';

Script.include('/~/system/libraries/toolBars.js');

const DEFAULT_NUM_LAYERS = 16;
const DEFAULT_BASE_DIMENSION = { x: 7, y: 2, z: 7 };
const DEFAULT_BLOCKS_PER_LAYER = 3;
const DEFAULT_BLOCK_SIZE = {x: 0.2, y: 0.1, z: 0.8};
const DEFAULT_BLOCK_SPACING = DEFAULT_BLOCK_SIZE.x / DEFAULT_BLOCKS_PER_LAYER;
// BLOCK_HEIGHT_VARIATION removes a random percentages of the default block height per block. (for example 0.001 %)
const DEFAULT_BLOCK_HEIGHT_VARIATION = 0.001;
const DEFAULT_GRAVITY = {x: 0, y: -2.8, z: 0};
const DEFAULT_DENSITY = 2000;
const DEFAULT_DAMPING_FACTOR = 0.98;
const DEFAULT_ANGULAR_DAMPING_FACTOR = 0.8;
const DEFAULT_FRICTION = 0.99;
const DEFAULT_RESTITUTION = 0.0;
const DEFAULT_SPAWN_DISTANCE = 3;
const DEFAULT_BLOCK_YAW_OFFSET = 45;

const PLANKY_LIFETIME = 3600; // 1 hour (3600 seconds)

var editMode = false;

const BUTTON_DIMENSIONS = {width: 49, height: 49};
const MAXIMUM_PERCENTAGE = 100.0;
const NO_ANGLE = 0;
const RIGHT_ANGLE = 90;

var windowWidth = Window.innerWidth;
var size;
var pieces = [];
var ground = false;
var layerRotated = false;
var button;
var cogButton;
var toolBar;

SettingsWindow = function() {
    var _this = this;
    this.plankyStack = null;
    this.webWindow = null;
    this.init = function(plankyStack) {
        _this.webWindow = new OverlayWebWindow({
            title: 'Planky',
            source: Script.resolvePath('../../html/plankySettings.html'),
            toolWindow: true
        });
        _this.webWindow.setVisible(false);
        _this.webWindow.webEventReceived.connect(_this.onWebEventReceived);
        _this.plankyStack = plankyStack;
    };
    this.sendData = function(data) {
        _this.webWindow.emitScriptEvent(JSON.stringify(data));
    };
    this.onWebEventReceived = function(data) {
        data = JSON.parse(data);
        switch (data.action) {
            case 'loaded':
                _this.sendData({action: 'load', options: _this.plankyStack.options.getJSON()})
                break;
            case 'value-change':
                _this.plankyStack.onValueChanged(data.option, data.value);
                break;
            case 'factory-reset':
                _this.plankyStack.options.factoryReset();
                _this.sendData({action: 'load', options: _this.plankyStack.options.getJSON()})
                break;
            case 'save-default':
                _this.plankyStack.options.save();
                break;
            case 'cleanup':
                _this.plankyStack.deRez();
                break;
            default:
                Window.alert('[planky] unknown action ' + data.action);
        }
    };
};

PlankyOptions = function() {
    var _this = this;
    this.factoryReset = function() {
        _this.setDefaults();
        Settings.setValue('plankyOptions', '');
    };
    this.save = function() {
        Settings.setValue('plankyOptions', JSON.stringify(_this.getJSON()));
    };
    this.load = function() {
        _this.setDefaults();
        var plankyOptions = Settings.getValue('plankyOptions')
        if (plankyOptions === null || plankyOptions === '') {
            return;
        }
        var options = JSON.parse(plankyOptions);
        for (option in options) {
            _this[option] = options[option];
        }
    };
    this.getJSON = function() {
        return {
            numLayers: _this.numLayers,
            baseDimension: _this.baseDimension,
            blocksPerLayer: _this.blocksPerLayer,
            blockSize: _this.blockSize,
            blockSpacing: _this.blockSpacing,
            blockHeightVariation: _this.blockHeightVariation,
            gravity: _this.gravity,
            density: _this.density,
            dampingFactor: _this.dampingFactor,
            angularDampingFactor: _this.angularDampingFactor,
            friction: _this.friction,
            restitution: _this.restitution,
            spawnDistance: _this.spawnDistance,
            blockYawOffset: _this.blockYawOffset,
        };
    }
    this.setDefaults = function() {
        _this.numLayers = DEFAULT_NUM_LAYERS;
        _this.baseDimension = DEFAULT_BASE_DIMENSION;
        _this.blocksPerLayer = DEFAULT_BLOCKS_PER_LAYER;
        _this.blockSize = DEFAULT_BLOCK_SIZE;
        _this.blockSpacing = DEFAULT_BLOCK_SPACING;
        _this.blockHeightVariation = DEFAULT_BLOCK_HEIGHT_VARIATION;
        _this.gravity = DEFAULT_GRAVITY;
        _this.density = DEFAULT_DENSITY;
        _this.dampingFactor = DEFAULT_DAMPING_FACTOR;
        _this.angularDampingFactor = DEFAULT_ANGULAR_DAMPING_FACTOR;
        _this.friction = DEFAULT_FRICTION;
        _this.restitution = DEFAULT_RESTITUTION;
        _this.spawnDistance = DEFAULT_SPAWN_DISTANCE;
        _this.blockYawOffset = DEFAULT_BLOCK_YAW_OFFSET;
    };
    this.load();
};

// The PlankyStack exists out of rows and layers
PlankyStack = function() {
    var _this = this;
    this.planks = [];
    this.ground = false;
    this.editLines = [];
    this.options = new PlankyOptions();

    this.deRez = function() {
        _this.planks.forEach(function(plank) {
            Entities.deleteEntity(plank.entity);
        });
        _this.planks = [];
        if (_this.ground) {
            Entities.deleteEntity(_this.ground);
        }
        _this.editLines.forEach(function(line) {
            Entities.deleteEntity(line);
        })
        _this.editLines = [];
        if (_this.centerLine) {
            Entities.deleteEntity(_this.centerLine);
        }
        _this.ground = false;
        _this.centerLine = false;
    };

    this.rez = function() {
        if (_this.planks.length > 0) {
            _this.deRez();
        }
        _this.baseRotation = Quat.fromPitchYawRollDegrees(0.0, MyAvatar.bodyYaw, 0.0);  
        var basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(_this.options.spawnDistance, Quat.getFront(_this.baseRotation)));
        basePosition.y = grabLowestJointY();
        _this.basePosition = basePosition;
        _this.refresh();
    };

    //private function
    var refreshGround = function() {
        if (!_this.ground) {
            _this.ground = Entities.addEntity({
                type: 'Model',
                modelURL: HIFI_PUBLIC_BUCKET + 'eric/models/woodFloor.fbx',
                dimensions: _this.options.baseDimension,
                position: Vec3.sum(_this.basePosition, {y: -(_this.options.baseDimension.y / 2)}),
                rotation: _this.baseRotation,
                shapeType: 'box',
                lifetime: PLANKY_LIFETIME
            });
            return;
        }
        // move ground to rez position/rotation
        Entities.editEntity(_this.ground, {dimensions: _this.options.baseDimension, position: Vec3.sum(_this.basePosition, {y: -(_this.options.baseDimension.y / 2)}), rotation: _this.baseRotation});
    };

    var refreshLines = function() {
        if (_this.editLines.length === 0) {
            _this.editLines.push(Entities.addEntity({
                type: 'Line',
                dimensions: {x: 5, y: 21, z: 5},
                position: Vec3.sum(_this.basePosition, {y: -(_this.options.baseDimension.y / 2)}),
                lineWidth: 7,
                color: {red: 20, green: 20, blue: 20},
                linePoints: [{x: 0, y: 0, z: 0}, {x: 0, y: 10, z: 0}],
                visible: editMode
            }));
            return;
        }
        _this.editLines.forEach(function(line) {
            Entities.editEntity(line, {visible: editMode});
        })        
    };

    var trimDimension = function(dimension, maxIndex) {
        var removingPlanks = [];
        _this.planks.forEach(function(plank, index, object) {
            if (plank[dimension] > maxIndex) {
                removingPlanks.push(index);
            }
        });
        removingPlanks.reverse();
        for (var i = 0; i < removingPlanks.length; i++) {
            Entities.deleteEntity(_this.planks[removingPlanks[i]].entity);
            _this.planks.splice(removingPlanks[i], 1);
        }
    };

    var createOrUpdate = function(layer, row) {
        var found = false;
        var layerRotated = layer % 2 === 0;
        var layerRotation = Quat.fromPitchYawRollDegrees(0, layerRotated ? NO_ANGLE : RIGHT_ANGLE, 0.0);
        var blockPositionXZ = (row - (_this.options.blocksPerLayer / 2) + 0.5) * (_this.options.blockSpacing + _this.options.blockSize.x);
        var localTransform = Vec3.multiplyQbyV(_this.offsetRot, {
            x: (layerRotated ? blockPositionXZ : 0),
            y: (_this.options.blockSize.y / 2) + (_this.options.blockSize.y * layer),
            z: (layerRotated ? 0 : blockPositionXZ)
        });
        var newProperties = {
            type: 'Model',
            modelURL: HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/block.fbx',
            shapeType: 'box',
            name: 'PlankyBlock' + layer + '-' + row,
            dimensions: Vec3.sum(_this.options.blockSize, {x: 0, y: -((_this.options.blockSize.y * (_this.options.blockHeightVariation / MAXIMUM_PERCENTAGE)) * Math.random()), z: 0}),
            position: Vec3.sum(_this.basePosition, localTransform),
            rotation: Quat.multiply(layerRotation, _this.offsetRot),
            damping: _this.options.dampingFactor,
            restitution: _this.options.restitution,
            friction: _this.options.friction,
            angularDamping: _this.options.angularDampingFactor,
            gravity: _this.options.gravity,
            density: _this.options.density,
            velocity: {x: 0, y: 0, z: 0},
            angularVelocity: Quat.fromPitchYawRollDegrees(0, 0, 0),
            collisionless: true,
            lifetime: PLANKY_LIFETIME
        };
        _this.planks.forEach(function(plank, index, object) {
            if (plank.layer === layer && plank.row === row) {
                Entities.editEntity(plank.entity, newProperties);
                found = true;
                // break loop:
                return false;
            }
        });
        if (!found) {
            _this.planks.push({layer: layer, row: row, entity: Entities.addEntity(newProperties)})
        }
    };

    this.onValueChanged = function(option, value) {
        _this.options[option] = value;
        if (['numLayers', 'blocksPerLayer', 'blockSize', 'blockSpacing', 'blockHeightVariation'].indexOf(option) !== -1) {
            _this.refresh();
        }
    };

    this.refresh = function() {
        refreshGround();
        refreshLines();
        trimDimension('layer', _this.options.numLayers - 1);
        trimDimension('row', _this.options.blocksPerLayer - 1);
        _this.offsetRot = Quat.multiply(_this.baseRotation, Quat.fromPitchYawRollDegrees(0.0, _this.options.blockYawOffset, 0.0));
        for (var layer = 0; layer < _this.options.numLayers; layer++) {
            for (var row = 0; row < _this.options.blocksPerLayer; row++) {
                createOrUpdate(layer, row);
            }
        }
        if (!editMode) {
            _this.planks.forEach(function(plank, index, object) {
                Entities.editEntity(plank.entity, {collisionless: false, dynamic: true});
            });
        }
    };

    this.isFound = function() {
        //TODO: identify entities here until one is found
        return _this.planks.length > 0;
    };
};

var settingsWindow = new SettingsWindow();
var plankyStack = new PlankyStack();
settingsWindow.init(plankyStack);

// This function is used to get the ideal y-location for a floor
function grabLowestJointY() {
    var jointNames = MyAvatar.getJointNames();
    var floorY = MyAvatar.position.y;
    for (var jointName in jointNames) {
        if (MyAvatar.getJointPosition(jointNames[jointName]).y < floorY) {
            floorY = MyAvatar.getJointPosition(jointNames[jointName]).y;
        }
    }
    return floorY;
}

toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL, "highfidelity.games.planky", function (windowDimensions, toolbar) {
    return {
        x: windowDimensions.x - (toolbar.width * 1.1),
        y: toolbar.height / 2
    };
});

button = toolBar.addTool({
    width: BUTTON_DIMENSIONS.width,
    height: BUTTON_DIMENSIONS.height,
    imageURL: HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/planky_button.svg',
    alpha: 0.8,
    visible: true
});

cogButton = toolBar.addTool({
    width: BUTTON_DIMENSIONS.width,
    height: BUTTON_DIMENSIONS.height,
    imageURL: HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/cog.svg',
    subImage: { x: 0, y: BUTTON_DIMENSIONS.height, width: BUTTON_DIMENSIONS.width, height: BUTTON_DIMENSIONS.height },
    alpha: 0.8,
    visible: true
}, true, false);

Controller.mousePressEvent.connect(function(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    if (toolBar.clicked(clickedOverlay) === button) {
        if (!plankyStack.isFound()) {
            plankyStack.rez();
            return;
        }
        plankyStack.refresh();
    } else if (toolBar.clicked(clickedOverlay) === cogButton) {
        editMode = !editMode;
        toolBar.selectTool(cogButton, editMode);
        settingsWindow.webWindow.setVisible(editMode);
        if(plankyStack.planks.length) {
            plankyStack.refresh();
        }
    }
});

Script.update.connect(function() {
    if (windowWidth !== Window.innerWidth) {
        windowWidth = Window.innerWidth;
        Overlays.editOverlay(button, {x: getButtonPosX()});
        Overlays.editOverlay(cogButton, {x: getCogButtonPosX()});
    }
})

Script.scriptEnding.connect(function() {
    toolBar.cleanup();
    if (ground) {
        Entities.deleteEntity(ground);
    }
    plankyStack.deRez();
});
