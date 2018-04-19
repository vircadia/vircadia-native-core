
/* global Script, Vec3, MyAvatar Tablet Messages Quat DebugDraw Mat4 Xform*/


Script.include("/~/system/libraries/Xform.js");

var MESSAGE_CHANNEL = "Hifi-Step-Cg"; 

var ANIM_VARS = [
    "isTalking",
    "isNotMoving",
    "isMovingForward",
    "isMovingBackward",
    "isMovingRight",
    "isMovingLeft",
    "isTurningRight",
    "isTurningLeft",
    "isFlying",
    "isTakeoffStand",
    "isTakeoffRun",
    "isInAirStand",
    "isInAirRun",
    "hipsPosition",
    "hipsRotation",
    "hipsType",
    "headWeight",
    "headType"
];

var DEBUGDRAWING;
var YELLOW;
var BLUE;
var GREEN;
var RED;

var ROT_Y90;
var ROT_Y180;
var FLOOR_Y;
var IDENT_QUAT;

var TABLET_BUTTON_NAME;
var RECENTER;
var JOINT_MASSES;

var hipsUnderHead;

var armsHipRotation;
var hipsPosition;
var filteredHipsPosition;
var hipsRotation;

var jointList;
var rightFootName;
var leftFootName;
var rightToeName;
var leftToeName;
var leftToeEnd;
var rightToeEnd;
var leftFoot;
var rightFoot;
var base;

var clampFront;
var clampBack;
var clampLeft;
var clampRight;

var tablet;
var tabletButton;

function initCg() {

    DEBUGDRAWING = false;

    YELLOW = { r: 1, g: 1, b: 0, a: 1 };
    BLUE = { r: 0, g: 0, b: 1, a: 1 };
    GREEN = { r: 0, g: 1, b: 0, a: 1 };
    RED = { r: 1, g: 0, b: 0, a: 1 };

    ROT_Y90 = { x: 0, y: 0.7071067811865475, z: 0, w: 0.7071067811865476 };
    ROT_Y180 = { x: 0, y: 1, z: 0, w: 0 };
    FLOOR_Y = -0.9;
    IDENT_QUAT = { x: 0, y: 0, z: 0, w: 1 };

    JOINT_MASSES = [{ joint: "Head", mass: 20.0, pos: { x: 0, y: 0, z: 0 } },
                    { joint: "LeftHand", mass: 2.0, pos: { x: 0, y: 0, z: 0 } },
                    { joint: "RightHand", mass: 2.0, pos: { x: 0, y: 0, z: 0 } }];

    TABLET_BUTTON_NAME = "CG";
    RECENTER = false;

    MyAvatar.hmdLeanRecenterEnabled = RECENTER;
    hipsUnderHead;

    armsHipRotation = { x: 0, y: 1, z: 0, w: 0 };
    hipsPosition = MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(MyAvatar.getJointIndex("Hips"));
    filteredHipsPosition = MyAvatar.position;
    hipsRotation = { x: 0, y: 0, z: 0, w: 1 };

    jointList = MyAvatar.getJointNames();
    //    print(JSON.stringify(jointList));

    rightFootName = null;
    leftFootName = null;
    rightToeName = null;
    leftToeName = null;
    leftToeEnd = null;
    rightToeEnd = null;
    leftFoot;
    rightFoot;

    clampFront = -0.10;
    clampBack = 0.17;
    clampLeft = -0.50;
    clampRight = 0.50;

    getFeetAndToeNames();
    base = computeBase();
    mirrorPoints();


    tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    tabletButton = tablet.addButton({
        text: TABLET_BUTTON_NAME,
        icon: "icons/tablet-icons/avatar-record-i.svg"
    });

    tabletButton.clicked.connect(function () {
        print("recenter is: " + RECENTER);
        MyAvatar.hmdLeanRecenterEnabled = RECENTER;
        RECENTER = !RECENTER;

        //  messageSend("clicked button in cg");
    });


    var handlerId = MyAvatar.addAnimationStateHandler(function (props) {

        var result = {};

        //  prevent animations from ever leaving the idle state
        result.isTalking = false;
        result.isFlying = false;
        result.isTakeoffStand = false;
        result.isTakeoffRun = false;
        result.isInAirStand = false;
        result.isInAirRun = false;
        result.hipsPosition = hipsPosition;
        result.hipsRotation = hipsRotation;
        result.hipsType = 0;
        result.headWeight = 4;
        result.headType = 4;

        return result;
    }, ANIM_VARS);

    Messages.subscribe(MESSAGE_CHANNEL);
    Messages.messageReceived.connect(messageHandler);
    Script.update.connect(update);
    MyAvatar.skeletonChanged.connect(function () {
        Script.setTimeout(function () {
            // stop logic if needed
            MyAvatar.clearJointsData();
            // reset logic
        }, 200);
    });
    HMD.displayModeChanged.connect(function () {
        Script.setTimeout(function () {
            // stop logic if needed
            MyAvatar.clearJointsData();
            // reset logic
        }, 200);
    });


}

