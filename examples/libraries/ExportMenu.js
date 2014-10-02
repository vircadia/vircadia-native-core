var ExportMenu = function (opts) {
    var self = this;

    var windowDimensions = Controller.getViewportDimensions();
    var pos = { x: windowDimensions.x / 2, y: windowDimensions.y - 100 };

    this._onClose = opts.onClose || function () { };
    this._position = { x: 0.0, y: 0.0, z: 0.0 };
    this._scale = 1.0;

    var minScale = 1;
    var maxScale = 32768;
    var titleWidth = 120;
    var locationWidth = 100;
    var scaleWidth = 144;
    var exportWidth = 100;
    var cancelWidth = 100;
    var margin = 4;
    var height = 30;
    var outerHeight = height + (2 * margin);
    var buttonColor = { red: 128, green: 128, blue: 128 };

    var SCALE_MINUS = scaleWidth * 40.0 / 100.0;
    var SCALE_PLUS = scaleWidth * 63.0 / 100.0;

    var fullWidth = locationWidth + scaleWidth + exportWidth + cancelWidth + (2 * margin);
    var offset = fullWidth / 2;
    pos.x -= offset;

    var background = Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y,
        opacity: 1,
        width: fullWidth,
        height: outerHeight,
        backgroundColor: { red: 200, green: 200, blue: 200 },
        text: "",
    });

    var titleText = Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y - height,
        font: { size: 14 },
        width: titleWidth,
        height: height,
        backgroundColor: { red: 255, green: 255, blue: 255 },
        color: { red: 255, green: 255, blue: 255 },
        text: "Export Models"
    });

    var locationButton = Overlays.addOverlay("text", {
        x: pos.x + margin,
        y: pos.y + margin,
        width: locationWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        text: "0, 0, 0",
    });
    var scaleOverlay = Overlays.addOverlay("image", {
        x: pos.x + margin + locationWidth,
        y: pos.y + margin,
        width: scaleWidth,
        height: height,
        subImage: { x: 0, y: 3, width: 144, height: height },
        imageURL: toolIconUrl + "voxel-size-selector.svg",
        alpha: 0.9,
    });
    var scaleViewWidth = 40;
    var scaleView = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + SCALE_MINUS,
        y: pos.y + margin,
        width: scaleViewWidth,
        height: height,
        alpha: 0.0,
        color: { red: 255, green: 255, blue: 255 },
        text: "1"
    });
    var exportButton = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + scaleWidth,
        y: pos.y + margin,
        width: exportWidth,
        height: height,
        color: { red: 0, green: 255, blue: 255 },
        text: "Export"
    });
    var cancelButton = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + scaleWidth + exportWidth,
        y: pos.y + margin,
        width: cancelWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        text: "Cancel"
    });

    var voxelPreview = Overlays.addOverlay("cube", {
        position: { x: 0, y: 0, z: 0 },
        size: this._scale,
        color: { red: 255, green: 255, blue: 0 },
        alpha: 1,
        solid: false,
        visible: true,
        lineWidth: 4
    });

    this.parsePosition = function (str) {
        var parts = str.split(',');
        if (parts.length == 3) {
            var x = parseFloat(parts[0]);
            var y = parseFloat(parts[1]);
            var z = parseFloat(parts[2]);
            if (isFinite(x) && isFinite(y) && isFinite(z)) {
                return { x: x, y: y, z: z };
            }
        }
        return null;
    };

    this.showPositionPrompt = function () {
        var positionStr = self._position.x + ", " + self._position.y + ", " + self._position.z;
        while (1) {
            positionStr = Window.prompt("Position to export form:", positionStr);
            if (positionStr == null) {
                break;
            }
            var position = self.parsePosition(positionStr);
            if (position != null) {
                self.setPosition(position.x, position.y, position.z);
                break;
            }
            Window.alert("The position you entered was invalid.");
        }
    };

    this.setScale = function (scale) {
        self._scale = Math.min(maxScale, Math.max(minScale, scale));
        Overlays.editOverlay(scaleView, { text: self._scale });
        Overlays.editOverlay(voxelPreview, { size: self._scale });
    }

    this.decreaseScale = function () {
        self.setScale(self._scale /= 2);
    }

    this.increaseScale = function () {
        self.setScale(self._scale *= 2);
    }

    this.exportEntities = function() {
        var x = self._position.x;
        var y = self._position.y;
        var z = self._position.z;
        var s = self._scale;
        var filename = "models__" + Window.location.hostname + "__" + x + "_" + y + "_" + z + "_" + s + "__.svo";
        filename = Window.save("Select where to save", filename, "*.svo")
        if (filename) {
            var success = Clipboard.exportEntities(filename, x, y, z, s);
            if (!success) {
                Window.alert("Export failed: no models found in selected area.");
            }
        }
        self.close();
    };

    this.getPosition = function () {
        return self._position;
    };

    this.setPosition = function (x, y, z) {
        self._position = { x: x, y: y, z: z };
        var positionStr = x + ", " + y + ", " + z;
        Overlays.editOverlay(locationButton, { text: positionStr });
        Overlays.editOverlay(voxelPreview, { position: self._position });

    };

    this.mouseReleaseEvent = function (event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (clickedOverlay == locationButton) {
            self.showPositionPrompt();
        } else if (clickedOverlay == exportButton) {
            self.exportEntities();
        } else if (clickedOverlay == cancelButton) {
            self.close();
        } else if (clickedOverlay == scaleOverlay) {
            var x = event.x - pos.x - margin - locationWidth;
            print(x);
            if (x < SCALE_MINUS) {
                self.decreaseScale();
            } else if (x > SCALE_PLUS) {
                self.increaseScale();
            }
        }
    };

    this.close = function () {
        this.cleanup();
        this._onClose();
    };

    this.cleanup = function () {
        Overlays.deleteOverlay(background);
        Overlays.deleteOverlay(titleText);
        Overlays.deleteOverlay(locationButton);
        Overlays.deleteOverlay(exportButton);
        Overlays.deleteOverlay(cancelButton);
        Overlays.deleteOverlay(voxelPreview);
        Overlays.deleteOverlay(scaleOverlay);
        Overlays.deleteOverlay(scaleView);
    };

    print("CONNECTING!");
    Controller.mouseReleaseEvent.connect(this.mouseReleaseEvent);
};
