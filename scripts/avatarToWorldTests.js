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

//avatarToWorldPoint
// create sphere for finger on left hand
//	each frame, calculate world position of finger, with some offset
//	update sphere to match this position
var avatarToWorldPointTest_sphereEntity;
function avatarToWorldPointTest() {
   var jointIndex = MyAvatar.getJointIndex("LeftHandPinky4");
   var jointOffset_WorldSpace = { x: 0.1, y: 0, z: 0 };
   var worldPos = MyAvatar.avatarToWorldPoint(jointOffset_WorldSpace, jointIndex);
       
   var jointSphereProps = Object.create(debugSphereBaseProperties);
   jointSphereProps.name = "avatarToWorldPointTest_Sphere";
   jointSphereProps.color =  { blue: 240, green: 150, red: 150 };
   jointSphereProps.position = worldPos;
   avatarToWorldPointTest_sphereEntity = Entities.addEntity(jointSphereProps);
}
function avatarToWorldPointTest_update(deltaTime) {
   var jointIndex = MyAvatar.getJointIndex("LeftHandPinky4");
   var jointOffset_WorldSpace = { x: 0.1, y: 0, z: 0 };
   var worldPos = MyAvatar.avatarToWorldPoint(jointOffset_WorldSpace, jointIndex);
   var newProperties = { position: worldPos };   
   Entities.editEntity(avatarToWorldPointTest_sphereEntity, newProperties);
}

//avatarToWorldDirection
//	create line in world space
//	each frame calculate world space direction of players head z axis
//	update line to match
var avatarToWorldDirectionTest_lineEntity;
function avatarToWorldDirectionTest() {
   var jointIndex = MyAvatar.getJointIndex("Head");
   var avatarPos = MyAvatar.getJointPosition(jointIndex);

   var jointDir = { x: 1, y: 0, z: 1 };
   var worldDir = MyAvatar.avatarToWorldDirection(jointDir, jointIndex);
   print(worldDir.x);
   print(worldDir.y);
   print(worldDir.z);
   avatarToWorldDirectionTest_lineEntity = Entities.addEntity({
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
function avatarToWorldDirection_update(deltaTime) {
   var jointIndex = MyAvatar.getJointIndex("Head");
   var avatarPos = MyAvatar.getJointPosition(jointIndex);
   var jointDir = { x: 1, y: 0, z: 0 };
   var worldDir = MyAvatar.avatarToWorldDirection(jointDir, jointIndex);
   var newProperties = {
      linePoints: [{
        x: 0,
        y: 0,
        z: 0
      }, worldDir
      ],
      position : avatarPos
   };
   
   Entities.editEntity(avatarToWorldDirectionTest_lineEntity, newProperties);
}

//avatarToWorldRotation
//	create box in world space
//	each frame calculate world space rotation of players head
//	update box rotation to match	
var avatarToWorldRotationTest_boxEntity;
function avatarToWorldRotationTest() {
   var jointIndex = MyAvatar.getJointIndex("Head");
   var jointPosition_WorldSpace =  MyAvatar.getJointPosition(jointIndex);
   var jointRot = MyAvatar.getJointRotation(jointIndex);
   var jointRot_WorldSpace = MyAvatar.avatarToWorldRotation(jointRot, jointIndex);
       
   var boxProps = Object.create(debugBoxBaseProperties);
   boxProps.name = "avatarToWorldRotationTest_Box";
   boxProps.color =  { blue: 250, green: 250, red: 250 };
   boxProps.position = jointPosition_WorldSpace;
   boxProps.rotation = jointRot_WorldSpace;
   avatarToWorldRotationTest_boxEntity = Entities.addEntity(boxProps);
}
function avatarToWorldRotationTest_update(deltaTime) {
   var jointIndex = MyAvatar.getJointIndex("Head");
   var jointPosition_WorldSpace =  MyAvatar.getJointPosition(jointIndex);
   var jointRot = MyAvatar.getJointRotation(jointIndex);
   var jointRot_WorldSpace = MyAvatar.avatarToWorldRotation(jointRot, jointIndex);
   var newProperties = { position: jointPosition_WorldSpace, rotation: jointRot_WorldSpace };   
   Entities.editEntity(avatarToWorldRotationTest_boxEntity, newProperties);
}

avatarToWorldPointTest();
Script.update.connect(avatarToWorldPointTest_update);

avatarToWorldDirectionTest();
Script.update.connect(avatarToWorldDirection_update);

avatarToWorldRotationTest();
Script.update.connect(avatarToWorldRotationTest_update);
