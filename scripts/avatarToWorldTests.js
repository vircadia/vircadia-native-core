var AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

var debugSphereBaseProperties = {
               type: "Sphere",
               dimensions: { x: 0.2, y: 0.2, z: 0.2 },
               dynamic: false,
               collisionless: true,
               gravity: { x: 0, y: 0, z: 0 },
               lifetime: 10.0,
               userData: "{ \"grabbableKey\": { \"grabbable\": false, \"kinematic\": false } }"
};

var debugBoxBaseProperties = {
               type: "Box",
               dimensions: { x: 0.2, y: 0.2, z: 0.2 },
               dynamic: false,
               collisionless: true,
               gravity: { x: 0, y: 0, z: 0 },
               lifetime: 10.0,
               userData: "{ \"grabbableKey\": { \"grabbable\": false, \"kinematic\": false } }"
};

//jointToWorldPoint
// create sphere for finger on left hand
//	each frame, calculate world position of finger, with some offset
//	update sphere to match this position
var jointToWorldPointTest_sphereEntity;
function jointToWorldPointTest() {
   var jointIndex = MyAvatar.getJointIndex("LeftHandPinky4");
   var jointOffset_WorldSpace = { x: 0.1, y: 0, z: 0 };
   var worldPos = MyAvatar.jointToWorldPoint(jointOffset_WorldSpace, jointIndex);
       
   var jointSphereProps = Object.create(debugSphereBaseProperties);
   jointSphereProps.name = "jointToWorldPointTest_Sphere";
   jointSphereProps.color =  { blue: 240, green: 150, red: 150 };
   jointSphereProps.position = worldPos;
   jointToWorldPointTest_sphereEntity = Entities.addEntity(jointSphereProps);
}
function jointToWorldPointTest_update(deltaTime) {
   var jointIndex = MyAvatar.getJointIndex("LeftHandPinky4");
   var jointOffset_WorldSpace = { x: 0.1, y: 0, z: 0 };
   var worldPos = MyAvatar.jointToWorldPoint(jointOffset_WorldSpace, jointIndex);
   var newProperties = { position: worldPos };   
   Entities.editEntity(jointToWorldPointTest_sphereEntity, newProperties);
}

//jointToWorldDirection
//	create line in world space
//	each frame calculate world space direction of players head z axis
//	update line to match
var jointToWorldDirectionTest_lineEntity;
function jointToWorldDirectionTest() {
   var jointIndex = MyAvatar.getJointIndex("Head");
   var avatarPos = MyAvatar.getJointPosition(jointIndex);

   var jointDir = { x: 1, y: 0, z: 1 };
   var worldDir = MyAvatar.jointToWorldDirection(jointDir, jointIndex);
   print(worldDir.x);
   print(worldDir.y);
   print(worldDir.z);
   jointToWorldDirectionTest_lineEntity = Entities.addEntity({
      type: "Line",
      color: {red: 250, green: 0, blue: 0},
      dimensions: {x: 5, y: 5, z: 5},
      lifetime: 10.0,
      linePoints: [{
        x: 0,
        y: 0,
        z: 0
       }, worldDir
       ],
      position : avatarPos,
   });
}
function jointToWorldDirection_update(deltaTime) {
   var jointIndex = MyAvatar.getJointIndex("Head");
   var avatarPos = MyAvatar.getJointPosition(jointIndex);
   var jointDir = { x: 1, y: 0, z: 0 };
   var worldDir = MyAvatar.jointToWorldDirection(jointDir, jointIndex);
   var newProperties = {
      linePoints: [{
        x: 0,
        y: 0,
        z: 0
      }, worldDir
      ],
      position : avatarPos
   };
   
   Entities.editEntity(jointToWorldDirectionTest_lineEntity, newProperties);
}

//jointToWorldRotation
//	create box in world space
//	each frame calculate world space rotation of players head
//	update box rotation to match	
var jointToWorldRotationTest_boxEntity;
function jointToWorldRotationTest() {
   var jointIndex = MyAvatar.getJointIndex("Head");
   var jointPosition_WorldSpace =  MyAvatar.getJointPosition(jointIndex);
   var jointRot = MyAvatar.getJointRotation(jointIndex);
   var jointRot_WorldSpace = MyAvatar.jointToWorldRotation(jointRot, jointIndex);
       
   var boxProps = Object.create(debugBoxBaseProperties);
   boxProps.name = "jointToWorldRotationTest_Box";
   boxProps.color =  { blue: 250, green: 250, red: 250 };
   boxProps.position = jointPosition_WorldSpace;
   boxProps.rotation = jointRot_WorldSpace;
   jointToWorldRotationTest_boxEntity = Entities.addEntity(boxProps);
}
function jointToWorldRotationTest_update(deltaTime) {
   var jointIndex = MyAvatar.getJointIndex("Head");
   var jointPosition_WorldSpace =  MyAvatar.getJointPosition(jointIndex);
   var jointRot = MyAvatar.getJointRotation(jointIndex);
   var jointRot_WorldSpace = MyAvatar.jointToWorldRotation(jointRot, jointIndex);
   var newProperties = { position: jointPosition_WorldSpace, rotation: jointRot_WorldSpace };   
   Entities.editEntity(jointToWorldRotationTest_boxEntity, newProperties);
}

jointToWorldPointTest();
Script.update.connect(jointToWorldPointTest_update);

jointToWorldDirectionTest();
Script.update.connect(jointToWorldDirection_update);

jointToWorldRotationTest();
Script.update.connect(jointToWorldRotationTest_update);
