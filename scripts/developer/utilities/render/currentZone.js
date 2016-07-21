//
//  currentZone.js
//  examples/utilities/tools/render
//
//  Sam Gateau created on 6/18/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
/*var qml = Script.resolvePath('framebuffer.qml');
var window = new OverlayWindow({
    title: 'Framebuffer Debug',
    source: qml,
    width: 400, height: 400,
});
window.setPosition(25, 50);
window.closed.connect(function() { Script.stop(); });
*/



function findCurrentZones() {
    var foundEntitiesArray = Entities.findEntities(MyAvatar.position, 2.0);
    //print(foundEntitiesArray.length);
    var zones = [];

    foundEntitiesArray.forEach(function(foundID){
        var properties = Entities.getEntityProperties(foundID);
        if (properties.type == "Zone") {
            zones.push(foundID);
        } 
    });
    return zones;
}


var currentZone;
var currentZoneProperties;

function setCurrentZone(newCurrentZone) {
    if (currentZone == newCurrentZone) {
        return;
    }
        
    currentZone = newCurrentZone;
    currentZoneProperties = Entities.getEntityProperties(currentZone);

    print(JSON.stringify(currentZoneProperties));
}

var checkCurrentZone = function() {
   
    var currentZones = findCurrentZones();
    if (currentZones.length > 0) {
        if (currentZone != currentZones[0]) {
            print("New Zone");
            setCurrentZone(currentZones[0]);
        }
    }
   
}
var ticker = Script.setInterval(checkCurrentZone, 2000);

//checkCurrentZone();

function onQuit() {
    Script.clearInterval(ticker);
    print("Quit Zone");
}
Script.scriptEnding.connect(onQuit);
