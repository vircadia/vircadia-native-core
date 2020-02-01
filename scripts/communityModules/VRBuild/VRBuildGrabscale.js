var isGrabLeftInProgres = false;
var isGrabRightInProgres = false;
var isGrabLeftAndRight = false;
var grabbedLeftEntityID;
var grabbedRightEntityID;
var channelName = "Hifi-Object-Manipulation";
var closeEntities = [];
var entityToBeGrabbedID = null;
var RIGHT_JOINT = MyAvatar.getJointIndex("RightHandMiddle1");
var LEFT_JOINT = MyAvatar.getJointIndex("LeftHandMiddle1");
var startHandDistance = 0;
var isScaling = false;
var newScale = 1;
var oldDimensions;
var newDimensions = {x:1,y:1,z:1};
var overlayPosition;
var entityPosition = {x:0,y:0,z:0};
var entityRotation;
var isDirectionFound =  false;
var handPositionLeft;
var handPositionRight;
var GIZMO_CYLINDER_LENGTH = 0.3;
var GIZMO_CYLINDER_DIAMETER = GIZMO_CYLINDER_LENGTH / 40;
var GIZMO_CENTER_DIAMETER  = 0.2;
var GIZMO_END_DIAMETER = 0.02;
var GIZMO_END_POSITION = GIZMO_CYLINDER_LENGTH / 2;
var GIZMO_DEFAULT_SCALE = 0.2;
var gizmoShapeIDs = [];

var minimum = 1000;
var index = 0; 

var distanceToGizmoXplus = 0;
var distanceToGizmoXmin = 0;
var distanceToGizmoYplus = 0;
var distanceToGizmoYmin = 0;
var distanceToGizmoZplus = 0;
var distanceToGizmoZmin = 0;

var isScaleModeX = false;
var isScaleModeY = false;
var isScaleModeZ = false;

var getClosest = [];

var overlayID = Overlays.addOverlay('text3d', {
    text: 'hover',
    visible: true,
    backgroundAlpha: 0.2,
    isFacingAvatar: true,
    lineHeight: 0.05,
    dimensions: {x:2,y:0.5,z:0.5},
    drawInFront:true
  },"local");  

