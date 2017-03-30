//
//  Created by James B. Pollack @imgntn on April 18, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
// This script shows how to create an entity with a picture texture on it that you can change either in script or in the entity's textures property.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  familiar code to put the entity in front of us


var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(1, Quat.getForward(Camera.getOrientation())));

// this is just a model exported from blender with a texture named 'Picture' on one face.  also made it emissive so it doesn't require lighting.
var MODEL_URL = "http://hifi-production.s3.amazonaws.com/tutorials/pictureFrame/finalFrame.fbx";

//this is where we are going to get our image from.  the stuff at the end is our API key.
var NASA_API_ENDPOINT = "https://api.nasa.gov/planetary/apod?api_key=XNmgPJvVK8hGroZHB19PaQtlqKZk4q8GorWViuND";

//actually go get the data and return it
function getDataFromNASA() {
    var request = new XMLHttpRequest();
    request.open("GET", NASA_API_ENDPOINT, false);
    request.send();

    var response = JSON.parse(request.responseText);
    return response;
};

//make the picture frame and set its texture url to the picture of the day from NASA
function makePictureFrame() {
    // Calculate rotation necessary to face picture towards user at spawn time.
    var rotation = Quat.multiply(Quat.fromPitchYawRollDegrees(0, 180, 0), Camera.getOrientation());
    rotation.x = 0;
    rotation.z = 0;
    var data = getDataFromNASA();
    var pictureFrameProperties = {
        name: 'Tutorial Picture Frame',
        description: data.explanation,
        type: 'Model',
        dimensions: {
            x: 1.2,
            y: 0.9,
            z: 0.075
        },
        position: center,
        rotation: rotation,
        textures: JSON.stringify({
            Picture: data.url
        }),
        modelURL: MODEL_URL,
        lifetime: 3600,
        dynamic: true,
    }
    var pictureFrame = Entities.addEntity(pictureFrameProperties);

    var OUTER_FRAME_MODEL_URL = "http://hifi-production.s3.amazonaws.com/tutorials/pictureFrame/outer_frame.fbx";
    var outerFrameProps = {
        name: "Tutorial Outer Frame",
        type: "Model",
        position: center,
        rotation: rotation,
        modelURL: OUTER_FRAME_MODEL_URL,
        lifetime: 3600,
        dynamic: true,
        dimensions: {
            x: 1.4329,
            y: 1.1308,
            z: 0.0464
        },
        parentID: pictureFrame // A parentd object will move, rotate, and scale with its parent.
    }
    var outerFrame = Entities.addEntity(outerFrameProps);
    Script.stop();
}


makePictureFrame();

//the data the NASA API returns looks like this: 
//
// {
// date: "2016-04-18",
// explanation: "The International Space Station is the largest object ever constructed by humans in space. The station perimeter extends over roughly the area of a football field, although only a small fraction of this is composed of modules habitable by humans. The station is so large that it could not be launched all at once -- it continues to be built piecemeal. To function, the ISS needs huge trusses, some over 15 meters long and with masses over 10,000 kilograms, to keep it rigid and to route electricity and liquid coolants. Pictured above, the immense space station was photographed from the now-retired space shuttle Atlantis after a week-long stay in 2010. Across the image top hangs part of a bright blue Earth, in stark contrast to the darkness of interstellar space across the bottom.",
// hdurl: "http://apod.nasa.gov/apod/image/1604/ISS02_NASA_4288.jpg",
// media_type: "image",
// service_version: "v1",
// title: "The International Space Station over Earth",
// url: "http://apod.nasa.gov/apod/image/1604/ISS02_NASA_960.jpg"
// }