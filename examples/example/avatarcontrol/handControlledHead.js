// handControlledHead.js

//This script allows you to look around, driving the rotation of the avatar's head by the right hand orientation.

const YAW_MULTIPLIER = 20000;
const PITCH_MULTIPLIER = 15000;
const EPSILON = 0.001;
var firstPress = true;
var handPreviousVerticalRotation = 0.0;
var handCurrentVerticalRotation = 0.0;
var handPreviousHorizontalRotation = 0.0;
var handCurrentHorizontalRotation = 0.0;
var rotatedHandPosition;
var rotatedTipPosition;

function update(deltaTime) {
    if(Controller.getValue(Controller.Standard.RightPrimaryThumb)){
        pitchManager(deltaTime);
    }else if(!firstPress){
        firstPress = true;
    }
    if(firstPress && MyAvatar.headYaw){
        MyAvatar.headYaw -= MyAvatar.headYaw/10;
    }
    
}

function pitchManager(deltaTime){
    
    rotatedHandPosition = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, -MyAvatar.bodyYaw, 0), MyAvatar.getRightHandPosition());
    rotatedTipPosition = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, -MyAvatar.bodyYaw, 0), MyAvatar.getRightHandTipPosition());
    
    handCurrentVerticalRotation = Vec3.subtract(rotatedTipPosition, rotatedHandPosition).y;
    handCurrentHorizontalRotation = Vec3.subtract(rotatedTipPosition, rotatedHandPosition).x;
    
    var handCurrentHorizontalRotationFiltered = handCurrentHorizontalRotation;
   
    //to avoid yaw drift
    if((handCurrentHorizontalRotation - handPreviousHorizontalRotation) < EPSILON && (handCurrentHorizontalRotation - handPreviousHorizontalRotation) > -EPSILON){
        handCurrentHorizontalRotationFiltered = handPreviousHorizontalRotation;
    }
    
    if(firstPress){
        handPreviousVerticalRotation = handCurrentVerticalRotation;
        handPreviousHorizontalRotation = handCurrentHorizontalRotation;
        firstPress = false;
    }
    
    MyAvatar.headPitch += (handCurrentVerticalRotation - handPreviousVerticalRotation)*PITCH_MULTIPLIER*deltaTime;
    MyAvatar.headYaw -= (handCurrentHorizontalRotationFiltered - handPreviousHorizontalRotation)*YAW_MULTIPLIER*deltaTime;
    
    
    handPreviousVerticalRotation = handCurrentVerticalRotation;
    handPreviousHorizontalRotation = handCurrentHorizontalRotationFiltered;
    

}

function clean(){
    MyAvatar.headYaw = 0.0;
}


Script.update.connect(update);
Script.scriptEnding.connect(clean);