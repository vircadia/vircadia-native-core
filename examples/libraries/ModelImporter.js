ModelImporter = function (opts) {
    var self = this;

    var windowDimensions = Controller.getViewportDimensions();

    var height = 30;
    var margin = 4;
    var outerHeight = height + (2 * margin);
    var titleWidth = 120;
    var cancelWidth = 100;
    var fullWidth = titleWidth + cancelWidth + (2 * margin);

    var localModels = Overlays.addOverlay("localmodels", {
        position: { x: 1, y: 1, z: 1 },
        scale: 1,
        visible: false
    });
    var importScale = 1;
    var importBoundaries = Overlays.addOverlay("cube", {
        position: { x: 0, y: 0, z: 0 },
        size: 1,
        color: { red: 128, blue: 128, green: 128 },
        lineWidth: 4,
        solid: false,
        visible: false
    });

    var pos = { x: windowDimensions.x / 2 - (fullWidth / 2), y: windowDimensions.y - 100 };

    var background = Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y,
        opacity: 1,
        width: fullWidth,
        height: outerHeight,
        backgroundColor: { red: 200, green: 200, blue: 200 },
        visible: false,
        text: "",
    });

    var titleText = Overlays.addOverlay("text", {
        x: pos.x + margin,
        y: pos.y + margin,
        font: { size: 14 },
        width: titleWidth,
        height: height,
        backgroundColor: { red: 255, green: 255, blue: 255 },
        color: { red: 255, green: 255, blue: 255 },
        visible: false,
        text: "Import Models"
    });
    var cancelButton = Overlays.addOverlay("text", {
        x: pos.x + margin + titleWidth,
        y: pos.y + margin,
        width: cancelWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        visible: false,
        text: "Close"
    });
    this._importing = false;

    this.setImportVisible = function (visible) {
        Overlays.editOverlay(importBoundaries, { visible: visible });
        Overlays.editOverlay(localModels, { visible: visible });
        Overlays.editOverlay(cancelButton, { visible: visible });
        Overlays.editOverlay(titleText, { visible: visible });
        Overlays.editOverlay(background, { visible: visible });
    };

    var importPosition = { x: 0, y: 0, z: 0 };
    this.moveImport = function (position) {
        importPosition = position;
        Overlays.editOverlay(localModels, {
            position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
        });
        Overlays.editOverlay(importBoundaries, {
            position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
        });
    }

    this.mouseMoveEvent = function (event) {
        if (self._importing) {
            var pickRay = Camera.computePickRay(event.x, event.y);
            var intersection = Voxels.findRayIntersection(pickRay);

            var distance = 2;// * self._scale;

            if (false) {//intersection.intersects) {
                var intersectionDistance = Vec3.length(Vec3.subtract(pickRay.origin, intersection.intersection));
                if (intersectionDistance < distance) {
                    distance = intersectionDistance * 0.99;
                }

            }

            var targetPosition = {
                x: pickRay.origin.x + (pickRay.direction.x * distance),
                y: pickRay.origin.y + (pickRay.direction.y * distance),
                z: pickRay.origin.z + (pickRay.direction.z * distance)
            };

            if (targetPosition.x < 0) targetPosition.x = 0;
            if (targetPosition.y < 0) targetPosition.y = 0;
            if (targetPosition.z < 0) targetPosition.z = 0;

            var nudgeFactor = 1;
            var newPosition = {
                x: Math.floor(targetPosition.x / nudgeFactor) * nudgeFactor,
                y: Math.floor(targetPosition.y / nudgeFactor) * nudgeFactor,
                z: Math.floor(targetPosition.z / nudgeFactor) * nudgeFactor
            }

            self.moveImport(newPosition);
        }
    }

    this.mouseReleaseEvent = function (event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({ x: event.x, y: event.y });

        if (clickedOverlay == cancelButton) {
            self._importing = false;
            self.setImportVisible(false);
        }
    };

    // Would prefer to use {4} for the coords, but it would only capture the last digit.
    var fileRegex = /__(.+)__(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)__/;
    this.doImport = function () {
        if (!self._importing) {
            var filename = Window.browse("Select models to import", "", "*.svo")
            if (filename) {
                parts = fileRegex.exec(filename);
                if (parts == null) {
                    Window.alert("The file you selected does not contain source domain or location information");
                } else {
                    var hostname = parts[1];
                    var x = parts[2];
                    var y = parts[3];
                    var z = parts[4];
                    var s = parts[5];
                    importScale = s;
                    if (hostname != location.hostname) {
                        if (!Window.confirm(("These models were not originally exported from this domain. Continue?"))) {
                            return;
                        }
                    } else {
                        if (Window.confirm(("Would you like to import back to the source location?"))) {
                            var success = Clipboard.importEntities(filename);
                            if (success) {
                                Clipboard.pasteEntities(x, y, z, 1);
                            } else {
                                Window.alert("There was an error importing the entity file.");
                            }
                            return;
                        }
                    }
                }
                var success = Clipboard.importEntities(filename);
                if (success) {
                    self._importing = true;
                    self.setImportVisible(true);
                    Overlays.editOverlay(importBoundaries, { size: s });
                } else {
                    Window.alert("There was an error importing the entity file.");
                }
            }
        }
    }

    this.paste = function () {
        if (self._importing) {
            // self._importing = false;
            // self.setImportVisible(false);
            Clipboard.pasteEntities(importPosition.x, importPosition.y, importPosition.z, 1);
        }
    }

    this.cleanup = function () {
        Overlays.deleteOverlay(localModels);
        Overlays.deleteOverlay(importBoundaries);
        Overlays.deleteOverlay(cancelButton);
        Overlays.deleteOverlay(titleText);
        Overlays.deleteOverlay(background);
    }

    Controller.mouseReleaseEvent.connect(this.mouseReleaseEvent);
    Controller.mouseMoveEvent.connect(this.mouseMoveEvent);
};
