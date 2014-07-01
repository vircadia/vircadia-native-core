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
  /*  var dataString = label + " " +
         vec3ToString( spatialEvent.locTranslation ) + " " +
         quatToString( spatialEvent.locRotation ) + " " +
         vec3ToString( spatialEvent.absTranslation ) + " " +
         quatToString( spatialEvent.absRotation );
  //  print( dataString )
  */;
}

function avatarToWorld( apos ) {

    // apply offset ?
    var offset = { x: 0, y: 0.5, z: -0.5 };   
    var lpos = Vec3.sum(apos, offset);

    var wpos = Vec3.sum( MyAvatar.position , Vec3.multiplyQbyV(MyAvatar.orientation, lpos) );

    return wpos;
}

var jointParticles = [];
function updateJointParticle( joint, pos, look ) {
  /*  print( "debug 1" );
    var jointID = jointParticles[ joint ];
    if ( jointID == null ) {
        print( "debug create " + joint );
*/
        var ballProperties = {
            position: pos,
            velocity: { x: 0, y: 0, z: 0},
            gravity: { x: 0, y: 0, z: 0 },
            damping: 0, 
            radius : 0.005* look.r,
            color: look.c,
            lifetime: 0.05
        };
        jointParticles[ joint ] = Particles.addParticle(ballProperties);
 /*   } else {
        print( "debug update " + joint );
        var prop = Particles.getParticleProperties( jointID );
        prop.position = pos;
        prop.lifetime = 1.0;
        Particles.editParticle( jointID, prop );
    }*/
}

function evalFingerBoneLook( isRightSide, finger, bone ) {
    return { c: { red: (255 * ( 1 - isRightSide )),
                  green: 255 * ( ((bone - 1)) / 3 ),
                  blue: (255 * isRightSide) },
             r: (5 + (5 - (finger-1))) / 10.0  };
}

var leapJoints = [
    { n: "LeftHand",        l: { c: { red: 255, green: 0, blue: 0 },     r: 3 } },
    
    { n: "LeftHandThumb2",  l: evalFingerBoneLook( 0, 1, 2) },
    { n: "LeftHandThumb3",  l: evalFingerBoneLook( 0, 1, 3) },
    { n: "LeftHandThumb4",  l: evalFingerBoneLook( 0, 1, 4) },

    { n: "LeftHandIndex1",  l: evalFingerBoneLook( 0, 2, 1) },
    { n: "LeftHandIndex2",  l: evalFingerBoneLook( 0, 2, 2) },
    { n: "LeftHandIndex3",  l: evalFingerBoneLook( 0, 2, 3) },
    { n: "LeftHandIndex4",  l: evalFingerBoneLook( 0, 2, 4) },
    
    { n: "LeftHandMiddle1",  l: evalFingerBoneLook( 0, 3, 1) },
    { n: "LeftHandMiddle2",  l: evalFingerBoneLook( 0, 3, 2) },
    { n: "LeftHandMiddle3",  l: evalFingerBoneLook( 0, 3, 3) },
    { n: "LeftHandMiddle4",  l: evalFingerBoneLook( 0, 3, 4) },

    { n: "LeftHandRing1",  l: evalFingerBoneLook( 0, 4, 1) },
    { n: "LeftHandRing2",  l: evalFingerBoneLook( 0, 4, 2) },
    { n: "LeftHandRing3",  l: evalFingerBoneLook( 0, 4, 3) },
    { n: "LeftHandRing4",  l: evalFingerBoneLook( 0, 4, 4) },

    { n: "LeftHandPinky1",  l: evalFingerBoneLook( 0, 5, 1) },
    { n: "LeftHandPinky2",  l: evalFingerBoneLook( 0, 5, 2) },
    { n: "LeftHandPinky3",  l: evalFingerBoneLook( 0, 5, 3) },
    { n: "LeftHandPinky4",  l: evalFingerBoneLook( 0, 5, 4) },   

    { n: "RightHand",        l: { c: { red: 0, green: 0, blue: 255 }, r: 3 } },

    { n: "RightHandThumb2",  l: evalFingerBoneLook( 1, 1, 2) },
    { n: "RightHandThumb3",  l: evalFingerBoneLook( 1, 1, 3) },
    { n: "RightHandThumb4",  l: evalFingerBoneLook( 1, 1, 4) },

    { n: "RightHandIndex1",  l: evalFingerBoneLook( 1, 2, 1) },
    { n: "RightHandIndex2",  l: evalFingerBoneLook( 1, 2, 2) },
    { n: "RightHandIndex3",  l: evalFingerBoneLook( 1, 2, 3) },
    { n: "RightHandIndex4",  l: evalFingerBoneLook( 1, 2, 4) },

    { n: "RightHandMiddle1",  l: evalFingerBoneLook( 1, 3, 1) },
    { n: "RightHandMiddle2",  l: evalFingerBoneLook( 1, 3, 2) },
    { n: "RightHandMiddle3",  l: evalFingerBoneLook( 1, 3, 3) },
    { n: "RightHandMiddle4",  l: evalFingerBoneLook( 1, 3, 4) },

    { n: "RightHandRing1",  l: evalFingerBoneLook( 1, 4, 1) },
    { n: "RightHandRing2",  l: evalFingerBoneLook( 1, 4, 2) },
    { n: "RightHandRing3",  l: evalFingerBoneLook( 1, 4, 3) },
    { n: "RightHandRing4",  l: evalFingerBoneLook( 1, 4, 4) },

    { n: "RightHandPinky1",  l: evalFingerBoneLook( 1, 5, 1) },
    { n: "RightHandPinky2",  l: evalFingerBoneLook( 1, 5, 2) },
    { n: "RightHandPinky3",  l: evalFingerBoneLook( 1, 5, 3) },
    { n: "RightHandPinky4",  l: evalFingerBoneLook( 1, 5, 4) }, 

 ];

function onSpatialEventHandler( jointName, look ) {
    var _jointName = jointName;
    var _look = look;
    return (function( spatialEvent ) {
        MyAvatar.setJointData(_jointName, spatialEvent.absRotation);
        updateJointParticle(_jointName, avatarToWorld( spatialEvent.absTranslation ), _look );
        printSpatialEvent(_jointName, spatialEvent );
    });
}

var isPullingSpatialData = true;

var jointControllers = [];
for ( i in leapJoints ) {

    print( leapJoints[i].n );
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
                                     absRotation: jointControllers[i].c.getAbsRotation() };
                jointControllers[i].h( spatialEvent );
            }
        }
    }
 
});

Script.scriptEnding.connect(function() {
});
