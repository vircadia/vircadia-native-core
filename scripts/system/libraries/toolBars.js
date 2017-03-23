//
//  toolBars.js
//  examples
//
//  Created by Clément Brisset on 5/7/14.
//  Persistable drag position by HRS 6/11/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Overlay2D = function(properties, overlay) { // overlay is an optional variable
    if (!(typeof(properties) === 'undefined')) {
        if(typeof(overlay) === 'undefined') {
            overlay = Overlays.addOverlay("image", properties);
        } else {
            Overlays.editOverlay(overlay, properties);
        }
    }
    
    this.overlay = function() {
        return overlay;
    }
    this.x = function() {
        return properties.x;
    }
    this.y = function() {
        return properties.y;
    }
    this.width = function() {
        return properties.width;
    }
    this.height = function() {
        return properties.height;
    }
    this.alpha = function() {
        return properties.alpha;
    }
    this.visible = function() {
        return properties.visible;
    }
    
    
    this.move = function(x, y) {
        properties.x = x;
        properties.y = y;
        Overlays.editOverlay(overlay, { x: x, y: y });
    }
    this.resize = function(width, height) {
        properties.width = width;
        properties.height = height;
        Overlays.editOverlay(overlay, { width: width, height: height });
    }
    this.setAlpha = function(alpha) {
        properties.alpha = alpha;
        Overlays.editOverlay(overlay, { alpha: alpha });
    }
    this.setImageURL = function(imageURL) {
        properties.imageURL = imageURL;
        Overlays.editOverlay(overlay, { imageURL: imageURL });
    }
    this.show = function(doShow) {
        properties.visible = doShow;
        Overlays.editOverlay(overlay, { visible: doShow });
    }
    
    this.clicked = function(clickedOverlay) {
        return overlay === clickedOverlay;
    }
    
    this.cleanup = function() {
        Overlays.deleteOverlay(overlay);
    }
}


Tool = function(properties, selectable, selected) { // selectable and selected are optional variables.
    Overlay2D.call(this, properties);
    
    if(typeof(selectable)==='undefined') {
        selectable = false;
        if(typeof(selected)==='undefined') {
            selected = false;
            
        }
    }
    
    this.selectable = function() {
        return selectable;
    }
    
    this.selected = function() {
        return selected;
    }
    this.select = function(doSelect) {
        if (!selectable) {
            return;
        }

        selected = doSelect;
        properties.subImage.y = (selected ? 0 : 1) * properties.subImage.height;
        Overlays.editOverlay(this.overlay(), { subImage: properties.subImage });
    }
    this.toggle = function() {
        if (!selectable) {
            return;
        }
        selected = !selected;
        properties.subImage.y = (selected ? 0 : 1) * properties.subImage.height;
        Overlays.editOverlay(this.overlay(), { subImage: properties.subImage });
        
        return selected;
    }
    
    this.select(selected);

    this.isButtonDown = false;
    this.buttonDown = function (down) {
        if (down !== this.isButtonDown) {
            properties.subImage.y = (down ? 0 : 1) * properties.subImage.height;
            Overlays.editOverlay(this.overlay(), { subImage: properties.subImage });
            this.isButtonDown = down;
        }
    }

    this.baseClicked = this.clicked;
    this.clicked = function(clickedOverlay, update) {
        if (this.baseClicked(clickedOverlay)) {
            if (update) {
                if (selectable) {
                    this.toggle();
                }
            }
            return true;
        }
        return false;
    }
}
Tool.prototype = new Overlay2D;
Tool.IMAGE_HEIGHT = 50;
Tool.IMAGE_WIDTH = 50;

