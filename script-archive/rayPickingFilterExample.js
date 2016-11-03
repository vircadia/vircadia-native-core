   //
   //  rayPickingFilterExample.js
   //  examples
   //
   //  Created by Eric Levin on 12/24/2015
   //  Copyright 2015 High Fidelity, Inc.
   //
   //  This is an example script that demonstrates the use of filtering entities for ray picking
   //
   //  Distributed under the Apache License, Version 2.0.
   //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
   //


   var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));

   var whiteListBox = Entities.addEntity({
       type: "Box",
       color: {
           red: 10,
           green: 200,
           blue: 10
       },
       dimensions: {
           x: 0.2,
           y: 0.2,
           z: 0.2
       },
       position: center
   });

   var blackListBox = Entities.addEntity({
       type: "Box",
       color: {
           red: 100,
           green: 10,
           blue: 10
       },
       dimensions: {
           x: 0.2,
           y: 0.2,
           z: 0.2
       },
       position: Vec3.sum(center, {
           x: 0,
           y: 0.3,
           z: 0
       })
   });


   function castRay(event) {
       var pickRay = Camera.computePickRay(event.x, event.y);
       // In this example every entity will be pickable except the entities in the blacklist array
       // the third argument is the whitelist array,and the fourth and final is the blacklist array
       var pickResults = Entities.findRayIntersection(pickRay, true, [], [blackListBox]);
       
       // With below example, only entities added to whitelist will be pickable
       // var pickResults = Entities.findRayIntersection(pickRay, true, [whiteListBox], []);

       if (pickResults.intersects) {
           print("INTERSECTION!");
       }

   }

   function cleanup() {
       Entities.deleteEntity(whiteListBox);
       Entities.deleteEntity(blackListBox);
   }

   Script.scriptEnding.connect(cleanup);
   Controller.mousePressEvent.connect(castRay);