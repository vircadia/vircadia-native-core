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

  //  var qoffset = Quat.angleAxis( -90, {x:0, y:1, z:0});
  //  wori = Quat.multiply( wori, qoffset );

    return wori;a
}

function controlerToSkeletonOri( jointName, isRightSide, event ) {
 
  //  var qrootoffset = Quat.angleAxis( -180, {x:0, y:1, z:0});
  //  var qoffset = Quat.angleAxis( -( 2 * isRightSide - 1) * 90, {x:0, y:1, z:0});



 //  return Quat.multiply( qrootoffset, Quat.multiply( crot, qoffset ) );
  // return Quat.multiply( crot, qoffset );
   // return Quat.multiply( qrootoffset, crot );
 // return ( crot );

  //return MyAvatar.getJointRotation( jointName );
  //return Quat.fromPitchYawRollDegrees(0,0,30);

  var qx = Quat.angleAxis( -90, {x:1, y:0, z:0});
  var qy = Quat.angleAxis( 180, {x:0, y:1, z:0});
  var q = Quat.multiply( qy, qx );

  //  return q;

 // return  Quat.multiply( event.locRotation, q );
    var cq = jointControllers[0].c.getAbsRotation();
  //  return cq;
    //var q = spatialEvent.absRotation;

   // MyAvatar.clearJointData( jointName );    
    var qjointRef = MyAvatar.getJointRotation( jointName );
    print( jointName + " " + quatToString( qjointRef ) );

        var qjointRefI = Quat.inverse( qjointRef );
        var qjoint = Quat.multiply( cq, qjointRefI );


    print( quatToString( cq )  );

    return  qjoint;
}


var jointParticles = [];
function updateJointParticle( joint, pos, ori, look ) {
 /*   print( "debug 1" );
    var jointID = jointParticles[ joint ];
    if ( jointID == null ) {
        print( "debug create " + joint );
*/
        var radius = 0.005* look.r;
        var ballProperties = {
            position: pos,
            velocity: { x: 0, y: 0, z: 0},
            gravity: { x: 0, y: 0, z: 0 },
            damping: 0, 
            radius : radius,
            color: look.c,
            lifetime: 0.1
        };
        var atomPos = Particles.addParticle(ballProperties);
/* 
        // Zaxis
        var Zaxis = Vec3.multiply( Quat.getFront( ori ), - 1.5 * radius ) ;
        ballProperties.position = Vec3.sum(pos, Zaxis );
        ballProperties.radius = 0.35* radius;
        ballProperties.color= { red: 255, green: 255, blue: 255 };

        var atomZ = Particles.addParticle(ballProperties);

        var up = Vec3.multiply( Quat.getUp( ori ), 1.5 * radius ) ;
        ballProperties.position = Vec3.sum(pos, up) ;
        ballProperties.radius = 0.35* radius;
        ballProperties.color= { red: 0, green: 255, blue: 0 };

        var atomY = Particles.addParticle(ballProperties);

       var right = Vec3.multiply( Quat.getRight( ori ), 1.5 * radius ) ;
        ballProperties.position = Vec3.sum(pos, right) ;
        ballProperties.radius = 0.35* radius;
        ballProperties.color= { red: 255, green: 0, blue: 225 };

        var atomX = Particles.addParticle(ballProperties);
*/
    //    jointParticles[ joint ] = { p: atomPos, x: atomX, y: atomY, z: atomZ };
/*
    } else {
        //print( "debug update " + joint );

        var p = Particles.getParticleProperties( jointID.p );
        p.position = pos;
       // p.lifetime = 1.0;
        Particles.editParticle( jointID.p, p );


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
    { n: "LeftHand",        l: evalArmBoneLook( 0, 0) },
    
    { n: "LeftHandThumb2",  l: evalFingerBoneLook( 0, 1, 2) },
    { n: "LeftHandThumb3",  l: evalFingerBoneLook( 0, 1, 3) },
    { n: "LeftHandThumb4",  l: evalFingerBoneLook( 0, 1, 4) },

/*    { n: "LeftHandIndex1",  l: evalFingerBoneLook( 0, 2, 1) },
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
*/
    { n: "RightHand",        l: evalArmBoneLook( 1, 0) },
/*
    { n: "RightHandThumb2",  l: evalFingerBoneLook( 1, 1, 2) },
    { n: "RightHandThumb3",  l: evalFingerBoneLook( 1, 1, 3) },
    { n: "RightHandThumb4",  l: evalFingerBoneLook( 1, 1, 4) },
*/
    { n: "RightHandIndex1",  l: evalFingerBoneLook( 1, 2, 1) },
    { n: "RightHandIndex2",  l: evalFingerBoneLook( 1, 2, 2) },
    { n: "RightHandIndex3",  l: evalFingerBoneLook( 1, 2, 3) },
    { n: "RightHandIndex4",  l: evalFingerBoneLook( 1, 2, 4) },

/*    { n: "RightHandMiddle1",  l: evalFingerBoneLook( 1, 3, 1) },
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
    */
 ];

function onSpatialEventHandler( jointName, look ) {
    var _jointName = jointName;
    var _look = look;
    var _side = look.side;
    return (function( spatialEvent ) {

        MyAvatar.setJointData(_jointName, controlerToSkeletonOri( _jointName, _side, spatialEvent ));

        updateJointParticle(_jointName,
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

    // simple test
   // MyAvatar.setJointData("LeftArm", jointControllers[6].c.getLocRotation());
   // MyAvatar.setJointData("LeftForeArm", jointControllers[7].c.getLocRotation());
 
});

Script.scriptEnding.connect(function() {
});
