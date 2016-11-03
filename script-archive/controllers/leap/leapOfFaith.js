//
//  leapOfFaith.js
//  examples
//
//  Created by Sam Cake on 6/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var jointList = MyAvatar.getJointNames();
var jointMappings = "\n# Joint list start";
for (var i = 0; i < jointList.length; i++) {
    jointMappings = jointMappings + "\njointIndex = " + jointList[i] + " = " + i;
}
print(jointMappings + "\n# Joint list end");  

function vec3ToString( v ) {
    return ("(" + v.x +", " + v.y + ", " + v.z + ")" ); 
}
function quatToString( q ) {
    return ("(" + q.x +", " + q.y + ", " + q.z + ", " + q.w + ")" ); 
}

function printSpatialEvent( label, spatialEvent ) {
    if ( false ){//label == "RightHandIndex1" ) {
        var dataString = label + " " +
             /*vec3ToString( spatialEvent.locTranslation ) + " " +
             quatToString( spatialEvent.locRotation ) + " " +*/
             vec3ToString( spatialEvent.absTranslation ) + " " +
             quatToString( spatialEvent.absRotation );
        print( dataString );
    }
}

function avatarToWorldPos( apos ) {

    // apply offset ?
    var offset = { x: 0, y: 0.5, z: -0.5 };   
    var lpos = Vec3.sum(apos, offset);

    var wpos = Vec3.sum( MyAvatar.position , Vec3.multiplyQbyV(MyAvatar.orientation, lpos) );

    return wpos;
}

function avatarToWorldQuat( aori) {

    var wori = Quat.multiply(MyAvatar.orientation, aori);
    return wori;
}

function controlerToSkeletonOri( jointName, isRightSide, event ) {
 
    var qAvatarRootOffset = Quat.angleAxis( -180, {x:0, y:1, z:0});
    var qAxisOffset = Quat.angleAxis( -( 2 * isRightSide - 1) * 90, {x:0, y:1, z:0});
    var qAbsJoint = event.absRotation;


   return Quat.multiply( qAvatarRootOffset, Quat.multiply( qAbsJoint, qAxisOffset ) );
}


var jointShperes = [];
function updateJointShpere( joint, pos, ori, look ) {
 /*   print( "debug 1" );
    var jointID = jointShperes[ joint ];
    if ( jointID == null ) {
        print( "debug create " + joint );
*/
        var radius = 0.005* look.r;
        var ballProperties = {
            type: "Sphere",
            position: pos,
            velocity: { x: 0, y: 0, z: 0},
            gravity: { x: 0, y: 0, z: 0 },
            damping: 0, 
            dimensions: { x: radius, y: radius, z: radius },
            color: look.c,
            lifetime: 0.05
        };
        var atomPos = Entities.addEntity(ballProperties);
 
/*        // Zaxis
        var Zaxis = Vec3.multiply( Quat.getFront( ori ), - 1.5 * radius ) ;
        ballProperties.position = Vec3.sum(pos, Zaxis );
        ballProperties.radius = 0.35* radius;
        ballProperties.color= { red: 255, green: 255, blue: 255 };

        var atomZ = Entities.addEntity(ballProperties);

        var up = Vec3.multiply( Quat.getUp( ori ), 1.5 * radius ) ;
        ballProperties.position = Vec3.sum(pos, up) ;
        ballProperties.dimensions = { x: 0.35* radius, y: 0.35* radius, z: 0.35* radius };
        ballProperties.color= { red: 0, green: 255, blue: 0 };

        var atomY = Entities.addEntity(ballProperties);

       var right = Vec3.multiply( Quat.getRight( ori ), 1.5 * radius ) ;
        ballProperties.position = Vec3.sum(pos, right) ;
        ballProperties.dimensions = { x: 0.35* radius, y: 0.35* radius, z: 0.35* radius };
        ballProperties.color= { red: 255, green: 0, blue: 225 };

        var atomX = Entities.addEntity(ballProperties);
*/
    //    jointShperes[ joint ] = { p: atomPos, x: atomX, y: atomY, z: atomZ };
/*
    } else {
        //print( "debug update " + joint );

        var p = Entities.getEntityProperties( jointID.p );
        p.position = pos;
       // p.lifetime = 1.0;
        Entities.editEntity( jointID.p, p );


    }*/
}

function evalArmBoneLook( isRightSide, bone ) {
    return { c: { red: (255 * ( 1 - isRightSide )),
                  green: 255 * ( ((bone)) / 2 ),
                  blue: (255 * isRightSide) },
             r: 3 ,
             side: isRightSide };
}

function evalFingerBoneLook( isRightSide, finger, bone ) {
    return { c: { red: (255 * ( 1 - isRightSide )),
                  green: 255 * ( ((bone - 1)) / 3 ),
                  blue: (255 * isRightSide) },
             r: (5 + (5 - (finger-1))) / 10.0,
             side: isRightSide };
}

