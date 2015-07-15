//
//  floatingUI.js
//  examples/example/ui
//
//  Created by Alexander Otavka
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include([
  "../../libraries/globals.js",
  "../../libraries/overlayUtils.js",
]);

var BG_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/card-bg.svg";
var RED_DOT_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/red-dot.svg";
var BLUE_SQUARE_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/blue-square.svg";

var BLANK_ROTATION = { x: 0, y: 0, z: 0, w: 0 };

function isBlank(rotation) {
  return rotation.x == BLANK_ROTATION.x &&
         rotation.y == BLANK_ROTATION.y &&
         rotation.z == BLANK_ROTATION.z &&
         rotation.w == BLANK_ROTATION.w;
}

var panel = new FloatingUIPanel({
  offsetPosition: { x: 0, y: 0, z: 1 },
});

var bg = panel.addChild(new BillboardOverlay({
  url: BG_IMAGE_URL,
  dimensions: {
      x: 0.5,
      y: 0.5,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
}));

var redDot = panel.addChild(new BillboardOverlay({
  url: RED_DOT_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  offsetPosition: {
    x: -0.15,
    y: -0.15,
    z: -0.001
  }
}));

var redDot2 = panel.addChild(new BillboardOverlay({
  url: RED_DOT_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  offsetPosition: {
    x: -0.15,
    y: 0,
    z: -0.001
  }
}));

var blueSquare = panel.addChild(new BillboardOverlay({
  url: BLUE_SQUARE_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  offsetPosition: {
    x: 0.1,
    y: 0,
    z: -0.001
  }
}));

var blueSquare2 = panel.addChild(new BillboardOverlay({
  url: BLUE_SQUARE_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  offsetPosition: {
    x: 0.1,
    y: 0.11,
    z: -0.001
  }
}));

var blueSquare3 = blueSquare2.clone();
blueSquare3.offsetPosition = {
  x: -0.01,
  y: 0.11,
  z: -0.001
};
blueSquare3.ignoreRayIntersection = false;

Controller.mousePressEvent.connect(function(event) {
  if (event.isRightButton) {
    var newOffsetRotation;
    print(JSON.stringify(panel.offsetRotation))
    if (isBlank(panel.offsetRotation)) {
      newOffsetRotation = Quat.multiply(MyAvatar.orientation, { x: 0, y: 1, z: 0, w: 0 });
    } else {
      newOffsetRotation = BLANK_ROTATION;
    }
    panel.offsetRotation = newOffsetRotation;
  } else if (event.isLeftButton) {
    var pickRay = Camera.computePickRay(event.x, event.y)
    var overlay = panel.findRayIntersection(pickRay);
    if (overlay) {
      var oldPos = overlay.offsetPosition;
      var newPos = {
        x: Number(oldPos.x),
        y: Number(oldPos.y),
        z: Number(oldPos.z) + 0.1
      }
      overlay.offsetPosition = newPos;
    }
  }
});

Script.scriptEnding.connect(function() {
  panel.destroy();
});