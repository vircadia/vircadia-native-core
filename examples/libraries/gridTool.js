var SETTING_GRID_VISIBLE = 'gridVisible';
var SETTING_GRID_SNAP_TO_GRID = 'gridSnapToGrid';
var SETTING_GRID_MINOR_WIDTH= 'gridMinorWidth';
var SETTING_GRID_MAJOR_EVERY = 'gridMajorEvery';
var SETTING_GRID_COLOR = 'gridColor';

Grid = function(opts) {
    var that = {};

    var color = { red: 100, green: 152, blue: 203 };
    var gridColor = { red: 100, green: 152, blue: 203 };
    var gridAlpha = 1.0;
    var origin = { x: 0, y: 0, z: 0 };
    var majorGridEvery = 5;
    var minorGridSpacing = 0.2;
    var halfSize = 40;
    var yOffset = 0.001;

    var worldSize = 16384;

    var snapToGrid = false;

    var gridOverlay = Overlays.addOverlay("grid", {
        position: { x: 0 , y: 0, z: 0 },
        visible: true,
        color: { red: 0, green: 0, blue: 128 },
        alpha: 1.0,
        rotation: Quat.fromPitchYawRollDegrees(90, 0, 0),
        minorGridSpacing: 0.1,
        majorGridEvery: 2,
    });

    that.getMinorIncrement = function() { return minorGridSpacing; };
    that.getMajorIncrement = function() { return minorGridSpacing * majorGridEvery; };

    that.visible = false;
    that.enabled = false;

    that.getOrigin = function() {
        return origin;
    }

    that.getSnapToGrid = function() { return snapToGrid; };

    that.setEnabled = function(enabled) {
        if (that.enabled != enabled) {
            that.enabled = enabled;

            if (enabled) {
                if (selectionManager.hasSelection()) {
                    that.setPosition(selectionManager.getBottomPosition());
                } else {
                    that.setPosition(MyAvatar.position);
                }
            }

            updateGrid();
        }
    }

    that.setVisible = function(visible, noUpdate) {
        if (visible != that.visible) {
            that.visible = visible;
            updateGrid();

            if (visible) {
                if (selectionManager.hasSelection()) {
                    that.setPosition(selectionManager.getBottomPosition());
                } else {
                    that.setPosition(MyAvatar.position);
                }
            }

            if (!noUpdate) {
                that.emitUpdate();
            }
        }
    }

    that.snapToSurface = function(position, dimensions) {
        if (!snapToGrid) {
            return position;
        }

        if (dimensions === undefined) {
            dimensions = { x: 0, y: 0, z: 0 };
        }

        return {
            x: position.x,
            y: origin.y + (dimensions.y / 2),
            z: position.z
        };
    }

    that.snapToGrid = function(position, majorOnly, dimensions) {
        if (!snapToGrid) {
            return position;
        }

        if (dimensions === undefined) {
            dimensions = { x: 0, y: 0, z: 0 };
        }

        var spacing = majorOnly ? (minorGridSpacing * majorGridEvery) : minorGridSpacing;

        position = Vec3.subtract(position, origin);

        position.x = Math.round(position.x / spacing) * spacing;
        position.y = Math.round(position.y / spacing) * spacing;
        position.z = Math.round(position.z / spacing) * spacing;

        return Vec3.sum(Vec3.sum(position, Vec3.multiply(0.5, dimensions)), origin);
    }

    that.snapToSpacing = function(delta, majorOnly) {
        if (!snapToGrid) {
            return delta;
        }

        var spacing = majorOnly ? (minorGridSpacing * majorGridEvery) : minorGridSpacing;

        var snappedDelta = {
            x: Math.round(delta.x / spacing) * spacing,
            y: Math.round(delta.y / spacing) * spacing,
            z: Math.round(delta.z / spacing) * spacing,
        };

        return snappedDelta;
    }


    that.setPosition = function(newPosition, noUpdate) {
        origin = Vec3.subtract(newPosition, { x: 0, y: yOffset, z: 0 });
        origin.x = 0;
        origin.z = 0;
        updateGrid();

        if (!noUpdate) {
            that.emitUpdate();
        }
    };

    that.emitUpdate = function() {
        if (that.onUpdate) {
            that.onUpdate({
                origin: origin,
                minorGridSpacing: minorGridSpacing,
                majorGridEvery: majorGridEvery,
                gridSize: halfSize,
                visible: that.visible,
                snapToGrid: snapToGrid,
                gridColor: gridColor,
            });
        }
    };

    that.update = function(data) {
        if (data.snapToGrid !== undefined) {
            snapToGrid = data.snapToGrid;
        }

        if (data.origin) {
            var pos = data.origin;
            pos.x = pos.x === undefined ? origin.x : pos.x;
            pos.y = pos.y === undefined ? origin.y : pos.y;
            pos.z = pos.z === undefined ? origin.z : pos.z;
            that.setPosition(pos, true);
        }

        if (data.minorGridSpacing) {
            minorGridSpacing = data.minorGridSpacing;
        }

        if (data.majorGridEvery) {
            majorGridEvery = data.majorGridEvery;
        }

        if (data.gridColor) {
            gridColor = data.gridColor;
        }

        if (data.gridSize) {
            halfSize = data.gridSize;
        }

        if (data.visible !== undefined) {
            that.setVisible(data.visible, true);
        }

        updateGrid();
    }

    function updateGrid() {
        Overlays.editOverlay(gridOverlay, {
            position: { x: origin.y, y: origin.y, z: -origin.y },
            visible: that.visible && that.enabled,
            minorGridSpacing: minorGridSpacing,
            majorGridEvery: majorGridEvery,
                color: gridColor,
                alpha: gridAlpha,
        });

        that.emitUpdate();
    }

    function cleanup() {
        saveSettings();

        Overlays.deleteOverlay(gridOverlay);
    }

    function loadSettings() {
        that.setVisible(Settings.getValue(SETTING_GRID_VISIBLE) == "true", true);
        snapToGrid = Settings.getValue(SETTING_GRID_SNAP_TO_GRID) == "true";
        minorGridSpacing = parseFloat(Settings.getValue(SETTING_GRID_MINOR_WIDTH), 10);
        majorGridEvery = parseInt(Settings.getValue(SETTING_GRID_MAJOR_EVERY), 10);
        try {
            var newColor = JSON.parse(Settings.getValue(SETTING_GRID_COLOR));
            if (newColor.red !== undefined && newColor.green !== undefined && newColor.blue !== undefined) {
                gridColor.red = newColor.red;
                gridColor.green = newColor.green;
                gridColor.blue = newColor.blue;
            }
        } catch (e) {
        }
        updateGrid();
    }

    function saveSettings() {
        Settings.setValue(SETTING_GRID_VISIBLE, that.visible);
        Settings.setValue(SETTING_GRID_SNAP_TO_GRID, snapToGrid);
        Settings.setValue(SETTING_GRID_MINOR_WIDTH, minorGridSpacing);
        Settings.setValue(SETTING_GRID_MAJOR_EVERY, majorGridEvery);
        Settings.setValue(SETTING_GRID_COLOR, JSON.stringify(gridColor));
    }

    that.addListener = function(callback) {
        that.onUpdate = callback;
    }

    Script.scriptEnding.connect(cleanup);

    loadSettings();

    that.onUpdate = null;

    return that;
};

