//
//  entityCollisionExample.js
//  examples
//
//  Created by Howard Stearns on 5/25/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the per-entity event handlers.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function someCollisionFunction(entityA, entityB, collision) {
  print("collision: " + JSON.stringify({a: entityA, b: entityB, c: collision}));
}

var position = Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation));  
var properties = {
  type: "Box",
  position: position,
  dynamic: true,
  color: { red: 200, green: 0, blue: 0 }
};
var collider = Entities.addEntity(properties);
var armed = false;
function togglePrinting() {
  print('togglePrinting from ' + armed + ' on ' + collider);
  if (armed) {
    Script.removeEventHandler(collider, "collisionWithEntity", someCollisionFunction);
  } else {
    Script.addEventHandler(collider, "collisionWithEntity", someCollisionFunction);
  }
  armed = !armed;
  print("Red box " + (armed ? "will" : "will not") + " print on collision.");
}
togglePrinting();

properties.position.y += 0.2;
properties.color.blue += 200;
// A handy target for the collider to hit.
var target = Entities.addEntity(properties);

properties.position.y += 0.2;
properties.color.green += 200;
var button = Entities.addEntity(properties);
Script.addEventHandler(button, "clickReleaseOnEntity", togglePrinting);

Script.scriptEnding.connect(function () {
  Entities.deleteEntity(collider);
  Entities.deleteEntity(target);
  Entities.deleteEntity(button);
});
