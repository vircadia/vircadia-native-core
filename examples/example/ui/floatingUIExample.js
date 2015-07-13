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

Script.include(["../../libraries/globals.js"]);

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

var panel = Overlays.addPanel({
  offsetPosition: { x: 0, y: 0, z: 1 },
});

var panelChildren = [];

var bg = Overlays.addOverlay("billboard", {
  url: BG_IMAGE_URL,
  dimensions: {
      x: 0.5,
      y: 0.5,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  attachedPanel: panel,
});
panelChildren.push(bg);

var redDot = Overlays.addOverlay("billboard", {
  url: RED_DOT_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  attachedPanel: panel,
  offsetPosition: {
    x: -0.15,
    y: -0.15,
    z: -0.001
  }
});
panelChildren.push(redDot);

var redDot2 = Overlays.addOverlay("billboard", {
  url: RED_DOT_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  attachedPanel: panel,
  offsetPosition: {
    x: -0.15,
    y: 0,
    z: -0.001
  }
});
panelChildren.push(redDot2);

var blueSquare = Overlays.addOverlay("billboard", {
  url: BLUE_SQUARE_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  attachedPanel: panel,
  offsetPosition: {
    x: 0.1,
    y: 0,
    z: -0.001
  }
});
panelChildren.push(blueSquare);

var blueSquare2 = Overlays.addOverlay("billboard", {
  url: BLUE_SQUARE_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  attachedPanel: panel,
  offsetPosition: {
    x: 0.1,
    y: 0.11,
    z: -0.001
  }
});
panelChildren.push(blueSquare2);

var blueSquare3 = Overlays.addOverlay("billboard", {
  url: BLUE_SQUARE_IMAGE_URL,
  dimensions: {
      x: 0.1,
      y: 0.1,
  },
  isFacingAvatar: false,
  visible: true,
  alpha: 1.0,
  ignoreRayIntersection: false,
  attachedPanel: panel,
  offsetPosition: {
    x: -0.01,
    y: 0.11,
    z: -0.001
  }
});
panelChildren.push(blueSquare3);

Controller.mousePressEvent.connect(function(event) {
  if (event.isRightButton) {
    var newOffsetRotation = BLANK_ROTATION;
    if (isBlank(Overlays.getPanelProperty(panel, "offsetRotation"))) {
      newOffsetRotation = Quat.multiply(MyAvatar.orientation, { x: 0, y: 1, z: 0, w: 0 });
    }
    Overlays.editPanel(panel, {
      offsetRotation: newOffsetRotation
    });
  } else if (event.isLeftButton) {
    var pickRay = Camera.computePickRay(event.x, event.y)
    var rayPickResult = Overlays.findRayIntersection(pickRay);
    print(String(rayPickResult.overlayID));
    if (rayPickResult.intersects) {
      for (var i in panelChildren) {
        if (panelChildren[i] == rayPickResult.overlayID) {
          var oldPos = Overlays.getProperty(rayPickResult.overlayID, "offsetPosition");
          var newPos = {
            x: Number(oldPos.x),
            y: Number(oldPos.y),
            z: Number(oldPos.z) + 0.1
          }
          Overlays.editOverlay(rayPickResult.overlayID, { offsetPosition: newPos });
        }
      }
    }
  }
});

Script.scriptEnding.connect(function() {
  Overlays.deletePanel(panel);
});