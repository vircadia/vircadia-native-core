//
//  lineExample.js
//  examples/example
//
//  Created by Ryan Huffman on October 27, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../libraries/line.js");

var basePosition = MyAvatar.position;
var color = { red: 128, green: 220, blue: 190 };
var strokeWidth = 0.01;
var line = new InfiniteLine(basePosition, color, 20);

for (var i = 0; i < (16 * Math.PI); i += 0.05) {
    var x = 0
    var y = 0.25 * Math.sin(i);
    var z = i / 10;

    var position = Vec3.sum(basePosition, { x: x, y: y, z: z });
    line.enqueuePoint(position, strokeWidth);
}