function messageSend(message) {
    Messages.sendLocalMessage(MESSAGE_CHANNEL, message);
}

function messageHandler(channel, messageString, senderID) {
    if (channel !== MESSAGE_CHANNEL) {
        return;
    }
    
    var hipquat = JSON.parse(messageString);
    armsHipRotation = Quat.multiply(ROT_Y180,hipquat);
    
}

function getFeetAndToeNames() {

    for (var i = 0; i < jointList.length; i++) {
        if ((jointList[i].indexOf('Right') !== -1) && (jointList[i].indexOf('Foot') !== -1)) {
            print(JSON.stringify(jointList[i]));
            rightFootName = jointList[i];
        }
        if ((jointList[i].indexOf('Left') !== -1) && (jointList[i].indexOf('Foot') !== -1)) {
            print(JSON.stringify(jointList[i]));
            leftFootName = jointList[i];
        }
        if ((jointList[i].indexOf('Right') !== -1) && (jointList[i].indexOf('Toe') !== -1) && (jointList[i].indexOf('End') !== -1)) {
            print(JSON.stringify(jointList[i]));
            rightToeName = jointList[i];
        }
        if ((jointList[i].indexOf('Left') !== -1) && (jointList[i].indexOf('Toe') !== -1) && (jointList[i].indexOf('End') !== -1)) {
            print(JSON.stringify(jointList[i]));
            leftToeName = jointList[i];
        }
    }
}

function computeBase() {
    
    if (rightFootName === null || leftFootName === null) {
        //  if the feet names aren't found then use our best guess of the base.
        leftToeEnd = {x: 0.12, y: 0.0, z: 0.12};
        rightToeEnd = {x: -0.18, y: 0.0, z: 0.12};
        leftFoot = {x: 0.15, y: 0.0, z: -0.17};
        rightFoot = {x: -0.20, y: 0.0, z: -0.17};
    } else {
        //  else we at least found the feet in the skeleton.
        var leftFootIndex = MyAvatar.getJointIndex(leftFootName);
        var rightFootIndex = MyAvatar.getJointIndex(rightFootName);
        var leftFoot = MyAvatar.getAbsoluteJointTranslationInObjectFrame(leftFootIndex);
        var rightFoot = MyAvatar.getAbsoluteJointTranslationInObjectFrame(rightFootIndex);
        
        if (rightToeName === null || leftToeName === null) {
            //  the toe ends were not found then we use a guess for the length and width of the feet. 
            leftToeEnd = {x: (leftFoot.x + 0.02), y: 0.0, z: (leftFoot.z - 0.2)};
            rightToeEnd = {x: (rightFoot.x - 0.02), y: 0.0, z: (rightFoot.z - 0.2)};
        } else {
            //  else we found the toe ends and now we can really compute the base.
            var leftToeIndex = MyAvatar.getJointIndex(leftToeName);
            var rightToeIndex = MyAvatar.getJointIndex(rightToeName);
            leftToeEnd = MyAvatar.getAbsoluteJointTranslationInObjectFrame(leftToeIndex);
            rightToeEnd = MyAvatar.getAbsoluteJointTranslationInObjectFrame(rightToeIndex);
        }
            
    }
        
    //  project each point into the FLOOR plane.
    var points = [{x: leftToeEnd.x, y: FLOOR_Y, z: leftToeEnd.z},
                  {x: rightToeEnd.x, y: FLOOR_Y, z: rightToeEnd.z},
                  {x: rightFoot.x, y: FLOOR_Y, z: rightFoot.z},
                  {x: leftFoot.x, y: FLOOR_Y, z: rightFoot.z}];

    //  compute normals for each plane
    var normal, normals = [];
    var n = points.length;
    var next, prev;
    for (next = 0, prev = n - 1; next < n; prev = next, next++) {
        normal = Vec3.multiplyQbyV(ROT_Y90, Vec3.normalize(Vec3.subtract(points[next], points[prev])));
        normals.push(normal);
    }

    var TOE_FORWARD_RADIUS = 0.01;
    var TOE_SIDE_RADIUS = 0.05;
    var HEEL_FORWARD_RADIUS = 0.01;
    var HEEL_SIDE_RADIUS = 0.03;
    var radii = [
        TOE_SIDE_RADIUS, TOE_FORWARD_RADIUS, TOE_FORWARD_RADIUS, TOE_SIDE_RADIUS,
        HEEL_SIDE_RADIUS, HEEL_FORWARD_RADIUS, HEEL_FORWARD_RADIUS, HEEL_SIDE_RADIUS
    ];

    //  subdivide base and extrude by the toe and heel radius.
    var newPoints = [];
    for (next = 0, prev = n - 1; next < n; prev = next, next++) {
        newPoints.push(Vec3.sum(points[next], Vec3.multiply(radii[2 * next], normals[next])));
        newPoints.push(Vec3.sum(points[next], Vec3.multiply(radii[(2 * next) + 1], normals[(next + 1) % n])));
    }

    //  compute newNormals
    var newNormals = [];
    n = newPoints.length;
    for (next = 0, prev = n - 1; next < n; prev = next, next++) {
        normal = Vec3.multiplyQbyV(ROT_Y90, Vec3.normalize(Vec3.subtract(newPoints[next], newPoints[prev])));
        newNormals.push(normal);
    }
        
    for (var j = 0;j<points.length;j++) {
        print(JSON.stringify(points[j]));
    }
    return {points: newPoints, normals: newNormals};

}