ToolBar = function(x, y, direction, optionalPersistenceKey, optionalInitialPositionFunction, optionalOffset) {
    this.tools = new Array();
    this.x = x;
    this.y = y;
    this.offset = optionalOffset ? optionalOffset : { x: 0, y: 0 };
    this.width = 0;
    this.height = 0;
    this.backAlpha = 1.0;
    this.back = Overlays.addOverlay("rectangle", {
                    color: { red: 255, green: 255, blue: 255 },
                    x: this.x,
                    y: this.y,
                    radius: 4,
                    width: this.width,
                    height: this.height,
                    alpha: this.backAlpha,
                    visible: false
                });
    this.spacing = [];
    this.onMove = null;
    
    this.addTool = function(properties, selectable, selected) {
        if (direction == ToolBar.HORIZONTAL) {
            properties.x = this.x + this.width;
            properties.y = this.y;
            this.width += properties.width + ToolBar.SPACING;
            this.height = Math.max(properties.height, this.height);
        } else {
            properties.x = this.x;
            properties.y = this.y + this.height;
            this.width = Math.max(properties.width, this.width);
            this.height += properties.height + ToolBar.SPACING;
        }
        
        if (this.back != null) {
            Overlays.editOverlay(this.back, {
                                 width: this.width +
                                 ((direction == ToolBar.HORIZONTAL) ? 1 : 2) * ToolBar.SPACING,
                                 height: this.height +
                                 ((direction == ToolBar.VERTICAL) ? 1 : 2) * ToolBar.SPACING,
            });
        }
        
        this.tools.push(new Tool(properties, selectable, selected));
        return ((this.tools.length) - 1);
    }
    
    this.addSpacing = function(size) {
        if (direction == ToolBar.HORIZONTAL) {
            this.width += size;
        } else {
            this.height += size;
        }
        this.spacing[this.tools.length] = size;
        
        return (this.tools.length);
    }
    
    this.changeSpacing = function(size, id) {
        if (this.spacing[id] === null) {
            this.spacing[id] = 0;
        }
        var diff = size - this.spacing[id];
        this.spacing[id] = size;
        
        var dx = (direction == ToolBar.HORIZONTAL) ? diff : 0;
        var dy = (direction == ToolBar.VERTICAL) ? diff : 0;
        this.width += dx;
        this.height += dy;
        
        for(i = id; i < this.tools.length; i++) {
            this.tools[i].move(this.tools[i].x() + dx,
                               this.tools[i].y() + dy);
        }
        if (this.back != null) {
            Overlays.editOverlay(this.back, {
                                 width: this.width +
                                 ((direction == ToolBar.HORIZONTAL) ? 1 : 2) * ToolBar.SPACING,
                                 height: this.height +
                                 ((direction == ToolBar.VERTICAL) ? 1 : 2) * ToolBar.SPACING,
                                 });
        }
    }

    this.removeLastTool = function() {
        this.tools.pop().cleanup();

        if (direction == ToolBar.HORIZONTAL) {
            this.width -= Tool.IMAGE_WIDTH + ToolBar.SPACING;
        } else {
            this.height -= Tool.IMAGE_HEIGHT + ToolBar.SPACING;
        }
        if (this.back != null) {
            Overlays.editOverlay(this.back, {
                width: this.width + 2 * ToolBar.SPACING,
                height: this.height + 2 * ToolBar.SPACING
            });
        }
    }
    
    this.move = function (x, y) {
        var dx = x - this.x;
        var dy = y - this.y;
        this.x = x;
        this.y = y;
        for(var tool in this.tools) {
            this.tools[tool].move(this.tools[tool].x() + dx, this.tools[tool].y() + dy);
        }
        if (this.back != null) {
            Overlays.editOverlay(this.back, {
                x: x - ToolBar.SPACING,
                y: y - ToolBar.SPACING
            });
        }
        if (this.onMove !== null) {
            this.onMove(x, y, dx, dy);
        };
    }

    this.setAlpha = function(alpha, tool) {
        if(typeof(tool) === 'undefined') {
            for(var tool in this.tools) {
                this.tools[tool].setAlpha(alpha);
            }
            if (this.back != null) {
                this.backAlpha = alpha;
                Overlays.editOverlay(this.back, { alpha: alpha });
            }
        } else {
            this.tools[tool].setAlpha(alpha);
        }
    }

    this.setImageURL = function(imageURL, tool) {
        this.tools[tool].setImageURL(imageURL);
    }

    this.setBack = function(color, alpha) {
        if (color == null) {
            Overlays.editOverlay(this.back, { visible: false });
        } else {
            Overlays.editOverlay(this.back, {
                                 width: this.width +
                                 ((direction == ToolBar.HORIZONTAL) ? 1 : 2) * ToolBar.SPACING,
                                 height: this.height +
                                 ((direction == ToolBar.VERTICAL) ? 1 : 2) * ToolBar.SPACING,
                                 visible: true,
                                 color: color,
                                 alpha: alpha
                                 });
        }
    }

    this.showTool = function(tool, doShow) {
        this.tools[tool].show(doShow);
    }
    
    this.show = function(doShow) {
        for(var tool in this.tools) {
            this.tools[tool].show(doShow);
        }
        if (this.back != null) {
            Overlays.editOverlay(this.back, { visible: doShow});
        }
    }
    
    this.clicked = function(clickedOverlay, update) {
        if(typeof(update) === 'undefined') {
            update = true;
        }
        
        for(var tool in this.tools) {
            if (this.tools[tool].visible() && this.tools[tool].clicked(clickedOverlay, update)) {
                return parseInt(tool);
            }
        }
        return -1;
    }
    
    this.numberOfTools = function() {
        return this.tools.length;
    }
    
    this.selectTool = function (tool, select) {
        this.tools[tool].select(select);
    }

    this.toolSelected = function (tool) {
        return this.tools[tool].selected();
    }

    this.cleanup = function() {
        for(var tool in this.tools) {
            this.tools[tool].cleanup();
        }

        if (this.back != null) {
            Overlays.deleteOverlay(this.back);
            this.back = null;
        }

        this.tools = [];
        this.x = x;
        this.y = y;
        this.width = 0;
        this.height = 0;
    }

    var that = this;
    this.contains = function (xOrPoint, optionalY) {  // All four margins are draggable.
        var x = (optionalY === undefined) ? xOrPoint.x : xOrPoint,
            y = (optionalY === undefined) ? xOrPoint.y : optionalY;
        return ((that.x - ToolBar.SPACING) <= x) && (x <= (that.x + that.width + ToolBar.SPACING)) &&
            ((that.y - ToolBar.SPACING) <= y) && (y <= (that.y + that.height));
    }
    that.hover = function (enable) {  // Can be overridden or extended by clients.
        that.isHovering = enable;
        if (that.back) {
            Overlays.editOverlay(this.back, {
                visible: enable,
                alpha: enable ? 0.5 : that.backAlpha
            });
        }
    };

    function clamp(value, min, max) {
        return Math.min(Math.max(value, min), max);
    }

    var recommendedRect = Controller.getRecommendedOverlayRect();
    var recommendedDimmensions = { x: recommendedRect.width, y: recommendedRect.height };
    that.windowDimensions = recommendedDimmensions; // Controller.getViewportDimensions();
    that.origin = { x: recommendedRect.x, y: recommendedRect.y };
    // Maybe fixme: Keeping the same percent of the window size isn't always the right thing.
    // For example, maybe we want "keep the same percentage to whatever two edges are closest to the edge of screen".
    // If we change that, the places to do so are onResizeViewport, save (maybe), and the initial move based on Settings, below.
    that.onResizeViewport = function (newSize) { // Can be overridden or extended by clients.
        var recommendedRect = Controller.getRecommendedOverlayRect();
        var recommendedDimmensions = { x: recommendedRect.width, y: recommendedRect.height };
        var originRelativeX = (that.x - that.origin.x - that.offset.x);
        var originRelativeY = (that.y - that.origin.y - that.offset.y);
        var fractionX = clamp(originRelativeX / that.windowDimensions.x, 0, 1);
        var fractionY = clamp(originRelativeY / that.windowDimensions.y, 0, 1);
        that.windowDimensions = newSize || recommendedDimmensions;
        that.origin = { x: recommendedRect.x, y: recommendedRect.y };
        var newX = (fractionX * that.windowDimensions.x) + recommendedRect.x + that.offset.x;
        var newY = (fractionY * that.windowDimensions.y) + recommendedRect.y + that.offset.y;
        that.move(newX, newY);
    };
    if (optionalPersistenceKey) {
        this.fractionKey = optionalPersistenceKey + '.fraction';
        // FIXME: New default position in RC8 is bottom center of screen instead of right. Can remove this key and associated
        // code once the new toolbar position is well established with users.
        this.isNewPositionKey = optionalPersistenceKey + '.isNewPosition';
        this.save = function () {
            var recommendedRect = Controller.getRecommendedOverlayRect();
            var screenSize = { x: recommendedRect.width, y: recommendedRect.height };
            if (screenSize.x > 0 && screenSize.y > 0) {
                // Guard against invalid screen size that can occur at shut-down.
                var fraction = {x: (that.x - that.offset.x) / screenSize.x, y: (that.y - that.offset.y) / screenSize.y};
                Settings.setValue(this.fractionKey, JSON.stringify(fraction));
            }
        }
    } else {
        this.save = function () { }; // Called on move. Can be overriden or extended by clients.
    }
    // These are currently only doing that which is necessary for toolbar hover and toolbar drag.
    // They have not yet been extended to tool hover/click/release, etc.
    this.mousePressEvent = function (event) {
        if (Overlays.getOverlayAtPoint({ x: event.x, y: event.y }) == that.back) {
            that.mightBeDragging = true;
            that.dragOffsetX = that.x - event.x;
            that.dragOffsetY = that.y - event.y;
        } else {
            that.mightBeDragging = false;
        }
    };
    this.mouseReleaseEvent = function (event) {
        for (var tool in that.tools) {
            that.tools[tool].buttonDown(false);
        }
        if (that.mightBeDragging) {
            that.save();
        }
    }
    this.mouseMove  = function (event) {
        if (!that.mightBeDragging || !event.isLeftButton) {
            that.mightBeDragging = false;
            if (!that.contains(event)) {
                if (that.isHovering) {
                    that.hover(false);
                }
                return;
            }
            if (!that.isHovering) {
                that.hover(true);
            }
            return;
        }
        that.move(that.dragOffsetX + event.x, that.dragOffsetY + event.y);
    };
    that.checkResize = function () { // Can be overriden or extended, but usually not. See onResizeViewport.
        var recommendedRect = Controller.getRecommendedOverlayRect();
        var currentWindowSize = { x: recommendedRect.width, y: recommendedRect.height };

        if ((currentWindowSize.x !== that.windowDimensions.x) || (currentWindowSize.y !== that.windowDimensions.y)) {
            that.onResizeViewport(currentWindowSize);            
        }
    };
    Controller.mousePressEvent.connect(this.mousePressEvent);
    Controller.mouseReleaseEvent.connect(this.mouseReleaseEvent);
    Controller.mouseMoveEvent.connect(this.mouseMove);
    Script.update.connect(that.checkResize);
    // This compatability hack breaks the model, but makes converting existing scripts easier:
    this.addOverlay = function (ignored, oldSchoolProperties) {
        var properties = JSON.parse(JSON.stringify(oldSchoolProperties)); // a copy
        if ((that.numberOfTools() === 0) && (properties.x != undefined)  && (properties.y != undefined)) {
            that.move(properties.x, properties.y);
        }
        delete properties.x;
        delete properties.y;
        var index = that.addTool(properties);
        var id = that.tools[index].overlay();
        return id;
    }
    if (this.fractionKey || optionalInitialPositionFunction) {
        var isNewPosition = Settings.getValue(this.isNewPositionKey);
        var savedFraction = isNewPosition ? JSON.parse(Settings.getValue(this.fractionKey) || "0") : 0;
        Settings.setValue(this.isNewPositionKey, true);

        var recommendedRect = Controller.getRecommendedOverlayRect();
        var screenSize = { x: recommendedRect.width, y: recommendedRect.height };
        if (savedFraction) {
            // If we have saved data, keep the toolbar at the same proportion of the screen width/height.
            that.move(savedFraction.x * screenSize.x + that.offset.x, savedFraction.y * screenSize.y + that.offset.y);
        } else if (!optionalInitialPositionFunction) {
            print("No initPosition(screenSize, intializedToolbar) specified for ToolBar");
        } else {
            // Call the optionalInitialPositionFunctinon() AFTER the client has had a chance to set up.
            var that = this;
            Script.setTimeout(function () {
                var position = optionalInitialPositionFunction(screenSize, that);
                that.move(position.x + that.offset.x, position.y + that.offset.y);
            }, 0);
        }
    }
}
ToolBar.SPACING = 6;
ToolBar.VERTICAL = 0;
ToolBar.HORIZONTAL = 1;
