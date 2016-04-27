//
//  line.js
//  examples/libraries
//
//  Created by Ryan Huffman on October 27, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function error(message) {
    print("[ERROR] " + message);
}

// PolyLine
var LINE_DIMENSIONS = { x: 2000, y: 2000, z: 2000 };
var MAX_LINE_LENGTH = 40; // This must be 2 or greater;
var DEFAULT_STROKE_WIDTH = 0.1;
var DEFAULT_LIFETIME = 20;
var DEFAULT_COLOR = { red: 255, green: 255, blue: 255 };
var PolyLine = function(position, color, lifetime, texture) {
    this.position = position;
    this.color = color;
    this.lifetime = lifetime === undefined ? DEFAULT_LIFETIME : lifetime;
    this.texture = texture ? texture : "";
    this.points = [
    ];
    this.strokeWidths = [
    ];
    this.normals = [
    ]
    this.entityID = Entities.addEntity({
        type: "PolyLine",
        position: position,
        linePoints: this.points,
        normals: this.normals,
        strokeWidths: this.strokeWidths,
        dimensions: LINE_DIMENSIONS,
        color: color,
        lifetime: lifetime,
        textures: this.texture
    });
};

PolyLine.prototype.enqueuePoint = function(position, strokeWidth) {
    if (this.isFull()) {
        error("Hit max PolyLine size");
        return;
    }

    position = Vec3.subtract(position, this.position);
    this.points.push(position);
    this.normals.push({ x: 1, y: 0, z: 0 });
    this.strokeWidths.push(strokeWidth);
    Entities.editEntity(this.entityID, {
        linePoints: this.points,
        normals: this.normals,
        strokeWidths: this.strokeWidths
    });
};

PolyLine.prototype.dequeuePoint = function() {
    if (this.points.length == 0) {
        error("Hit min PolyLine size");
        return;
    }

    this.points = this.points.slice(1);
    this.normals = this.normals.slice(1);
    this.strokeWidths = this.strokeWidths.slice(1);

    Entities.editEntity(this.entityID, {
        linePoints: this.points,
        normals: this.normals,
        strokeWidths: this.strokeWidths
    });
};

PolyLine.prototype.getFirstPoint = function() {
    return Vec3.sum(this.position, this.points[0]);
};

PolyLine.prototype.getLastPoint = function() {
    return Vec3.sum(this.position, this.points[this.points.length - 1]);
};

PolyLine.prototype.getSize = function() {
    return this.points.length;
}

PolyLine.prototype.isFull = function() {
    return this.points.length >= MAX_LINE_LENGTH;
};

PolyLine.prototype.destroy = function() {
    Entities.deleteEntity(this.entityID);
    this.points = [];
};


// InfiniteLine
InfiniteLine = function(position, color, lifetime, textureBegin, textureMiddle) {
    this.position = position;
    this.color = color;
    this.lifetime = lifetime === undefined ? DEFAULT_LIFETIME : lifetime;
    this.lines = [];
    this.size = 0;

    this.textureBegin = textureBegin ? textureBegin : "";
    this.textureMiddle = textureMiddle ? textureMiddle : "";
};

InfiniteLine.prototype.enqueuePoint = function(position, strokeWidth) {
    var currentLine;

    if (this.lines.length == 0) {
        currentLine = new PolyLine(position, this.color, this.lifetime, this.textureBegin);
        this.lines.push(currentLine);
    } else {
        currentLine = this.lines[this.lines.length - 1];
    }

    if (currentLine.isFull()) {
        var newLine = new PolyLine(currentLine.getLastPoint(), this.color, this.lifetime, this.textureMiddle);
        newLine.enqueuePoint(currentLine.getLastPoint(), strokeWidth);
        this.lines.push(newLine);
        currentLine = newLine;
    }

    currentLine.enqueuePoint(position, strokeWidth);

    ++this.size;
};

InfiniteLine.prototype.dequeuePoint = function() {
    if (this.lines.length == 0) {
        error("Trying to dequeue from InfiniteLine when no points are left");
        return;
    }

    var lastLine = this.lines[0];
    lastLine.dequeuePoint();

    if (lastLine.getSize() <= 1) {
        this.lines = this.lines.slice(1);
    }

    --this.size;
};

InfiniteLine.prototype.getFirstPoint = function() {
    return this.lines.length > 0 ? this.lines[0].getFirstPoint() : null;
};

InfiniteLine.prototype.getLastPoint = function() {
    return this.lines.length > 0 ? this.lines[lines.length - 1].getLastPoint() : null;
};

InfiniteLine.prototype.destroy = function() {
    for (var i = 0; i < this.lines.length; ++i) {
        this.lines[i].destroy();
    }

    this.size = 0;
};