function createGizmo(){    
  gizmoCenterID = Entities.addEntity({
    type: "Shape",        
    shape: "Sphere",                       
    name:"gizmoCenter",        
    description:"",            
    position: { x: 0, y: 0, z: 0},      
    lifetime: -1,
    color:{r:100,g:100,b:100},
    alpha:0.5,
    dimensions: {x:GIZMO_CENTER_DIAMETER,y:GIZMO_CENTER_DIAMETER,z:GIZMO_CENTER_DIAMETER},
    collisionless:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoCenterID);
  
  var tempRotation = Quat.fromPitchYawRollRadians(0,0,0);
  gizmoXaxisID = Entities.addEntity({
    type: "Shape",        
    shape: "Cylinder",                       
    name:"x-axis",
    parentID:gizmoCenterID,        
    description:"",            
    localPosition: {x:0,y:0,z:0},
    localRotation: tempRotation,      
    lifetime: -1,
    color:{r:100,g:0,b:0},
    alpha:1,
    dimensions: {x:GIZMO_CYLINDER_LENGTH,y:GIZMO_END_DIAMETER,z:GIZMO_END_DIAMETER},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoXaxisID);

  var tempRotation = Quat.fromPitchYawRollRadians(0,0,-Math.PI/2);  
  gizmoYaxisID = Entities.addEntity({
    type: "Shape",        
    shape: "Cylinder",                       
    name:"y-axis",
    parentID:gizmoCenterID,        
    description:"",            
    localPosition: {x:0,y:0,z:0},
    localRotation: tempRotation,      
    lifetime: -1,
    color:{r:0,g:100,b:0},
    alpha:1,
    dimensions: {x:GIZMO_END_DIAMETER,y:GIZMO_CYLINDER_LENGTH,z:GIZMO_END_DIAMETER},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoYaxisID);

  var tempRotation = Quat.fromPitchYawRollRadians(-Math.PI/2,0,0);
  gizmoZaxisID = Entities.addEntity({
    type: "Shape",        
    shape: "Cylinder",                       
    name:"z-axis",
    parentID:gizmoCenterID,        
    description:"",            
    localPosition: {x:0,y:0,z:0},
    localRotation: tempRotation,      
    lifetime: -1,
    color:{r:0,g:0,b:100},
    alpha:1,
    dimensions: {x:GIZMO_END_DIAMETER,y:GIZMO_END_DIAMETER,z:GIZMO_CYLINDER_LENGTH},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoZaxisID);

  gizmoXplusID = Entities.addEntity({
    type: "Shape",        
    shape: "Sphere",                       
    name:"x-plus",
    parentID:gizmoXaxisID,        
    description:"",            
    localPosition: {x:0,y:GIZMO_END_POSITION,z:0},         
    lifetime: -1,
    color:{r:255,g:0,b:0},
    alpha:1,
    dimensions: {x:GIZMO_END_DIAMETER,y:GIZMO_END_DIAMETER,z:GIZMO_END_DIAMETER},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoXplusID);

  gizmoXminusID = Entities.addEntity({
    type: "Shape",        
    shape: "Sphere",                       
    name:"x-minus",
    parentID:gizmoXaxisID,        
    description:"",            
    localPosition: {x:0,y:-GIZMO_END_POSITION,z:0},
    lifetime: -1,
    color:{r:255,g:0,b:0},
    alpha:1,
    dimensions: {x:GIZMO_END_DIAMETER,y:GIZMO_END_DIAMETER,z:GIZMO_END_DIAMETER},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoXminusID);

  gizmoYplusID = Entities.addEntity({
    type: "Shape",        
    shape: "Sphere",                       
    name:"y-plus",
    parentID:gizmoYaxisID,        
    description:"",            
    localPosition: {x:0,y:GIZMO_END_POSITION,z:0},         
    lifetime: -1,
    color:{r:0,g:255,b:0},
    alpha:1,
    dimensions: {x:GIZMO_END_DIAMETER,y:GIZMO_END_DIAMETER,z:GIZMO_END_DIAMETER},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoYplusID);

  gizmoYminusID = Entities.addEntity({
    type: "Shape",        
    shape: "Sphere",                       
    name:"y-minus",
    parentID:gizmoYaxisID,        
    description:"",            
    localPosition: {x:0,y:-GIZMO_END_POSITION,z:0},
    lifetime: -1,
    color:{r:0,g:255,b:0},
    alpha:1,
    dimensions: {x:GIZMO_END_DIAMETER,y:GIZMO_END_DIAMETER,z:GIZMO_END_DIAMETER},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoYminusID);

  gizmoZplusID = Entities.addEntity({
    type: "Shape",        
    shape: "Sphere",                       
    name:"z-plus",
    parentID:gizmoZaxisID,        
    description:"",            
    localPosition: {x:0,y:GIZMO_END_POSITION,z:0},         
    lifetime: -1,
    color:{r:0,g:0,b:255},
    alpha:1,
    dimensions: {x:GIZMO_END_DIAMETER,y:GIZMO_END_DIAMETER,z:GIZMO_END_DIAMETER},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoZplusID);

  gizmoZminusID = Entities.addEntity({
    type: "Shape",        
    shape: "Sphere",                       
    name:"z-minus",
    parentID:gizmoZaxisID,        
    description:"",            
    localPosition: {x:0,y:-GIZMO_END_POSITION,z:0},
    lifetime: -1,
    color:{r:0,g:0,b:255},
    alpha:1,
    dimensions: {x:GIZMO_END_DIAMETER,y:GIZMO_END_DIAMETER,z:GIZMO_END_DIAMETER},
    ignoreForCollisions:true,
    userData:"{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}"    
  });
  gizmoShapeIDs.push(gizmoZminusID);
}

function updateOverlay(overlayID) {
overlayPosition = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 2, z: -2 }));  
    var text = [
        "  GrabLeft: " + isGrabLeftInProgres + "     GrabRight: " + isGrabRightInProgres + "     isGrabLeftAndRight: " + isGrabLeftAndRight,
        "  isScaling: " + isScaling + " newScale: " + newScale,
        "  HandposLeft:  " + JSON.stringify(Vec3.subtract(handPositionLeft,MyAvatar.position)),
        "  HandposRight:  " + JSON.stringify(Vec3.subtract(handPositionRight,MyAvatar.position)),        
        "  dxplus: " + distanceToGizmoXplus.toFixed(4) + "  dyplus: " + distanceToGizmoYplus.toFixed(4) + "  dzplus: " + distanceToGizmoZplus.toFixed(4),
        "  dxmin: " + distanceToGizmoXmin.toFixed(4) + "  dymin: " + distanceToGizmoYmin.toFixed(4) + "  dzmin: " + distanceToGizmoZmin.toFixed(4),
        "  isScaleModeX:  " + isScaleModeX + "  isScaleModeY:  " + isScaleModeY +"  isScaleModeZ:  " + isScaleModeZ,
        "  index:   "+ index + "minimum:   " + minimum        
    ].filter(Boolean).join('\n');

    Overlays.editOverlay(overlayID, {
        text: text,
        position: overlayPosition      
    });
}

function updateGizmoPositionRotation(gizmoPos,gizmoRot){
    var tempRotation = Quat.fromPitchYawRollRadians(0,0,-Math.PI/2);
    Entities.editEntity(gizmoCenterID,{
        position: gizmoPos,        
        rotation:Quat.multiply(gizmoRot,tempRotation)
    });
}

function updateGizmoScale(scaleUpdate){
    Entities.editEntity(gizmoXaxisID,{        
        dimensions:{x:GIZMO_CYLINDER_DIAMETER,y:GIZMO_CYLINDER_LENGTH*scaleUpdate*2,z:GIZMO_CYLINDER_DIAMETER}        
    });
    Entities.editEntity(gizmoYaxisID,{        
        dimensions:{x:GIZMO_CYLINDER_DIAMETER,y:GIZMO_CYLINDER_LENGTH*scaleUpdate*2,z:GIZMO_CYLINDER_DIAMETER}       
    });
    Entities.editEntity(gizmoZaxisID,{        
        dimensions:{x:GIZMO_CYLINDER_DIAMETER,y:GIZMO_CYLINDER_LENGTH*scaleUpdate*2,z:GIZMO_CYLINDER_DIAMETER}       
    });
    Entities.editEntity(gizmoXplusID,{
        localPosition: {x:0,y:GIZMO_END_POSITION*scaleUpdate*2,z:0}       
    });
    Entities.editEntity(gizmoXminusID,{
        localPosition: {x:0,y:-GIZMO_END_POSITION*scaleUpdate*2,z:0}       
    });
    Entities.editEntity(gizmoYplusID,{
        localPosition: {x:0,y:GIZMO_END_POSITION*scaleUpdate*2,z:0}       
    });
    Entities.editEntity(gizmoYminusID,{
        localPosition: {x:0,y:-GIZMO_END_POSITION*scaleUpdate*2,z:0}       
    });
    Entities.editEntity(gizmoZplusID,{
        localPosition: {x:0,y:GIZMO_END_POSITION*scaleUpdate*2,z:0}       
    });
    Entities.editEntity(gizmoZminusID,{
        localPosition: {x:0,y:-GIZMO_END_POSITION*scaleUpdate*2,z:0}       
    });   
}

function hideGizmo(){
    for(i in gizmoShapeIDs){
        Entities.editEntity(gizmoShapeIDs[i],{visible: false});
    }
}

function showGizmo(){
    for(i in gizmoShapeIDs){
        Entities.editEntity(gizmoShapeIDs[i],{visible: true});
    }
}

function checkIfEntityIsInGizmo(checkID){
    var check = false;       
            for(i in gizmoShapeIDs){          
                if (gizmoShapeIDs[i] == checkID){
                check = true;
                }
            }  
            if(check){
                return true
            }
            else{
                return false
            }
};

function startScaling(){    
    if(!isScaling){        
        showGizmo();
        var startHandPositionLeft = MyAvatar.getJointPosition(LEFT_JOINT);
        var startHandPositionRight = MyAvatar.getJointPosition(RIGHT_JOINT);
        startHandDistance = Vec3.distance(startHandPositionLeft,startHandPositionRight);
        isScaling = true;
        oldDimensions = Entities.getEntityProperties(entityToBeGrabbedID,["dimensions"]).dimensions;        
    }
    handDistance = Vec3.distance(handPositionLeft,handPositionRight);
    newScale = handDistance / startHandDistance;
    if(isScaleModeX){
        newDimensions = {x:oldDimensions.x * newScale ,y:oldDimensions.y,z:oldDimensions.z};
    } 
    if(isScaleModeY){
        newDimensions = {x:oldDimensions.x  ,y:oldDimensions.y* newScale,z:oldDimensions.z};
    }
    if(isScaleModeZ){
        newDimensions = {x:oldDimensions.x  ,y:oldDimensions.y,z:oldDimensions.z* newScale};
    }    
    Entities.editEntity(entityToBeGrabbedID,{dimensions:newDimensions});
    updateGizmoScale(newScale*GIZMO_DEFAULT_SCALE);       
}


function startBuilding(){
    updateOverlay();    
    if(isGrabLeftInProgres && isGrabRightInProgres){
        if(grabbedLeftEntityID){entityToBeGrabbedID = grabbedLeftEntityID};
        if(grabbedRightEntityID){entityToBeGrabbedID = grabbedRightEntityID};
        isGrabLeftAndRight = true;               
    }
    else{ 
        entityToBeGrabbedID = null;             
        isGrabLeftAndRight = false; 
        isScaling = false;
        isDirectionFound = false;
        isScaleModeX = false;
        isScaleModeY = false;
        isScaleModeZ = false;
        newScale = 2;
        hideGizmo();       
    }

    if(isGrabLeftAndRight){        
        startScaling();
    }

    if (entityToBeGrabbedID){
        entityPosition = Entities.getEntityProperties(entityToBeGrabbedID,["position"]).position;
        entityRotation = Entities.getEntityProperties(entityToBeGrabbedID,["rotation"]).rotation;
        handPositionLeft = MyAvatar.getJointPosition(LEFT_JOINT);
        handPositionRight = MyAvatar.getJointPosition(RIGHT_JOINT);       

        updateGizmoPositionRotation(entityPosition,entityRotation);

        var gizmoXplusPosition = Entities.getEntityProperties(gizmoXplusID,["localPosition"]).localPosition;
        var gizmoXplusWorldPosition = Entities.localToWorldPosition(gizmoXplusPosition, gizmoXaxisID,-1,false);
        distanceToGizmoXplus = Vec3.distance(handPositionLeft,gizmoXplusWorldPosition);
        getClosest[0] = distanceToGizmoXplus;

        var gizmoXminusPosition = Entities.getEntityProperties(gizmoXminusID,["localPosition"]).localPosition;
        var gizmoXminusWorldPosition = Entities.localToWorldPosition(gizmoXminusPosition, gizmoXaxisID,-1,false);
        distanceToGizmoXmin = Vec3.distance(handPositionLeft,gizmoXminusWorldPosition);
        getClosest[1] = distanceToGizmoXmin;

        var gizmoYplusPosition = Entities.getEntityProperties(gizmoYplusID,["localPosition"]).localPosition;
        var gizmoYplusWorldPosition = Entities.localToWorldPosition(gizmoYplusPosition, gizmoYaxisID,-1,false);
        distanceToGizmoYplus = Vec3.distance(handPositionLeft,gizmoYplusWorldPosition);
        getClosest[2] = distanceToGizmoYplus;

        var gizmoYminusPosition = Entities.getEntityProperties(gizmoYminusID,["localPosition"]).localPosition;
        var gizmoYminusWorldPosition = Entities.localToWorldPosition(gizmoYminusPosition, gizmoYaxisID,-1,false);
        distanceToGizmoYmin = Vec3.distance(handPositionLeft,gizmoYminusWorldPosition);
        getClosest[3] = distanceToGizmoYmin;

        var gizmoZplusPosition = Entities.getEntityProperties(gizmoZplusID,["localPosition"]).localPosition;
        var gizmoZplusWorldPosition = Entities.localToWorldPosition(gizmoZplusPosition, gizmoZaxisID,-1,false);
        distanceToGizmoZplus = Vec3.distance(handPositionLeft,gizmoZplusWorldPosition);
        getClosest[4] = distanceToGizmoZplus;

        var gizmoZminusPosition = Entities.getEntityProperties(gizmoZminusID,["localPosition"]).localPosition;
        var gizmoZminusWorldPosition = Entities.localToWorldPosition(gizmoZminusPosition, gizmoZaxisID,-1,false);
        distanceToGizmoZmin = Vec3.distance(handPositionLeft,gizmoZminusWorldPosition);
        getClosest[5] = distanceToGizmoZmin;

        minimum = Math.min(getClosest[0],getClosest[1],getClosest[2],getClosest[3],getClosest[4],getClosest[5]);        
       
            index = getClosest.indexOf(minimum);
            if (index == 0 || index == 1){
                if(!isDirectionFound){
                    isScaleModeX = true;
                    isScaleModeY = false;
                    isScaleModeZ = false;
                    isDirectionFound = true;
                }
            }
            if (index == 2 || index == 3){
                if(!isDirectionFound){
                    isScaleModeY = false;
                    isScaleModeY = true;
                    isScaleModeZ = false;
                    isDirectionFound = true;
                }
            }
            if (index == 4 || index == 5){
                if(!isDirectionFound){
                    isScaleModeZ = false;
                    isScaleModeY = false;
                    isScaleModeZ = true;
                    isDirectionFound = true;
                }
            }       
    }
}

Script.scriptEnding.connect(function () {
    Entities.deleteEntity( overlayID);     
    Entities.deleteEntity( gizmoCenterID);
    Script.update.disconnect(startBuilding);
});

function onMessageReceived(channel, message) {
    if(channel == channelName){   
        var action = JSON.parse(message).action;
        actionHand = JSON.parse(message).joint;  
        if(action == "grab"){
            if(actionHand == "LeftHand"){
                grabbedLeftEntityID = JSON.parse(message).grabbedEntity;
                var check = checkIfEntityIsInGizmo(grabbedLeftEntityID);
                if(!check){         
                    isGrabLeftInProgres = true;                    
                }
            }
            if(actionHand == "RightHand"){
                grabbedRightEntityID = JSON.parse(message).grabbedEntity;
                var check = checkIfEntityIsInGizmo(grabbedLeftEntityID);
                if(!check){         
                    isGrabRightInProgres = true;               
                }                
            }
        }
        if(action == "release"){
            if(actionHand == "LeftHand"){   
                isGrabLeftInProgres = false;
                grabbedLeftEntityID = null;
            }
            if(actionHand == "RightHand"){                  
                isGrabRightInProgres = false;
                grabbedRightEntityID = null;
            }
        }
    }
}

Messages.subscribe(channelName);
Messages.messageReceived.connect(onMessageReceived);
createGizmo();
updateGizmoScale(GIZMO_DEFAULT_SCALE);
Script.update.connect(startBuilding);


