Grid = function(opts) {
    var that = {};

    var colors = [
        { red: 102, green: 180, blue: 126 },
        { red: 83, green: 210, blue: 83 },
        { red: 235, green: 173, blue: 0 },
        { red: 210, green: 115, blue: 0 },
        { red: 48, green: 116, blue: 119 },
    ];
    var colorIndex = 0;
    var gridAlpha = 1.0;
    var origin = { x: 0, y: 0, z: 0 };
    var majorGridEvery = 5;
    var minorGridWidth = 0.2;
    var halfSize = 40;
    var yOffset = 0.001;

    var worldSize = 16384;

    var minorGridWidth = 0.5;

    var snapToGrid = false;

    var gridOverlay = Overlays.addOverlay("grid", {
        position: { x: 0 , y: 0, z: 0 },
        visible: true,
        color: colors[0],
        alpha: 1.0,
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

    var UI_URL = HIFI_PUBLIC_BUCKET + "images/tools/grid-toolbar.svg";
    var UI_WIDTH = 854;
    var UI_HEIGHT = 37;

    var horizontalGrid = opts.horizontalGrid;

    var uiOverlays = {};
    var allOverlays = [];

    function addUIOverlay(key, overlay, x, y, width, height) {
        uiOverlays[key] = {
            overlay: overlay,
            x: x,
            y: y,
            width: width,
            height: height,
        };
        allOverlays.push(overlay);
    }

    var lastKnownWindowWidth = null;
    function repositionUI() {
        if (lastKnownWindowWidth == Window.innerWidth) {
            return;
        }

        lastKnownWindowWidth = Window.innerWidth;
        var x =  Window.innerWidth / 2 - UI_WIDTH / 2;
        var y = 10;

        for (var key in uiOverlays) {
            info = uiOverlays[key];
            Overlays.editOverlay(info.overlay, {
                x: x + info.x,
                y: y + info.y,
            });
        }
    }

    // "Spritesheet" is laid out horizontally in this order
    var UI_SPRITE_LIST = [
        { name: "gridText", width: 54 },
        { name: "visibleCheckbox", width: 60 },
        { name: "snapToGridCheckbox", width: 105 },

        { name: "color0", width: 27 },
        { name: "color1", width: 27 },
        { name: "color2", width: 27 },
        { name: "color3", width: 27 },
        { name: "color4", width: 27 },

        { name: "minorGridIcon", width: 34 },
        { name: "minorGridDecrease", width: 25 },
        { name: "minorGridInput", width: 26 },
        { name: "minorGridIncrease", width: 25 },

        { name: "majorGridIcon", width: 40 },
        { name: "majorGridDecrease", width: 25 },
        { name: "majorGridInput", width: 26 },
        { name: "majorGridIncrease", width: 25 },

        { name: "yPositionLabel", width: 160 },
        { name: "moveToLabel", width: 54 },
        { name: "moveToAvatar", width: 26 },
        { name: "moveToSelection", width: 34 },
    ];

    // Add all overlays from spritesheet
    var baseOverlay = null;
    var x = 0;
    for (var i = 0; i < UI_SPRITE_LIST.length; i++) {
        var info = UI_SPRITE_LIST[i];

        var props = {
            imageURL: UI_URL,
            subImage: { x: x, y: 0, width: info.width, height: UI_HEIGHT },
            width: info.width,
            height: UI_HEIGHT,
            alpha: 1.0,
            visible: false,
        };

        var overlay;
        if (baseOverlay == null) {
            overlay = Overlays.addOverlay("image", {
                imageURL: UI_URL,
            });
            baseOverlay = overlay;
        } else {
            overlay = Overlays.cloneOverlay(baseOverlay);
        }

        Overlays.editOverlay(overlay, props);

        addUIOverlay(info.name, overlay, x, 0, info.width, UI_HEIGHT);

        x += info.width;
    }

    // Add Text overlays
    var textProperties = {
        color: { red: 255, green: 255, blue: 255 },
        topMargin: 6,
        leftMargin: 4,
        alpha: 1,
        backgroundAlpha: 0,
        text: "",
        font: { size: 12 },
        visible: false,
    };
    var minorGridWidthText = Overlays.addOverlay("text", textProperties);
    var majorGridEveryText = Overlays.addOverlay("text", textProperties);
    var yPositionText = Overlays.addOverlay("text", textProperties);

    addUIOverlay('minorGridWidthText', minorGridWidthText, 414, 8, 24, 24);
    addUIOverlay('majorGridEveryText', majorGridEveryText, 530, 8, 24, 24);
    addUIOverlay('yPositionText', yPositionText, 660, 8, 24, 24);

    var NUM_COLORS = 5;
    function updateColorIndex(index) {
        if (index < 0 || index >= NUM_COLORS) {
            return;
        }

        for (var i = 0 ; i < NUM_COLORS; i++) {
            var info = uiOverlays['color' + i];
            Overlays.editOverlay(info.overlay, {
                subImage: {
                    x: info.x,
                    y: i == index ? UI_HEIGHT : 0,
                    width: info.width,
                    height: info.height,
                }
            });
        }
    }

    function updateGridVisible(value) {
        var info = uiOverlays.visibleCheckbox;
        Overlays.editOverlay(info.overlay, {
            subImage: {
                x: info.x,
                y: value ? UI_HEIGHT : 0,
                width: info.width,
                height: info.height,
            }
        });
    }

    function updateSnapToGrid(value) {
        var info = uiOverlays.snapToGridCheckbox;
        Overlays.editOverlay(info.overlay, {
            subImage: {
                x: info.x,
                y: value ? UI_HEIGHT : 0,
                width: info.width,
                height: info.height,
            }
        });
    }

    function updateMinorGridWidth(value) {
        Overlays.editOverlay(minorGridWidthText, {
            text: value.toFixed(1),
        });
    }

    function updateMajorGridEvery(value) {
        Overlays.editOverlay(majorGridEveryText, {
            text: value,
        });
    }

    function updateYPosition(value) {
        Overlays.editOverlay(yPositionText, {
            text: value.toFixed(2),
        });
    }

    function updateOverlays() {
        updateGridVisible(horizontalGrid.getVisible());
        updateSnapToGrid(horizontalGrid.getSnapToGrid());
        updateColorIndex(horizontalGrid.getColorIndex());

        updateMinorGridWidth(horizontalGrid.getMinorGridWidth());
        updateMajorGridEvery(horizontalGrid.getMajorGridEvery());

        updateYPosition(horizontalGrid.getOrigin().y);
    }

    that.setVisible = function(visible) {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.editOverlay(allOverlays[i], { visible: visible });
        }
    }

    that.mousePressEvent = function(event) {
        var overlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (allOverlays.indexOf(overlay) >= 0) {
            if (overlay == uiOverlays.color0.overlay) {
                horizontalGrid.setColorIndex(0);
            } else if (overlay == uiOverlays.color1.overlay) {
                horizontalGrid.setColorIndex(1);
            } else if (overlay == uiOverlays.color2.overlay) {
                horizontalGrid.setColorIndex(2);
            } else if (overlay == uiOverlays.color3.overlay) {
                horizontalGrid.setColorIndex(3);
            } else if (overlay == uiOverlays.color4.overlay) {
                horizontalGrid.setColorIndex(4);
            } else if (overlay == uiOverlays.visibleCheckbox.overlay) {
                horizontalGrid.setVisible(!horizontalGrid.getVisible());
            } else if (overlay == uiOverlays.snapToGridCheckbox.overlay) {
                horizontalGrid.setSnapToGrid(!horizontalGrid.getSnapToGrid());
            } else if (overlay == uiOverlays.moveToAvatar.overlay) {
                horizontalGrid.setPosition(MyAvatar.position);
            } else if (overlay == uiOverlays.moveToSelection.overlay) {
                var newPosition = selectionManager.worldPosition;
                newPosition = Vec3.subtract(newPosition, { x: 0, y: selectionManager.worldDimensions.y * 0.5, z: 0 });
                horizontalGrid.setPosition(newPosition);
            } else if (overlay == uiOverlays.minorGridDecrease.overlay) {
                var newValue = Math.max(0.1, horizontalGrid.getMinorGridWidth() - 0.1);
                horizontalGrid.setMinorGridWidth(newValue);
            } else if (overlay == uiOverlays.minorGridIncrease.overlay) {
                horizontalGrid.setMinorGridWidth(horizontalGrid.getMinorGridWidth() + 0.1);
            } else if (overlay == uiOverlays.majorGridDecrease.overlay) {
                var newValue = Math.max(2, horizontalGrid.getMajorGridEvery() - 1);
                horizontalGrid.setMajorGridEvery(newValue);
            } else if (overlay == uiOverlays.majorGridIncrease.overlay) {
                horizontalGrid.setMajorGridEvery(horizontalGrid.getMajorGridEvery() + 1);
            } else if (overlay == uiOverlays.yPositionLabel.overlay) {
                var newValue = Window.prompt("Y Position:", horizontalGrid.getOrigin().y.toFixed(4));
                if (newValue !== null) {
                    var y = parseFloat(newValue)
                    if (isNaN(y)) {
                        Window.alert("Invalid position");
                    } else {
                        horizontalGrid.setPosition({ x: 0, y: y, z: 0 });
                    }
                }
            }

            // Clicking anywhere within the toolbar will "consume" this press event
            return true;
        }

        return false;
    };

    that.mouseReleaseEvent = function(event) {
    };

    Script.scriptEnding.connect(function() {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.deleteOverlay(allOverlays[i]);
        }
    });
    Script.update.connect(repositionUI);

    horizontalGrid.addListener(function() {
        selectionDisplay.updateHandles();
        updateOverlays();
    });

    updateOverlays();
    repositionUI();

    return that;
};