function mirrorPoints() {

    if (Math.abs(base.points[0].x) > Math.abs(base.points[3].x)) {
        base.points[3].x = -base.points[0].x;
        base.points[2].x = -base.points[1].x;
    } else {
        base.points[0].x = -base.points[3].x;
        base.points[1].x = -base.points[2].x;
    }

    if (Math.abs(base.points[4].x) > Math.abs(base.points[7].x)) {
        base.points[7].x = -base.points[4].x;
        base.points[6].x = -base.points[5].x;
    } else {
        base.points[4].x = -base.points[7].x;
        base.points[5].x = -base.points[6].x;
    }

    if (Math.abs(base.points[0].z) > Math.abs(base.points[0].z)) {
        base.points[3].z = base.points[0].z;
        base.points[2].z = base.points[1].z;
    } else {
        base.points[0].z = base.points[3].z;
        base.points[1].z = base.points[2].z;
    }

    if (Math.abs(base.points[4].z) > Math.abs(base.points[7].z)) {
        base.points[7].z = base.points[4].z;
        base.points[6].z = base.points[5].z;
    } else {
        base.points[4].z = base.points[7].z;
        base.points[5].z = base.points[6].z;
    }

    for (var i = 0; i < base.points.length; i++) {

        print("point: " + i + " " + JSON.stringify(base.points[i]));
    }
    for (var j = 0; j < base.normals.length; j++) {
        print("normal: " + j + " " + JSON.stringify(base.normals[j]));
    }
}


function drawBase(base) {
    //  transform corners into world space, for rendering.
    var xform = new Xform(MyAvatar.orientation, MyAvatar.position);
    var worldPoints = base.points.map(function (point) {
        return xform.xformPoint(point);
    });
    var worldNormals = base.normals.map(function (normal) {
        return xform.xformVector(normal);
    });

    var n = worldPoints.length;
    var next, prev;
    for (next = 0, prev = n - 1; next < n; prev = next, next++) {
        if (DEBUGDRAWING) {
            //  draw border
            DebugDraw.drawRay(worldPoints[prev], worldPoints[next], GREEN);
            DebugDraw.drawRay(worldPoints[next], worldPoints[prev], GREEN);

            //  draw normal   
            var midPoint = Vec3.multiply(0.5, Vec3.sum(worldPoints[prev], worldPoints[next]));
            DebugDraw.drawRay(midPoint, Vec3.sum(midPoint, worldNormals[next]), YELLOW);
            DebugDraw.drawRay(midPoint, Vec3.sum(midPoint, worldNormals[next+1]), YELLOW);
        }
    }
}

function computeCg() {
    // point mass.
    var n = JOINT_MASSES.length;
    var moments = {x: 0, y: 0, z: 0};
    var masses = 0;
    for (var i = 0; i < n; i++) {
        var pos = MyAvatar.getAbsoluteJointTranslationInObjectFrame(MyAvatar.getJointIndex(JOINT_MASSES[i].joint));
        JOINT_MASSES[i].pos = pos;
        moments = Vec3.sum(moments, Vec3.multiply(JOINT_MASSES[i].mass, pos));
        masses += JOINT_MASSES[i].mass;
    }
    return Vec3.multiply(1 / masses, moments);
}


