//
//  locationsMenu.js
//  examples
//
//  Created by Ryan Huffman on 5/28/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var scriptUrl = "https://script.google.com/macros/s/AKfycbwIo4lmF-qUwX1Z-9eA_P-g2gse9oFhNcjVyyksGukyDDEFXgU/exec?action=listOwners&domain=alpha.highfidelity.io";

var LocationMenu = function(opts) {
    var self = this;

    var pageSize = opts.pageSize || 10;
    var menuWidth = opts.menuWidth || 150;
    var menuHeight = opts.menuItemHeight || 24;

    var inactiveColor = { red: 51, green: 102, blue: 102 };
    var activeColor = { red: 18, green: 66, blue: 66 };
    var prevNextColor = { red: 192, green: 192, blue: 192 };
    var disabledColor = { red: 64, green: 64, blue: 64};
    var position = { x: 0, y: 0 };

    var locationIconUrl = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/tools/location.svg";
    var toolHeight = 50;
    var toolWidth = 50;
    var visible = false;
    var menuItemOffset = {
        x: 55,
        y: 0,
    };
    var menuItemPadding = 5;
    var margin = 7;
    var fullMenuHeight = (2 * menuItemOffset.y) + (menuHeight * (pageSize + 1));
    var menuOffset = -fullMenuHeight + toolHeight;

    var windowDimensions = Controller.getViewportDimensions();

    this.locations = [];
    this.numPages = 1;
    this.page = 0;

    this.menuToggleButton = Overlays.addOverlay("image", {
        x: position.x,
        y: position.y,
        width: toolWidth, height: toolHeight,
        subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
        imageURL: locationIconUrl,
        alpha: 0.9
    });

    this.background = Overlays.addOverlay("text", {
        x: 0,
        y: 0,
        width: menuWidth + 10,
        height: (menuHeight * (pageSize + 1)) + 10,
        backgroundColor: { red: 0, green: 0, blue: 0},
        topMargin: 4,
        leftMargin: 4,
        text: "",
        visible: visible,
    });

    this.menuItems = [];
    for (var i = 0; i < pageSize; i++) {
        var menuItem = Overlays.addOverlay("text", {
            x: 0,
            y: 0,
            width: menuWidth,
            height: menuHeight,
            backgroundColor: inactiveColor,
            topMargin: margin,
            leftMargin: margin,
            text: (i == 0) ? "Loading..." : "",
            visible: visible,
        });
        this.menuItems.push({ overlay: menuItem, location: null });
    }

    this.previousButton = Overlays.addOverlay("text", {
        x: 0,
        y: 0,
        width: menuWidth / 2,
        height: menuHeight,
        backgroundColor: disabledColor,
        topMargin: margin,
        leftMargin: margin,
        text: "Previous",
        visible: visible,
    });

    this.nextButton = Overlays.addOverlay("text", {
        x: 0,
        y: 0,
        width: menuWidth / 2,
        height: menuHeight,
        backgroundColor: disabledColor,
        topMargin: margin,
        leftMargin: margin,
        text: "Next",
        visible: visible,
    });

    this.reposition = function(force) {
        var newWindowDimensions = Controller.getViewportDimensions();
        if (force || newWindowDimensions.y != windowDimensions.y) {
            windowDimensions = newWindowDimensions;

            position.x = 8;
            position.y = Math.floor(windowDimensions.y / 2) + 25 + 50 + 8;

            Overlays.editOverlay(self.menuToggleButton, {
                x: position.x,
                y: position.y,
            });
            Overlays.editOverlay(self.background, {
                x: position.x + menuItemOffset.x,
                y: position.y + menuItemOffset.y - 2 * menuItemPadding + menuOffset,
            });
            for (var i = 0; i < pageSize; i++) {
                Overlays.editOverlay(self.menuItems[i].overlay, {
                    x: position.x + menuItemOffset.x + menuItemPadding,
                    y: position.y + menuItemOffset.y - menuItemPadding + (i * menuHeight) + menuOffset,
                });
            }
            Overlays.editOverlay(self.previousButton, {
                x: position.x + menuItemOffset.x + menuItemPadding,
                y: position.y + menuItemOffset.y - menuItemPadding + (pageSize * menuHeight) + menuOffset,
            });
            Overlays.editOverlay(self.nextButton, {
                x: position.x + menuItemOffset.x + menuItemPadding + (menuWidth / 2),
                y: position.y + menuItemOffset.y - menuItemPadding + (pageSize * menuHeight) + menuOffset,
            });
        }
    }

    this.updateLocations = function(locations) {
        this.locations = locations;
        this.numPages = Math.ceil(locations.length / pageSize);
        this.goToPage(0);
    }

    this.setError = function() {
        Overlays.editOverlay(this.menuItems[0].overlay, { text: "Error loading data" });
    }

    this.toggleMenu = function() {
        visible = !visible;
        for (var i = 0; i < this.menuItems.length; i++) {
            Overlays.editOverlay(this.menuItems[i].overlay, { visible: visible});
        }
        Overlays.editOverlay(this.previousButton, { visible: visible});
        Overlays.editOverlay(this.nextButton, { visible: visible});
        Overlays.editOverlay(this.background, { visible: visible});
        if (visible) {
            Overlays.editOverlay(this.menuToggleButton, { subImage: { x: 0, y: 0, width: toolWidth, height: toolHeight } }),
        } else {
            Overlays.editOverlay(this.menuToggleButton, { subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight } }),
        }
    }

    this.goToPage = function(pageNumber) {
        if (pageNumber < 0 || pageNumber >= this.numPages) {
            return;
        }

        this.page = pageNumber;
        var start = pageNumber * pageSize;
        for (var i = 0; i < pageSize; i++) {
            var update = {};
            var location = null;
            if (start + i < this.locations.length) {
                location = this.locations[start + i];
                update.text = (start + i + 1) + ". " + location.username;
                update.backgroundColor = inactiveColor;
            } else {
                update.text = "";
                update.backgroundColor = disabledColor;
            }
            Overlays.editOverlay(this.menuItems[i].overlay, update);
            this.menuItems[i].location = location;
        }

        this.previousEnabled = pageNumber > 0;
        this.nextEnabled = pageNumber < (this.numPages - 1);

        Overlays.editOverlay(this.previousButton, { backgroundColor: this.previousEnabled ? prevNextColor : disabledColor});
        Overlays.editOverlay(this.nextButton, { backgroundColor: this.nextEnabled ? prevNextColor : disabledColor });
    }

    this.mousePressEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

        if (clickedOverlay == self.menuToggleButton) {
            self.toggleMenu();
        } else if (clickedOverlay == self.previousButton) {
            if (self.previousEnabled) {
                Overlays.editOverlay(clickedOverlay, { backgroundColor: activeColor });
            }
        } else if (clickedOverlay == self.nextButton) {
            if (self.nextEnabled) {
                Overlays.editOverlay(clickedOverlay, { backgroundColor: activeColor });
            }
        } else {
            for (var i = 0; i < self.menuItems.length; i++) {
                if (clickedOverlay == self.menuItems[i].overlay) {
                    if (self.menuItems[i].location != null) {
                        Overlays.editOverlay(clickedOverlay, { backgroundColor: activeColor });
                    }
                    break;
                }
            }
        }
    }

    this.mouseReleaseEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

        if (clickedOverlay == self.previousButton) {
            if (self.previousEnabled) {
                Overlays.editOverlay(clickedOverlay, { backgroundColor: inactiveColor });
                self.goToPage(self.page - 1);
            }
        } else if (clickedOverlay == self.nextButton) {
            if (self.nextEnabled) {
                Overlays.editOverlay(clickedOverlay, { backgroundColor: inactiveColor });
                self.goToPage(self.page + 1);
            }
        } else {
            for (var i = 0; i < self.menuItems.length; i++) {
                if (clickedOverlay == self.menuItems[i].overlay) {
                    if (self.menuItems[i].location != null) {
                        Overlays.editOverlay(clickedOverlay, { backgroundColor: inactiveColor });
                        var location = self.menuItems[i].location;
                        Window.location = "hifi://" + location.domain + "/"
                                          + location.x + "," + location.y + "," + location.z;
                    }
                    break;
                }
            }
        }
    }

    this.cleanup = function() {
        for (var i = 0; i < self.menuItems.length; i++) {
            Overlays.deleteOverlay(self.menuItems[i].overlay);
        }
        Overlays.deleteOverlay(self.menuToggleButton);
        Overlays.deleteOverlay(self.previousButton);
        Overlays.deleteOverlay(self.nextButton);
        Overlays.deleteOverlay(self.background);
    }

    Controller.mousePressEvent.connect(this.mousePressEvent);
    Controller.mouseReleaseEvent.connect(this.mouseReleaseEvent);
    Script.update.connect(this.reposition);
    Script.scriptEnding.connect(this.cleanup);

    this.reposition(true);
};

var locationMenu = new LocationMenu({ pageSize: 8 });

print("Loading strip data from " + scriptUrl);

var req = new XMLHttpRequest();
req.responseType = 'json';

req.onreadystatechange = function() {
    if (req.readyState == req.DONE) {
        if (req.status == 200 && req.response != null) {
            for (var domain in req.response) {
                var locations = req.response[domain];
                var users = [];
                for (var i = 0; i < locations.length; i++) {
                    var loc = locations[i];
                    var x1 = loc[1],
                        x2 = loc[2],
                        y1 = loc[3],
                        y2 = loc[4];
                    users.push({
                        domain: domain,
                        username: loc[0],
                        x: x1,
                        y: 300,
                        z: y1,
                    });
                }
                locationMenu.updateLocations(users);
            }
        } else {
            print("Error loading data: " + req.status + " " + req.statusText + ", " + req.errorCode + ": " + req.responseText);
            locationMenu.setError();
        }
    }
}

req.open("GET", scriptUrl);
req.send();
