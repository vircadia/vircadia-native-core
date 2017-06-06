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

//worldToJointPoint
//	calculate position offset from joint using getJointPosition
//	pass through worldToJointPoint to get offset in joint space of players joint
//	create a blue sphere and attach it to players joint using the joint space offset
// The two spheres should appear in the same place, but the blue sphere will follow the avatar
function worldToJointPointTest() {
   var jointIndex = MyAvatar.getJointIndex("LeftHandPinky4");
   var avatarPos = MyAvatar.position;

   var jointPosition_WorldSpace =  MyAvatar.getJointPosition(jointIndex);
   var jointOffset_WorldSpace = { x: 0.1, y: 0, z: 0 };
   var jointPosition_WorldSpaceOffset = Vec3.sum(jointPosition_WorldSpace, jointOffset_WorldSpace);
   var jointPosition_JointSpaceOffset = MyAvatar.worldToJointPoint(jointPosition_WorldSpaceOffset, jointIndex);
       
   var jointSphereProps = Object.create(debugSphereBaseProperties);
   jointSphereProps.name = "worldToJointPointTest_Joint";
   jointSphereProps.color =  { blue: 240, green: 150, red: 150 };
   jointSphereProps.localPosition = jointPosition_JointSpaceOffset;
   jointSphereProps.parentID = AVATAR_SELF_ID;           
   jointSphereProps.parentJointIndex = jointIndex;
   Entities.addEntity(jointSphereProps);
   
   var worldSphereProps = Object.create(debugSphereBaseProperties);
   worldSphereProps.name = "worldToJointPointTest_World";
   worldSphereProps.color =  { blue: 150, green: 250, red: 150 };
   worldSphereProps.position = jointPosition_WorldSpaceOffset;
   Entities.addEntity(worldSphereProps);
}

//worldToJointDirection
//	create line and attach to avatars head
//	each frame calculate direction of world x axis in joint space of players head
//	update arrow orientation to match
var worldToJointDirectionTest_lineEntity;
function worldToJointDirectionTest() {
   var jointIndex = MyAvatar.getJointIndex("Head");

   var jointPosition_WorldSpace =  MyAvatar.getJointPosition(jointIndex);
   var jointOffset_WorldSpace = { x: 0, y: 0, z: 0 };
   var jointPosition_WorldSpaceOffset = Vec3.sum(jointPosition_WorldSpace, jointOffset_WorldSpace);
   var jointPosition_JointSpaceOffset = MyAvatar.worldToJointPoint(jointPosition_WorldSpaceOffset, jointIndex);

   var worldDir = { x: 1, y: 0, z: 0 };
   var avatarDir = MyAvatar.worldToJointDirection(worldDir, jointIndex);
   
   worldToJointDirectionTest_lineEntity = Entities.addEntity({
      type: "Line",
      color: {red: 200, green: 250, blue: 0},
      dimensions: {x: 5, y: 5, z: 5},
      lifetime: 10.0,
      linePoints: [{
        x: 0,
        y: 0,
        z: 0
       }, avatarDir
       ],
      localPosition : jointOffset_WorldSpace,
      parentID : AVATAR_SELF_ID,         
      parentJointIndex : jointIndex      
   });
}

function worldToJointDirectionTest_update(deltaTime) {  
   var jointIndex = MyAvatar.getJointIndex("Head");
   var worldDir = { x: 1, y: 0, z: 0 };
   var avatarDir = MyAvatar.worldToJointDirection(worldDir, jointIndex);
   var newProperties = { linePoints: [{
        x: 0,
        y: 0,
        z: 0
       }, avatarDir
       ]};
   
   Entities.editEntity(worldToJointDirectionTest_lineEntity, newProperties);
}

//worldToJointRotation
//	create box and parent to some player joint
//	convert world identity rotation to joint space rotation
//	each frame, update box with new orientation
var worldToJointRotationTest_boxEntity;
function worldToJointRotationTest() {
   var jointIndex = MyAvatar.getJointIndex("RightHandPinky4");
   var avatarPos = MyAvatar.position;

   var jointPosition_WorldSpace =  MyAvatar.getJointPosition(jointIndex);
   var jointOffset_WorldSpace = { x: 0.0, y: 0, z: 0 };
   var jointPosition_WorldSpaceOffset = Vec3.sum(jointPosition_WorldSpace, jointOffset_WorldSpace);
   var jointPosition_JointSpaceOffset = MyAvatar.worldToJointPoint(jointPosition_WorldSpaceOffset, jointIndex);
       
   var jointBoxProps = Object.create(debugBoxBaseProperties);
   jointBoxProps.name = "worldToJointRotationTest_Box";
   jointBoxProps.color =  { blue: 0, green: 0, red: 250 };
   jointBoxProps.localPosition = jointPosition_JointSpaceOffset;
   jointBoxProps.parentID = AVATAR_SELF_ID;           
   jointBoxProps.parentJointIndex = jointIndex;
   worldToJointRotationTest_boxEntity = Entities.addEntity(jointBoxProps);
}
function worldToJointRotationTest_update(deltaTime) {
   var jointIndex = MyAvatar.getJointIndex("RightHandPinky4");
   var worldRot = Quat.fromPitchYawRollDegrees(0,0,0);
   var avatarRot = MyAvatar.worldToJointRotation(worldRot, jointIndex);
   var newProperties = { localRotation: avatarRot };   
   Entities.editEntity(worldToJointRotationTest_boxEntity, newProperties);
}

worldToJointPointTest();
worldToJointDirectionTest();
worldToJointRotationTest();

Script.update.connect(worldToJointDirectionTest_update);
Script.update.connect(worldToJointRotationTest_update);