function clamp(val, min, max) {
    return Math.max(min, Math.min(max, val));
}

function distancetoline(p1,p2,cg) {    
    var numerator = Math.abs((p2.z - p1.z)*(cg.x) - (p2.x - p1.x)*(cg.z) + (p2.x)*(p1.z) - (p2.z)*(p1.x));
    var denominator = Math.sqrt( Math.pow((p2.z - p1.z),2) + Math.pow((p2.x - p1.x),2));
	
    return numerator/denominator;   
}

function isLeft(a, b, c) {
    return (((b.x - a.x)*(c.z - a.z) - (b.z - a.z)*(c.x - a.x)) > 0);
}

function slope(num) {
    var constant = 1.0;
    return 1 - ( 1/(1+constant*num));
}

function dampenCgMovement(rawCg) {

    var distanceFromCenterZ = rawCg.z;
    var distanceFromCenterX = rawCg.x;

    //  clampFront = -0.10;
    //  clampBack = 0.17;
    //  clampLeft = -0.50;
    //  clampRight = 0.50;

    var dampedCg = { x: 0, y: 0, z: 0 };

    if (rawCg.z < 0.0) {
        var inputFront;
        inputFront = Math.abs(distanceFromCenterZ / clampFront);
        var scaleFrontNew = slope(inputFront);
        dampedCg.z = scaleFrontNew * clampFront;
    } else {
        //  cg.z > 0.0
        var inputBack;
        inputBack = Math.abs(distanceFromCenterZ / clampBack);
        var scaleBackNew = slope(inputBack);
        dampedCg.z = scaleBackNew * clampBack;
    }

    if (rawCg.x > 0.0) {
        var inputRight;
        inputRight = Math.abs(distanceFromCenterX / clampRight);
        var scaleRightNew = slope(inputRight);
        dampedCg.x = scaleRightNew * clampRight;
    } else {
        //  left of center
        var inputLeft;
        inputLeft = Math.abs(distanceFromCenterX / clampLeft);
        var scaleLeftNew = slope(inputLeft);
        dampedCg.x = scaleLeftNew * clampLeft;
    }
    return dampedCg;
}

function computeCounterBalance(desiredCgPos) {
    //   compute hips position to maintain desiredCg
    var HIPS_MASS = 40;
    var totalMass = JOINT_MASSES.reduce(function (accum, obj) {
        return accum + obj.mass;
    }, 0);
    var temp1 = Vec3.subtract(Vec3.multiply(totalMass + HIPS_MASS, desiredCgPos),
                              Vec3.multiply(JOINT_MASSES[0].mass, JOINT_MASSES[0].pos));
    var temp2 = Vec3.subtract(temp1,
                              Vec3.multiply(JOINT_MASSES[1].mass, JOINT_MASSES[1].pos));
    var temp3 = Vec3.subtract(temp2,
                              Vec3.multiply(JOINT_MASSES[2].mass, JOINT_MASSES[2].pos));
    var temp4 = Vec3.multiply(1 / HIPS_MASS, temp3);


    var currentHead = MyAvatar.getAbsoluteJointTranslationInObjectFrame(MyAvatar.getJointIndex("Head"));
    var tposeHead = MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(MyAvatar.getJointIndex("Head"));
    var tposeHips = MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(MyAvatar.getJointIndex("Hips"));

    var xzDiff = { x: (currentHead.x - temp4.x), y: 0, z: (currentHead.z - temp4.z) };
    var headMinusHipXz = Vec3.length(xzDiff);

    var headHipDefault = Vec3.length(Vec3.subtract(tposeHead, tposeHips));

    var hipHeight = Math.sqrt((headHipDefault * headHipDefault) - (headMinusHipXz * headMinusHipXz));

    temp4.y = (currentHead.y - hipHeight);
    if (temp4.y > tposeHips.y) {
        temp4.y = 0.0;
    }
    return temp4;
}

