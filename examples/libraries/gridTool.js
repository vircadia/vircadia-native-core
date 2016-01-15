var GRID_CONTROLS_HTML_URL = Script.resolvePath('../html/gridControls.html');

Grid = function(opts) {
    var that = {};

    var colors = [
        { red: 0, green: 0, blue: 0 },
        { red: 255, green: 255, blue: 255 },
        { red: 255, green: 0, blue: 0 },
        { red: 0, green: 255, blue: 0 },
        { red: 0, green: 0, blue: 255 },
    ];
    var colorIndex = 0;
    var gridAlpha = 0.6;
    var origin = { x: 0, y: 0, z: 0 };
    var majorGridEvery = 5;
    var minorGridWidth = 0.2;
    var halfSize = 40;
    var yOffset = 0.001;

    var worldSize = 16384;

    var snapToGrid = false;

    var gridOverlay = Overlays.addOverlay("grid", {
        position: { x: 0 , y: 0, z: 0 },
        visible: true,
        color: colors[0],
        alpha: gridAlpha,
        rotation: Quat.fromPitchYawRollDegrees(90, 0, 0),
        minorGridWidth: 0.1,
        majorGridEvery: 2,
    });

    that.visible = false;
    that.enabled = false;

    that.getOrigin = function() {
        return origin;
    }

    that.getMinorIncrement = function() { return minorGridWidth; };
    that.getMajorIncrement = function() { return minorGridWidth * majorGridEvery; };

    that.getMinorGridWidth = function() { return minorGridWidth; };
    that.setMinorGridWidth = function(value) {
        minorGridWidth = value;
        updateGrid();
    };

    that.getMajorGridEvery = function() { return majorGridEvery; };
    that.setMajorGridEvery = function(value) {
        majorGridEvery = value;
        updateGrid();
    };

    that.getColorIndex = function() { return colorIndex; };
    that.setColorIndex = function(value) {
        colorIndex = value;
        updateGrid();
    };

    that.getSnapToGrid = function() { return snapToGrid; };
    that.setSnapToGrid = function(value) {
        snapToGrid = value;
        that.emitUpdate();
    };

    that.setEnabled = function(enabled) {
        that.enabled = enabled;
        updateGrid();
    }

    that.getVisible = function() { return that.visible; };
    that.setVisible = function(visible, noUpdate) {
        that.visible = visible;
        updateGrid();

        if (!noUpdate) {
            that.emitUpdate();
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

        var spacing = majorOnly ? (minorGridWidth * majorGridEvery) : minorGridWidth;

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

        var spacing = majorOnly ? (minorGridWidth * majorGridEvery) : minorGridWidth;

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
                minorGridWidth: minorGridWidth,
                majorGridEvery: majorGridEvery,
                gridSize: halfSize,
                visible: that.visible,
                snapToGrid: snapToGrid,
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

        if (data.minorGridWidth) {
            minorGridWidth = data.minorGridWidth;
        }

        if (data.majorGridEvery) {
            majorGridEvery = data.majorGridEvery;
        }

        if (data.colorIndex !== undefined) {
            colorIndex = data.colorIndex;
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
            minorGridWidth: minorGridWidth,
            majorGridEvery: majorGridEvery,
            color: colors[colorIndex],
            alpha: gridAlpha,
        });

        that.emitUpdate();
    }

    function cleanup() {
        Overlays.deleteOverlay(gridOverlay);
    }

    that.addListener = function(callback) {
        that.onUpdate = callback;
    }

    Script.scriptEnding.connect(cleanup);
    updateGrid();

    that.onUpdate = null;

    return that;
};

GridTool = function(opts) {
    var that = {};

    var horizontalGrid = opts.horizontalGrid;
    var verticalGrid = opts.verticalGrid;
    var listeners = [];

    var url = GRID_CONTROLS_HTML_URL;
    var webView = new OverlayWebWindow({
        title: 'Grid',  source: url,  toolWindow: true
    });

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
                var position = MyAvatar.getJointPosition("LeftFoot");
                if (position.x == 0 && position.y == 0 && position.z == 0) {
                    position = MyAvatar.position;
                }
                horizontalGrid.setPosition(position);
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
