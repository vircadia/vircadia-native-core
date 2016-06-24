// earthquakes_live.js
//
// exploratory implementation in prep for abstract latlong to earth graphing tool for VR
// shows all of the quakes in the past 24 hours reported by the USGS
//
// created by james b. pollack @imgntn on 12/5/2015
//  Copyright 2015 High Fidelity, Inc.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// working notes: maybe try doing markers as boxes,rotated to the sphere normal, and with the height representing some value

Script.include('../libraries/promise.js');
var Promise = loadPromise();

Script.include('../libraries/tinyColor.js');
var tinyColor = loadTinyColor();

//you could make it the size of the actual earth. 
var EARTH_SPHERE_RADIUS = 6371;
var EARTH_SPHERE_RADIUS = 2;

var EARTH_CENTER_POSITION = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(EARTH_SPHERE_RADIUS, Quat.getFront(Camera.getOrientation())));

var EARTH_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/james/earthquakes_live/models/earth-noclouds.fbx';

var SHOULD_SPIN=false;
var POLL_FOR_CHANGES = true;
//USGS updates the data every five minutes
var CHECK_QUAKE_FREQUENCY = 5 * 60 * 1000;

var QUAKE_MARKER_DIMENSIONS = {
    x: 0.01,
    y: 0.01,
    z: 0.01
};

function createEarth() {
    var earthProperties = {
        name: 'Earth',
        type: 'Model',
        modelURL: EARTH_MODEL_URL,
        position: EARTH_CENTER_POSITION,
        dimensions: {
            x: EARTH_SPHERE_RADIUS,
            y: EARTH_SPHERE_RADIUS,
            z: EARTH_SPHERE_RADIUS
        },
        rotation: Quat.fromPitchYawRollDegrees(0, 90, 0),
        // dynamic: true,
        //if you have a shapetype it blocks the smaller markers
        // shapeType:'sphere'
        // userData: JSON.stringify({
        //     grabbableKey: {
        //         grabbable: false
        //     }
        // })
    }

    return Entities.addEntity(earthProperties)
}

function latLongToVector3(lat, lon, radius, height) {
    var phi = (lat) * Math.PI / 180;
    var theta = (lon - 180) * Math.PI / 180;

    var x = -(radius + height) * Math.cos(phi) * Math.cos(theta);
    var y = (radius + height) * Math.sin(phi);
    var z = (radius + height) * Math.cos(phi) * Math.sin(theta);

    return {
        x: x,
        y: y,
        z: z
    };
}

function getQuakePosition(earthquake) {
    var longitude = earthquake.geometry.coordinates[0];
    var latitude = earthquake.geometry.coordinates[1];
    var depth = earthquake.geometry.coordinates[2];

    var latlng = latLongToVector3(latitude, longitude, EARTH_SPHERE_RADIUS / 2, 0);

    var position = EARTH_CENTER_POSITION;
    var finalPosition = Vec3.sum(position, latlng);

    //print('finalpos::' + JSON.stringify(finalPosition))
    return finalPosition
}

var QUAKE_URL = 'http://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_day.geojson'

function get(url) {
    print('getting' + url)
        // Return a new promise.
    return new Promise(function(resolve, reject) {
        // Do the usual XHR stuff
        var req = new XMLHttpRequest();
        req.open('GET', url);
        req.onreadystatechange = function() {
            print('req status:: ' + JSON.stringify(req.status))

            if (req.readyState == 4 && req.status == 200) {
                var myArr = JSON.parse(req.responseText);
                resolve(myArr);
            }
        };

        req.send();
    });
}

function createQuakeMarker(earthquake) {
    var markerProperties = {
        name: earthquake.properties.place,
        type: 'Sphere',
        parentID:earth,
        dimensions: QUAKE_MARKER_DIMENSIONS,
        position: getQuakePosition(earthquake),
        collisionless:true,
        lifetime: 6000,
        color: getQuakeMarkerColor(earthquake)
    }

    //  print('marker properties::' + JSON.stringify(markerProperties))
    return Entities.addEntity(markerProperties);
}

function getQuakeMarkerColor(earthquake) {
    var color = {};
    var magnitude = earthquake.properties.mag;
    //realistic but will never get full red coloring and will probably be pretty dull for most.  must experiment
    var sValue = scale(magnitude, 0, 10, 0, 100);
    var HSL_string = "hsl(0, " + sValue + "%, 50%)"
    var color = tinyColor(HSL_string);
    var finalColor = {
        red: color._r,
        green: color._g,
        blue: color._b
    }

    return finalColor
}

function scale(value, min1, max1, min2, max2) {
    return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}

function processQuakes(earthquakes) {
    print('quakers length' + earthquakes.length)
    earthquakes.forEach(function(quake) {
        // print('PROCESSING A QUAKE')
        var marker = createQuakeMarker(quake);
        markers.push(marker);
    })
    print('markers length:' + markers.length)
}

var quakes;
var markers = [];

var earth = createEarth();

function getThenProcessQuakes() {
    get(QUAKE_URL).then(function(response) {
        print('got it::' + response.features.length)
        quakes = response.features;
        processQuakes(quakes);
        //print("Success!" + JSON.stringify(response));
    }, function(error) {
        print('error getting quakes')
    });
}

function cleanupMarkers() {
    print('CLEANING UP MARKERS')
    while (markers.length > 0) {
        Entities.deleteEntity(markers.pop());
    }
}

function cleanupEarth() {
    Entities.deleteEntity(earth);
    Script.update.disconnect(spinEarth);
}

function cleanupInterval() {
    if (pollingInterval !== null) {
        Script.clearInterval(pollingInterval)
    }
}

Script.scriptEnding.connect(cleanupMarkers);
Script.scriptEnding.connect(cleanupEarth);
Script.scriptEnding.connect(cleanupInterval);

getThenProcessQuakes();

var pollingInterval = null;

if (POLL_FOR_CHANGES === true) {
    pollingInterval = Script.setInterval(function() {
        cleanupMarkers();
        getThenProcessQuakes()
    }, CHECK_QUAKE_FREQUENCY)
}


function spinEarth(){
Entities.editEntity(earth,{
    angularVelocity:{
        x:0,
        y:0.25,
        z:0
    }
})
}

if(SHOULD_SPIN===true){
    Script.update.connect(spinEarth);
}
