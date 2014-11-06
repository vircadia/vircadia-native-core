Grid = function(opts) {
    var that = {};

    var color = { red: 100, green: 152, blue: 203 };
    var gridColor = { red: 100, green: 152, blue: 203 };
    var gridAlpha = 0.9;
    var origin = { x: 0, y: 0, z: 0 };
    var majorGridEvery = 5;
    var minorGridSpacing = 0.2;
    var halfSize = 40;
    var yOffset = 0.001;

    var worldSize = 16384;

    var minorGridWidth = 0.5;
    var majorGridWidth = 1.5;

    var gridOverlays = [];

    var snapToGrid = true;

    var gridPlane = Overlays.addOverlay("rectangle3d", {
        position: origin,
        color: color,
        size: halfSize * 2 * minorGridSpacing * 1.05,
        alpha: 0.2,
        solid: true,
        visible: false,
        ignoreRayIntersection: true,
    });

    that.getMinorIncrement = function() { return minorGridSpacing; };
    that.getMajorIncrement = function() { return minorGridSpacing * majorGridEvery; };

    that.visible = false;

    that.getOrigin = function() {
        return origin;
    }

    that.getSnapToGrid = function() { return snapToGrid; };

    that.setVisible = function(visible, noUpdate) {
        that.visible = visible;
        updateGrid();
        // for (var i = 0; i < gridOverlays.length; i++) {
        //     Overlays.editOverlay(gridOverlays[i], { visible: visible });
        // }
        // Overlays.editOverlay(gridPlane, { visible: visible });

        if (!noUpdate) {
            that.emitUpdate();
        }
    }

    that.snapToGrid = function(position, majorOnly) {
        if (!snapToGrid) {
            return position;
        }

        var spacing = majorOnly ? (minorGridSpacing * majorGridEvery) : minorGridSpacing;

        position = Vec3.subtract(position, origin);

        position.x = Math.round(position.x / spacing) * spacing;
        position.y = Math.round(position.y / spacing) * spacing;
        position.z = Math.round(position.z / spacing) * spacing;

        return Vec3.sum(position, origin);
    }

    that.setPosition = function(newPosition, noUpdate) {
        origin = Vec3.subtract(newPosition, { x: 0, y: yOffset, z: 0 });
        updateGrid();

        print("updated grid");
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
        print("Got update");
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
        // Delete overlays
        var gridLinesRequired = (halfSize * 2 + 1) * 2;
        if (gridLinesRequired > gridOverlays.length) {
            for (var i = gridOverlays.length; i < gridLinesRequired; i++) {
                gridOverlays.push(Overlays.addOverlay("line3d", {}));
            }
        } else if (gridLinesRequired < gridOverlays.length) {
            var numberToRemove = gridOverlays.length - gridLinesRequired;
            var removed = gridOverlays.splice(gridOverlays.length - numberToRemove, numberToRemove);
            for (var i = 0; i < removed.length; i++) {
                Overlays.deleteOverlay(removed[i]);
            }
        }

        Overlays.editOverlay(gridPlane, {
            position: origin,
            size: halfSize * 2 * minorGridSpacing * 1.05,
        });

        var startX = {
            x: origin.x - (halfSize * minorGridSpacing),
            y: origin.y,
            z: origin.z,
        };
        var endX = {
            x: origin.x + (halfSize * minorGridSpacing),
            y: origin.y,
            z: origin.z,
        };
        var startZ = {
            x: origin.x,
            y: origin.y,
            z: origin.z - (halfSize * minorGridSpacing)
        };
        var endZ = {
            x: origin.x,
            y: origin.y,
            z: origin.z + (halfSize * minorGridSpacing)
        };

        var overlayIdx = 0;
        for (var i = -halfSize; i <= halfSize; i++) {
            // Offset for X-axis aligned grid line
            var offsetX = { x: 0, y: 0, z: i * minorGridSpacing };

            // Offset for Z-axis aligned grid line
            var offsetZ = { x: i * minorGridSpacing, y: 0, z: 0 };

            var position = Vec3.sum(origin, offsetX);
            var size = i % majorGridEvery == 0 ? majorGridWidth : minorGridWidth;

            var gridLineX = gridOverlays[overlayIdx++];
            var gridLineZ = gridOverlays[overlayIdx++];

            Overlays.editOverlay(gridLineX, {
                start: Vec3.sum(startX, offsetX),
                end: Vec3.sum(endX, offsetX),
                lineWidth: size,
                color: gridColor,
                alpha: gridAlpha,
                solid: true,
                visible: that.visible,
                ignoreRayIntersection: true,
            });
            Overlays.editOverlay(gridLineZ, {
                start: Vec3.sum(startZ, offsetZ),
                end: Vec3.sum(endZ, offsetZ),
                lineWidth: size,
                color: gridColor,
                alpha: gridAlpha,
                solid: true,
                visible: that.visible,
                ignoreRayIntersection: true,
            });
        }
    }

    function cleanup() {
        Overlays.deleteOverlay(gridPlane);
        for (var i = 0; i < gridOverlays.length; i++) {
            Overlays.deleteOverlay(gridOverlays[i]);
        }
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

    // var webView = Window.createWebView('http://localhost:8000/gridControls.html', 200, 280);
    var webView = new WebWindow('http://localhost:8000/gridControls.html', 200, 280);

    horizontalGrid.addListener(function(data) {
        webView.eventBridge.emitScriptEvent(JSON.stringify(data));
    });

    webView.eventBridge.webEventReceived.connect(function(data) {
        print('got event: ' + data);
        data = JSON.parse(data);
        if (data.type == "init") {
            horizontalGrid.emitUpdate(); 
        } else if (data.type == "update") {
            horizontalGrid.update(data);
            for (var i = 0; i < listeners.length; i++) {
                listeners[i](data);
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