var leapJoints = [

    { n: "joint_L_elbow",        l: evalArmBoneLook( 0, 2) },
    { n: "joint_L_hand",        l: evalArmBoneLook( 0, 1) },
    { n: "joint_L_wrist",        l: evalArmBoneLook( 0, 0) },
    
    { n: "joint_L_thumb2",  l: evalFingerBoneLook( 0, 1, 2) },
    { n: "joint_L_thumb3",  l: evalFingerBoneLook( 0, 1, 3) },
    { n: "joint_L_thumb4",  l: evalFingerBoneLook( 0, 1, 4) },

    { n: "joint_L_index1",  l: evalFingerBoneLook( 0, 2, 1) },
    { n: "joint_L_index2",  l: evalFingerBoneLook( 0, 2, 2) },
    { n: "joint_L_index3",  l: evalFingerBoneLook( 0, 2, 3) },
    { n: "joint_L_index4",  l: evalFingerBoneLook( 0, 2, 4) },
    
    { n: "joint_L_middle1",  l: evalFingerBoneLook( 0, 3, 1) },
    { n: "joint_L_middle2",  l: evalFingerBoneLook( 0, 3, 2) },
    { n: "joint_L_middle3",  l: evalFingerBoneLook( 0, 3, 3) },
    { n: "joint_L_middle4",  l: evalFingerBoneLook( 0, 3, 4) },

    { n: "joint_L_ring1",  l: evalFingerBoneLook( 0, 4, 1) },
    { n: "joint_L_ring2",  l: evalFingerBoneLook( 0, 4, 2) },
    { n: "joint_L_ring3",  l: evalFingerBoneLook( 0, 4, 3) },
    { n: "joint_L_ring4",  l: evalFingerBoneLook( 0, 4, 4) },

    { n: "joint_L_pinky1",  l: evalFingerBoneLook( 0, 5, 1) },
    { n: "joint_L_pinky2",  l: evalFingerBoneLook( 0, 5, 2) },
    { n: "joint_L_pinky3",  l: evalFingerBoneLook( 0, 5, 3) },
    { n: "joint_L_pinky4",  l: evalFingerBoneLook( 0, 5, 4) },   

    { n: "joint_R_elbow",        l: evalArmBoneLook( 1, 2) },
    { n: "joint_R_hand",        l: evalArmBoneLook( 1, 1) },   
    { n: "joint_R_wrist",        l: evalArmBoneLook( 1, 0) },
    
    { n: "joint_R_thumb2",  l: evalFingerBoneLook( 1, 1, 2) },
    { n: "joint_R_thumb3",  l: evalFingerBoneLook( 1, 1, 3) },
    { n: "joint_R_thumb4",  l: evalFingerBoneLook( 1, 1, 4) },

    { n: "joint_R_index1",  l: evalFingerBoneLook( 1, 2, 1) },
    { n: "joint_R_index2",  l: evalFingerBoneLook( 1, 2, 2) },
    { n: "joint_R_index3",  l: evalFingerBoneLook( 1, 2, 3) },
    { n: "joint_R_index4",  l: evalFingerBoneLook( 1, 2, 4) },
    
    { n: "joint_R_middle1",  l: evalFingerBoneLook( 1, 3, 1) },
    { n: "joint_R_middle2",  l: evalFingerBoneLook( 1, 3, 2) },
    { n: "joint_R_middle3",  l: evalFingerBoneLook( 1, 3, 3) },
    { n: "joint_R_middle4",  l: evalFingerBoneLook( 1, 3, 4) },

    { n: "joint_R_ring1",  l: evalFingerBoneLook( 1, 4, 1) },
    { n: "joint_R_ring2",  l: evalFingerBoneLook( 1, 4, 2) },
    { n: "joint_R_ring3",  l: evalFingerBoneLook( 1, 4, 3) },
    { n: "joint_R_ring4",  l: evalFingerBoneLook( 1, 4, 4) },

    { n: "joint_R_pinky1",  l: evalFingerBoneLook( 1, 5, 1) },
    { n: "joint_R_pinky2",  l: evalFingerBoneLook( 1, 5, 2) },
    { n: "joint_R_pinky3",  l: evalFingerBoneLook( 1, 5, 3) },
    { n: "joint_R_pinky4",  l: evalFingerBoneLook( 1, 5, 4) },   

 ];

function onSpatialEventHandler( jointName, look ) {
    var _jointName = jointName;
    var _look = look;
    var _side = look.side;
    return (function( spatialEvent ) {

        // THis should be the call to update the skeleton joint from 
        // the absolute joint transform coming from the Leap controller
        // WE need a new method on MyAvatar which will ultimately call
        // state.setRotationFromBindFrame(rotation, priority)  in 
        // Avatar::simulate=>void Model::setJointState(int index, bool valid, const glm::quat& rotation, float priority)
      
        // MyAvatar.setJointRotationFromBindSpace(_jointName, controlerToSkeletonOri( _jointName, _side, spatialEvent ));


        updateJointShpere(_jointName,
            avatarToWorldPos( spatialEvent.absTranslation ),
            avatarToWorldQuat( spatialEvent.absRotation ),
            _look );       
        printSpatialEvent(_jointName, spatialEvent );
    });
}

var isPullingSpatialData = true;

var jointControllers = [];
for ( i in leapJoints ) {

    print( leapJoints[i].n );
    // In the current implementation, the LEapmotion is the only "Spatial" device
    // Each jointTracker is retreived from the joint name following the rigging convention
    var controller = Controller.createInputController( "Spatial", leapJoints[i].n );
    var handler = onSpatialEventHandler( leapJoints[i].n, leapJoints[i].l );
    jointControllers.push( { c: controller, h: handler } );

    if ( ! isPullingSpatialData ) {
        controller.spatialEvent.connect( handler );  
    }
}


Script.update.connect(function(deltaTime) {

    if ( isPullingSpatialData )
    {
        for ( i in jointControllers ) {
            if ( jointControllers[i].c.isActive() ) {
                var spatialEvent = { absTranslation: jointControllers[i].c.getAbsTranslation(),
                                     absRotation: jointControllers[i].c.getAbsRotation(),
                                     locTranslation: jointControllers[i].c.getLocTranslation(),
                                     locRotation: jointControllers[i].c.getLocRotation() };
                jointControllers[i].h( spatialEvent );
            }
        }
    } 
});

Script.scriptEnding.connect(function() {
    var i;
    for (i = 0; i < jointControllers.length; i += 1) {
        Controller.releaseInputController(jointControllers[i].c);
    }
});
