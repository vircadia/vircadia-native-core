//
//  toolBars.js
//  examples
//
//  Created by Clément Brisset on 5/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Overlay2D = function(properties, overlay) { // overlay is an optionnal variable
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
    this.show = function(doShow) {
        properties.visible = doShow;
        Overlays.editOverlay(overlay, { visible: doShow });
    }
    
    this.clicked = function(clickedOverlay) {
        return (overlay == clickedOverlay ? true : false);
    }
    
    this.cleanup = function() {
        print("Cleanup");
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
        properties.subImage.y = (selected ? 2 : 1) * properties.subImage.height;
        Overlays.editOverlay(this.overlay(), { subImage: properties.subImage });
    }
    this.toggle = function() {
        if (!selectable) {
            return;
        }
        selected = !selected;
        properties.subImage.y = (selected ? 2 : 1) * properties.subImage.height;
        Overlays.editOverlay(this.overlay(), { subImage: properties.subImage });
        
        return selected;
    }
    
    this.select(selected);
    
    this.baseClicked = this.clicked;
    this.clicked = function(clickedOverlay) {
        if (this.baseClicked(clickedOverlay)) {
            if (selectable) {
                this.toggle();
            }
            return true;
        }
        return false;
    }
}
Tool.prototype = new Overlay2D;
Tool.IMAGE_HEIGHT = 50;
Tool.IMAGE_WIDTH = 50;

ToolBar = function(x, y, direction) {
    this.tools = [];
    this.x = x;
    this.y = y;
    this.width = 0;
    this.height = 0;
    this.back = this.back = Overlays.addOverlay("text", {
                    backgroundColor: { red: 255, green: 255, blue: 255 },
                    x: this.x,
                    y: this.y,
                    width: this.width,
                    height: this.height,
                    alpha: 1.0,
                    visible: false
                });
    
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
                width: this.width + 2 * ToolBar.SPACING,
                height: this.height + 2 * ToolBar.SPACING
            });
        }
        
        this.tools[this.tools.length] = new Tool(properties, selectable, selected);
        return ((this.tools.length) - 1);
    }
    
    this.move = function(x, y) {
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
    }
    
    this.setAlpha = function(alpha, tool) {
        if(typeof(tool) === 'undefined') {
            for(var tool in this.tools) {
                this.tools[tool].setAlpha(alpha);
            }
            if (this.back != null) {
                Overlays.editOverlay(this.back, { alpha: alpha});
            }
        } else {
            this.tools[tool].setAlpha(alpha);
        }
    }

    this.setBack = function(color, alpha) {
        if (color == null) {
            Overlays.editOverlay(this.back, {
                visible: false 
            });
        } else {
            Overlays.editOverlay(this.back, {
                visible: true,
                backgroundColor: color,
                alpha: alpha
            })
        }
    }
    
    this.show = function(doShow) {
        for(var tool in this.tools) {
            this.tools[tool].show(doShow);
        }
        if (this.back != null) {
            Overlays.editOverlay(this.back, { visible: doShow});
        }
    }
    
    this.clicked = function(clickedOverlay) {
        for(var tool in this.tools) {
            if (this.tools[tool].visible() && this.tools[tool].clicked(clickedOverlay)) {
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
            delete this.tools[tool];
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
}
ToolBar.SPACING = 4;
ToolBar.VERTICAL = 0;
ToolBar.HORIZONTAL = 1;