GridTool = function(opts) {
    var that = {};

    var horizontalGrid = opts.horizontalGrid;
    var verticalGrid = opts.verticalGrid;
    var listeners = [];

    var url = Script.resolvePath('html/gridControls.html');
    var webView = new WebWindow('Grid', url, 200, 280);

    horizontalGrid.addListener(function(data) {
        webView.eventBridge.emitScriptEvent(JSON.stringify(data));
        selectionDisplay.updateHandles();
    });

    webView.eventBridge.webEventReceived.connect(function(data) {
        data = JSON.parse(data);
        if (data.type == "init") {
            horizontalGrid.emitUpdate();
        } else if (data.type == "update") {
            horizontalGrid.update(data);
            for (var i = 0; i < listeners.length; i++) {
                listeners[i](data);
            }
        } else if (data.type == "action") {
            var action = data.action;
            if (action == "moveToAvatar") {
                grid.setPosition(MyAvatar.position);
            } else if (action == "moveToSelection") {
                var newPosition = selectionManager.worldPosition;
                newPosition = Vec3.subtract(newPosition, { x: 0, y: selectionManager.worldDimensions.y * 0.5, z: 0 });
                grid.setPosition(newPosition);
            }
        }
    });

    that.addListener = function(callback) {
        listeners.push(callback);
    }

    that.setVisible = function(visible) {
        webView.setVisible(visible);
    }

    return that;
};
