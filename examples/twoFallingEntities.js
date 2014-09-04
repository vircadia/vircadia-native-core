//
//  Testing.js 
//  
//  Creates a red 0.2 meter diameter ball right in front of your avatar that lives for 60 seconds
// 

var radius = 0.1;
var position = Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.orientation));  
var properties = {
                type: "Sphere",
                position: position, 
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: -0.05, z: 0}, 
                radius: radius,
                damping: 0.999,
                color: { red: 200, green: 0, blue: 0 },
                lifetime: 60     
            };

var newEntity = Entities.addEntity(properties);
position.x -= radius * 1.0;
properties.position = position; 
var newEntityTwo = Entities.addEntity(properties);

Script.stop(); // no need to run anymore