function update(dt) {

    var cg = computeCg();
    //  print("time elapsed " + dt);
    
    var desiredCg = { x: 0, y: 0, z: 0 };
    //  print("the raw cg " + cg.x + " " + cg.y + " " + cg.z);

    desiredCg.x = cg.x; 
    desiredCg.y = 0;
    desiredCg.z = cg.z; 

    desiredCg = dampenCgMovement(cg);
    
    cg.y = FLOOR_Y;

    //  after the dampening above it might be right to clamp the desiredcg to the edge of the base
    //  of support.

    if (DEBUGDRAWING) {
        DebugDraw.addMyAvatarMarker("left toe", IDENT_QUAT, leftToeEnd, BLUE);
        DebugDraw.addMyAvatarMarker("right toe", IDENT_QUAT, rightToeEnd, BLUE);
        DebugDraw.addMyAvatarMarker("cg", IDENT_QUAT, cg, BLUE);
        DebugDraw.addMyAvatarMarker("desiredCg", IDENT_QUAT, desiredCg, GREEN);
        drawBase(base);
    }

    var currentHeadPos = MyAvatar.getAbsoluteJointTranslationInObjectFrame(MyAvatar.getJointIndex("Head"));
    var localHipsPos = computeCounterBalance(desiredCg);
    //  print("current hips " + cg.x + " " + cg.y + " " + cg.z);
    //  print("dampened hips " + desiredCg.x + " " + desiredCg.y + " " + desiredCg.z)

    var globalPosRoot = MyAvatar.position;
    var globalRotRoot = Quat.normalize(MyAvatar.orientation);
    var inverseGlobalRotRoot = Quat.normalize(Quat.inverse(globalRotRoot));
    var globalPosHips = Vec3.sum(globalPosRoot, Vec3.multiplyQbyV(globalRotRoot, localHipsPos));
    var unRotatedHipsPosition;

    if (!MyAvatar.isRecenteringHorizontally()) {
        
        filteredHipsPosition = Vec3.mix(filteredHipsPosition, globalPosHips, 0.1);
        unRotatedHipsPosition = Vec3.multiplyQbyV(inverseGlobalRotRoot, Vec3.subtract(filteredHipsPosition, globalPosRoot));
        hipsPosition = Vec3.multiplyQbyV(ROT_Y180, unRotatedHipsPosition);
    } else {
        //  DebugDraw.addMarker("hipsunder", IDENT_QUAT, hipsUnderHead, GREEN);
        filteredHipsPosition = Vec3.mix(filteredHipsPosition, globalPosHips, 0.1);
        unRotatedHipsPosition = Vec3.multiplyQbyV(inverseGlobalRotRoot, Vec3.subtract(filteredHipsPosition, globalPosRoot));
        hipsPosition = Vec3.multiplyQbyV(ROT_Y180, unRotatedHipsPosition);
    }

    var newYaxisHips = Vec3.normalize(Vec3.subtract(currentHeadPos, unRotatedHipsPosition));
    var forward = { x: 0.0, y: 0.0, z: 1.0 };

    //  arms hip rotation is sent from the step script
    var oldZaxisHips = Vec3.normalize(Vec3.multiplyQbyV(armsHipRotation, forward));
    var newXaxisHips = Vec3.normalize(Vec3.cross(newYaxisHips, oldZaxisHips));
    var newZaxisHips = Vec3.normalize(Vec3.cross(newXaxisHips, newYaxisHips));

    //  var beforeHips = MyAvatar.getAbsoluteJointRotationInObjectFrame(MyAvatar.getJointIndex("Hips"));
    var left = { x: newXaxisHips.x, y: newXaxisHips.y, z: newXaxisHips.z, w: 0.0 };
    var up = { x: newYaxisHips.x, y: newYaxisHips.y, z: newYaxisHips.z, w: 0.0 };
    var view = { x: newZaxisHips.x, y: newZaxisHips.y, z: newZaxisHips.z, w: 0.0 };

    var translation = { x: 0.0, y: 0.0, z: 0.0, w: 1.0 };
    var newRotHips = Mat4.createFromColumns(left, up, view, translation);
    var finalRot = Mat4.extractRotation(newRotHips);
   
    hipsRotation = Quat.multiply(ROT_Y180, finalRot);
    print("final rot" + finalRot.x + " " + finalRot.y + " " + finalRot.z + " " + finalRot.w);

    if (DEBUGDRAWING) {
        DebugDraw.addMyAvatarMarker("hipsPos", IDENT_QUAT, hipsPosition, RED);
    }
}


Script.setTimeout(initCg, 10);
Script.scriptEnding.connect(function () {
    Script.update.disconnect(update);
    if (tablet) {
        tablet.removeButton(tabletButton);
    }
    Messages.messageReceived.disconnect(messageHandler);
    Messages.unsubscribe(MESSAGE_CHANNEL);
    
});
