//earthquakes_live.js
//created by james b. pollack @imgntn on 12/5/2015
Script.include('../libraries/promise.js');
var Promise = loadPromise();

Script.include('../libraries/tinyColor.js');
var tinyColor = loadTinyColor();

var EARTH_SPHERE_RADIUS = 5;
var EARTH_CENTER_POSITION = Vec3.sum(MyAvatar.position, {
    x: 5,
    y: 0,
    z: 0
});

var EARTH_MODEL_URL='http://public.highfidelity.io/marketplace/hificontent/Scripts/planets/planets/earth.fbx';
//USGS updates the data every five minutes
var CHECK_QUAKE_FREQUENCY = 300 * 1000;

var QUAKE_MARKER_DIMENSIONS = {
    x: 0.1,
    y: 0.1,
    z: 0.1
};

function createEarth() {
    var earthProperties = {
        name: 'Earth',
        type: 'Model',
        modelURL:EARTH_MODEL_URL,
        position: EARTH_CENTER_POSITION,
        dimensions: {
            x: EARTH_SPHERE_RADIUS,
            y: EARTH_SPHERE_RADIUS,
            z: EARTH_SPHERE_RADIUS
        },
        // color: {
        //     red: 0,
        //     green: 100,
        //     blue: 150
        // },
        collisionsWillMove: false,
        userData: JSON.stringify({
            grabbableKey: {
                grabbable: false
            }
        })
    }

    return Entities.addEntity(earthProperties)
}

function plotLatitudeLongitude(radiusOfSphere, latitude, longitude) {
    var tx = radiusOfSphere * Math.cos(latitude) * Math.cos(longitude);
    var ty = radiusOfSphere * -Math.sin(latitude);
    var tz = radiusOfSphere * Math.cos(latitude) * Math.sin(longitude);
    return {
        x: tx,
        y: ty,
        z: tz
    }
}

function getQuakePosition(earthquake) {
    var latitude = earthquake.geometry.coordinates[0];
    var longitude = earthquake.geometry.coordinates[1];
    var latlng = plotLatitudeLongitude(2.5, latitude, longitude);

    var position = EARTH_CENTER_POSITION;
    var finalPosition = Vec3.sum(position, latlng);

   // print('finalpos::' + JSON.stringify(finalPosition))
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

function showEarthquake(snapshot) {
    var earthquake = snapshot.val();
    print("Mag " + earthquake.mag + " at " + earthquake.place);
}

function createQuakeMarker(earthquake) {
    var markerProperties = {
        name: 'Marker',
        type: 'Sphere',
        dimensions: QUAKE_MARKER_DIMENSIONS,
        position: getQuakePosition(earthquake),
        lifetime: 6000,
        color: getQuakeMarkerColor(earthquake)
    }

    //print('marker properties::' + JSON.stringify(markerProperties))
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

var quakea;
var markers = [];

var earth =  createEarth();

get(QUAKE_URL).then(function(response) {
    print('got it::' + response.features.length)
    quakes = response.features;
    processQuakes(quakes);
    //print("Success!" + JSON.stringify(response));
}, function(error) {
    print('error getting quakes')
});

function cleanupMarkers() {
    print('CLEANING UP MARKERS')
    while (markers.length > 0) {
        Entities.deleteEntity(markers.pop());
    }
}

function cleanupEarth() {
    Entities.deleteEntity(earth);
}

Script.scriptEnding.connect(cleanupMarkers);
Script.scriptEnding.connect(cleanupEarth);