var position = Vec3.sum(MyAvatar.position, { x: 0, y: -1, z: 0});

var scalingFactor = 0.01;

var sphereNaturalExtentsMin = { x: -1230, y: -1223, z: -1210 };
var sphereNaturalExtentsMax = { x: 1230, y: 1229, z: 1223 };
var panelsNaturalExtentsMin = { x: -1181, y: -326, z: 56 };
var panelsNaturalExtentsMax = { x: 1181, y: 576, z: 1183 };

var sphereNaturalDimensions = Vec3.subtract(sphereNaturalExtentsMax, sphereNaturalExtentsMin);
var panelsNaturalDimensions = Vec3.subtract(panelsNaturalExtentsMax, panelsNaturalExtentsMin);
Vec3.print("sphereNaturalDimensions:", sphereNaturalDimensions);
Vec3.print("panelsNaturalDimensions:", panelsNaturalDimensions);

var sphereNaturalCenter = Vec3.sum(sphereNaturalExtentsMin, Vec3.multiply(sphereNaturalDimensions, 0.5));
var panelsNaturalCenter = Vec3.sum(panelsNaturalExtentsMin, Vec3.multiply(panelsNaturalDimensions, 0.5));
Vec3.print("sphereNaturalCenter:", sphereNaturalCenter);
Vec3.print("panelsNaturalCenter:", panelsNaturalCenter);

var sphereDimensions = Vec3.multiply(sphereNaturalDimensions, scalingFactor);
var panelsDimensions = Vec3.multiply(panelsNaturalDimensions, scalingFactor);
Vec3.print("sphereDimensions:", sphereDimensions);
Vec3.print("panelsDimensions:", panelsDimensions);

var sphereCenter = Vec3.multiply(sphereNaturalCenter, scalingFactor);
var panelsCenter = Vec3.multiply(panelsNaturalCenter, scalingFactor);
Vec3.print("sphereCenter:", sphereCenter);
Vec3.print("panelsCenter:", panelsCenter);

var centerShift = Vec3.subtract(panelsCenter, sphereCenter);
Vec3.print("centerShift:", centerShift);

var spherePosition = position;
Vec3.print("spherePosition:", spherePosition);
var panelsPosition = Vec3.sum(spherePosition, centerShift);
Vec3.print("panelsPosition:", panelsPosition);


var screensOverlay = Overlays.addOverlay("model", {
                    position: panelsPosition,
                    dimensions: panelsDimensions,
                    url: "https://s3.amazonaws.com/hifi-public/models/sets/Lobby/LobbyConcepts/Lobby5_IsolatedPanelsFreezeTransforms.fbx"
                });
           
                
var structureOverlay = Overlays.addOverlay("model", {
                    position: spherePosition,
                    dimensions: sphereDimensions,
                    url: "https://s3.amazonaws.com/hifi-public/models/sets/Lobby/LobbyConcepts/Lobby5_OrbShellOnly.fbx",
                    ignoreRayIntersection: true, // we don't want to ray pick against any of this
                });

var statusText = Overlays.addOverlay("text", {
                    x: 200,
                    y: 100,
                    width: 200,
                    height: 20,
                    backgroundColor: { red: 0, green: 0, blue: 0},
                    alpha: 1.0,
                    backgroundAlpha: 1.0,
                    color: { red: 255, green: 255, blue: 255},
                    topMargin: 4,
                    leftMargin: 4,
                    text: "",
                });


Controller.mouseMoveEvent.connect(function(event){
    var pickRay = Camera.computePickRay(event.x, event.y);
    var result = Overlays.findRayIntersection(pickRay);

    if (result.intersects) {
        if (result.overlayID == screensOverlay) {
            Overlays.editOverlay(statusText, { text: "You are pointing at: " + result.extraInfo });
        } else {
            Overlays.editOverlay(statusText, { text: "You are not pointing at a panel..." });
        }
    } else {
        Overlays.editOverlay(statusText, { text: "You are not pointing at a panel..." });
    }
});


Script.scriptEnding.connect(function(){
    Overlays.deleteOverlay(screensOverlay);
    Overlays.deleteOverlay(structureOverlay);
    Overlays.deleteOverlay(statusText);
});
